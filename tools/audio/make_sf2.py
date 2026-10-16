#!/usr/bin/env python3
"""Build a SoundFont 2 (.sf2) that fluidsynth can play from folders of WAV samples.

Layout (one folder per instrument; the number is the MIDI program it answers to, bank 0):

  kit/
    000_piano/          C3.wav  C4.wav  C5.wav   -> multi-sampled; each note maps to its nearest sample
    033_bass/           bass.wav                 -> one sample, root key 60 (or set "root" in instrument.json)
    120_sfx/            72.wav  60.wav  ...      -> with {"kit": true}: each file plays only on its own key, unpitched
    */instrument.json   optional: {"program": 33, "bank": 0, "name": "Bass", "loop": true,
                                   "loop_start": 0.1, "loop_end": 0.5 (seconds), "release": 0.3, "root": 48, "kit": false,
                                   "normalize": true}

Sample file names give their root key: a MIDI number ("60.wav") or a note name ("C4.wav", "F#3.wav", C4 = 60).
WAVs may be 8/16/24/32-bit PCM, mono or stereo (mixed down), any sample rate.

Usage: make_sf2.py KIT_DIR OUT.sf2 [--name "Bank name"]
"""
import argparse, json, math, os, re, struct, sys, wave

NOTE_RE = re.compile(r"^([A-Ga-g])([#b]?)(-?\d+)$")
NOTE_BASE = {"c": 0, "d": 2, "e": 4, "f": 5, "g": 7, "a": 9, "b": 11}

# SoundFont generator operators (SF2.04 spec, section 8.1.3)
GEN_KEYRANGE, GEN_INSTRUMENT, GEN_SAMPLEID, GEN_SAMPLEMODES, GEN_ROOTKEY = 43, 41, 53, 54, 58
GEN_RELEASEVOLENV = 38


def parse_key(stem):
    if stem.isdigit():
        return int(stem)
    m = NOTE_RE.match(stem)
    if not m:
        return None
    letter, acc, octave = m.groups()
    key = NOTE_BASE[letter.lower()] + (1 if acc == "#" else -1 if acc == "b" else 0) + (int(octave) + 1) * 12
    return key if 0 <= key <= 127 else None


def read_wav(path):
    """Returns (list of int16 samples, sample rate)."""
    with wave.open(path, "rb") as w:
        nch, width, rate, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(n)
    out = []
    step = nch * width
    for i in range(0, len(raw) - step + 1, step):
        acc = 0
        for c in range(nch):
            b = raw[i + c * width:i + (c + 1) * width]
            if width == 1:
                v = (b[0] - 128) << 8
            elif width == 2:
                v = struct.unpack("<h", b)[0]
            elif width == 3:
                v = int.from_bytes(b, "little", signed=True) >> 8
            else:
                v = struct.unpack("<i", b)[0] >> 16
            acc += v
        out.append(max(-32768, min(32767, acc // nch)))
    return out, rate


def chunk(cid, data):
    pad = b"\0" if len(data) % 2 else b""
    return cid + struct.pack("<I", len(data)) + data + pad


def list_chunk(ltype, data):
    return chunk(b"LIST", ltype + data)


def zstr(s):
    """Zero-terminated string padded to an even length, as SF2 INFO strings must be."""
    b = s.encode("ascii", "replace") + b"\0"
    return b + b"\0" if len(b) % 2 else b


def name20(s):
    return s.encode("ascii", "replace")[:19].ljust(20, b"\0")


def load_kit(kit_dir):
    instruments = []
    for entry in sorted(os.listdir(kit_dir)):
        path = os.path.join(kit_dir, entry)
        if not os.path.isdir(path):
            continue
        cfg = {}
        cfg_path = os.path.join(path, "instrument.json")
        if os.path.exists(cfg_path):
            with open(cfg_path) as f:
                cfg = json.load(f)
        m = re.match(r"^(\d+)[_ -]?(.*)$", entry)
        program = cfg.get("program", int(m.group(1)) if m else None)
        if program is None:
            sys.exit(f"{entry}: name the folder NNN_name or set \"program\" in instrument.json")
        samples = []
        wavs = sorted(f for f in os.listdir(path) if f.lower().endswith(".wav"))
        for wav in wavs:
            stem = os.path.splitext(wav)[0]
            key = parse_key(stem)
            if key is None:
                key = cfg.get("root", 60) if len(wavs) == 1 else None
            if key is None:
                sys.exit(f"{entry}/{wav}: can't tell its root key; name it like 60.wav or C4.wav")
            data, rate = read_wav(os.path.join(path, wav))
            samples.append({"name": f"{entry[:12]}_{stem}"[:19], "key": key, "data": data, "rate": rate})
        if not samples:
            print(f"warning: {entry} has no WAVs; skipped", file=sys.stderr)
            continue
        samples.sort(key=lambda s: s["key"])
        if cfg.get("normalize", True):  # bring the loudest sample to ~90% so quiet stems aren't inaudible
            peak = max(max((abs(v) for v in s["data"]), default=0) for s in samples)
            if peak > 0:
                gain = 0.9 * 32767 / peak
                for s in samples:
                    s["data"] = [int(v * gain) for v in s["data"]]
        instruments.append({
            "name": cfg.get("name", (m.group(2) if m else entry) or entry),
            "program": program, "bank": cfg.get("bank", 0), "kit": cfg.get("kit", False),
            "loop": cfg.get("loop", False), "loop_start": cfg.get("loop_start"), "loop_end": cfg.get("loop_end"),
            "release": cfg.get("release", 0.2 if cfg.get("loop") else 0.05), "samples": samples,
        })
    return instruments


def key_ranges(samples, kit):
    """Each sample covers the keys nearest its root (or only its own key for kits)."""
    if kit:
        return [(s["key"], s["key"]) for s in samples]
    ranges = []
    for i, s in enumerate(samples):
        lo = 0 if i == 0 else (samples[i - 1]["key"] + s["key"]) // 2 + 1
        hi = 127 if i == len(samples) - 1 else (s["key"] + samples[i + 1]["key"]) // 2
        ranges.append((lo, hi))
    return ranges


def timecents(seconds):
    return int(round(1200 * math.log2(max(seconds, 0.001))))


def build_sf2(instruments, bank_name):
    smpl = bytearray()
    shdr, inst, ibag, igen, phdr, pbag, pgen = [], [], [], [], [], [], []
    sample_idx = 0
    for ii, ins in enumerate(instruments):
        inst.append(name20(ins["name"]) + struct.pack("<H", len(ibag)))
        for s, (lo, hi) in zip(ins["samples"], key_ranges(ins["samples"], ins["kit"])):
            start = len(smpl) // 2
            smpl += struct.pack(f"<{len(s['data'])}h", *s["data"])
            smpl += b"\0\0" * 46  # spec: 46 zero sample points after each sample
            end = start + len(s["data"])
            n = len(s["data"])
            # Loop points: seconds from instrument.json, else 25%..90% of the sample.
            ls = start + (int(ins["loop_start"] * s["rate"]) if ins["loop_start"] is not None else n // 4)
            le = start + (int(ins["loop_end"] * s["rate"]) if ins["loop_end"] is not None else n * 9 // 10)
            ls, le = max(start, min(ls, end - 32)), max(start + 32, min(le, end - 1))
            shdr.append(name20(s["name"]) + struct.pack("<IIIIIbbHH", start, end, ls, le, s["rate"], s["key"], 0, 0, 1))
            ibag.append(struct.pack("<HH", len(igen), 0))
            igen.append(struct.pack("<HBB", GEN_KEYRANGE, lo, hi))
            igen.append(struct.pack("<HH", GEN_ROOTKEY, s["key"]))
            igen.append(struct.pack("<HH", GEN_SAMPLEMODES, 1 if ins["loop"] else 0))
            igen.append(struct.pack("<Hh", GEN_RELEASEVOLENV, timecents(ins["release"])))
            igen.append(struct.pack("<HH", GEN_SAMPLEID, sample_idx))
            sample_idx += 1
        phdr.append(name20(ins["name"]) + struct.pack("<HHHIII", ins["program"], ins["bank"], len(pbag), 0, 0, 0))
        pbag.append(struct.pack("<HH", len(pgen), 0))
        pgen.append(struct.pack("<HH", GEN_INSTRUMENT, ii))
    # Terminal records
    phdr.append(name20("EOP") + struct.pack("<HHHIII", 0, 0, len(pbag), 0, 0, 0))
    pbag.append(struct.pack("<HH", len(pgen), 0))
    pgen.append(struct.pack("<HH", 0, 0))
    inst.append(name20("EOI") + struct.pack("<H", len(ibag)))
    ibag.append(struct.pack("<HH", len(igen), 0))
    igen.append(struct.pack("<HH", 0, 0))
    shdr.append(name20("EOS") + struct.pack("<IIIIIbbHH", 0, 0, 0, 0, 0, 0, 0, 0, 0))
    mod_terminal = b"\0" * 10
    info = chunk(b"ifil", struct.pack("<HH", 2, 1)) + chunk(b"isng", zstr("EMU8000")) + chunk(b"INAM", zstr(bank_name))
    pdta = (chunk(b"phdr", b"".join(phdr)) + chunk(b"pbag", b"".join(pbag)) + chunk(b"pmod", mod_terminal)
            + chunk(b"pgen", b"".join(pgen)) + chunk(b"inst", b"".join(inst)) + chunk(b"ibag", b"".join(ibag))
            + chunk(b"imod", mod_terminal) + chunk(b"igen", b"".join(igen)) + chunk(b"shdr", b"".join(shdr)))
    body = b"sfbk" + list_chunk(b"INFO", info) + list_chunk(b"sdta", chunk(b"smpl", bytes(smpl))) + list_chunk(b"pdta", pdta)
    return b"RIFF" + struct.pack("<I", len(body)) + body


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("kit_dir")
    ap.add_argument("out")
    ap.add_argument("--name", default="bicycle")
    a = ap.parse_args()
    instruments = load_kit(a.kit_dir)
    if not instruments:
        sys.exit("no instruments found")
    with open(a.out, "wb") as f:
        f.write(build_sf2(instruments, a.name))
    for ins in instruments:
        keys = ", ".join(str(s["key"]) for s in ins["samples"])
        print(f"program {ins['program']:3d} bank {ins['bank']}: {ins['name']} ({len(ins['samples'])} samples, keys {keys}"
              f"{', kit' if ins['kit'] else ''}{', looped' if ins['loop'] else ''})")
    print(f"wrote {a.out}")


if __name__ == "__main__":
    main()
