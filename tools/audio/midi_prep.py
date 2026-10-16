#!/usr/bin/env python3
"""Get a .mid from FL Studio, LMMS or midish ready for bicycle.

  * Checks that every program the song uses exists in the game's SoundFont (else General MIDI plays it).
  * Keeps the song off the engine's sound-effect channel (15) by moving it to a free channel.
  * Optionally assigns programs by track name (FL Studio and LMMS name MIDI tracks after their channels/tracks):
      --map "Lead=0" --map "Bass=33"     or     --map-json kit/programs.json   (written by flp_to_kit.py)

Usage: midi_prep.py IN.mid OUT.mid [--sf2 game.sf2] [--map NAME=PROGRAM ...] [--map-json FILE]
"""
import argparse, json, struct, sys
import mido

SFX_CHANNEL = 15
DRUM_CHANNEL = 9


def sf2_presets(path):
    """Returns {(bank, program): name} from a .sf2's phdr chunk."""
    data = open(path, "rb").read()
    i = data.find(b"phdr")
    if i < 0:
        sys.exit(f"{path}: no preset headers; not a SoundFont?")
    size = struct.unpack("<I", data[i + 4:i + 8])[0]
    presets = {}
    for off in range(i + 8, i + 8 + size - 38, 38):  # last record is the EOP terminator
        name = data[off:off + 20].split(b"\0")[0].decode("ascii", "replace")
        program, bank = struct.unpack("<HH", data[off + 20:off + 24])
        presets[(bank, program)] = name
    return presets


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("inp"); ap.add_argument("out")
    ap.add_argument("--sf2")
    ap.add_argument("--map", action="append", default=[], help="TRACKNAME=PROGRAM")
    ap.add_argument("--map-json")
    a = ap.parse_args()
    name_to_prog = {}
    if a.map_json:
        name_to_prog.update({k.lower(): v for k, v in json.load(open(a.map_json)).items()})
    for m in a.map:
        k, v = m.rsplit("=", 1)
        name_to_prog[k.strip().lower()] = int(v)

    mid = mido.MidiFile(a.inp)
    used_channels = {msg.channel for t in mid.tracks for msg in t if hasattr(msg, "channel")}
    free = [c for c in range(16) if c not in used_channels and c not in (SFX_CHANNEL, DRUM_CHANNEL)]
    remap = {}
    if SFX_CHANNEL in used_channels:
        if not free:
            sys.exit("song uses all 16 channels; free one up for sound effects (channel 16 in 1-based tools)")
        remap[SFX_CHANNEL] = free.pop(0)
        print(f"moved channel {SFX_CHANNEL} to {remap[SFX_CHANNEL]} (15 is reserved for sound effects)")

    programs = set()
    for ti, track in enumerate(mid.tracks):
        tname = next((m.name for m in track if m.type == "track_name"), "").strip()
        forced = name_to_prog.get(tname.lower())
        chans = set()
        for msg in track:
            if hasattr(msg, "channel"):
                if msg.channel in remap:
                    msg.channel = remap[msg.channel]
                chans.add(msg.channel)
                if msg.type == "program_change":
                    if forced is not None:
                        msg.program = forced
                    programs.add((0 if msg.channel != DRUM_CHANNEL else 128, msg.program))
        if forced is not None and chans and not any(m.type == "program_change" for m in track):
            for ch in sorted(chans):
                track.insert(0, mido.Message("program_change", channel=ch, program=forced, time=0))
                programs.add((0, forced))
        if forced is not None:
            print(f"track {ti} '{tname}' -> program {forced}")
    if a.sf2:
        presets = sf2_presets(a.sf2)
        for bank, prog in sorted(programs):
            name = presets.get((bank, prog))
            print(f"program {prog:3d} bank {bank}: " + (f"'{name}' from {a.sf2}" if name else "NOT in the SoundFont; General MIDI will play it"))
    mid.save(a.out)
    print(f"wrote {a.out} ({mid.length:.1f}s, channels {sorted({msg.channel for t in mid.tracks for msg in t if hasattr(msg, 'channel')})})")


if __name__ == "__main__":
    main()
