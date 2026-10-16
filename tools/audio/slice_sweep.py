#!/usr/bin/env python3
"""Cuts an audio render of note_sweep.py's MIDI into one WAV per note (named by MIDI key) for make_sf2.py.

The render must start at the sweep's first note (trim leading silence is automatic: the first sound found
is taken as the start). Use the same --low/--high/--step/--note/--gap you gave note_sweep.py.

Usage: slice_sweep.py STEM.wav OUT_DIR [--low 36] [--high 84] [--step 12] [--note 2.0] [--gap 1.0]
"""
import argparse, os, wave
import numpy as np
from note_sweep import sweep_notes


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("stem"); ap.add_argument("out_dir")
    ap.add_argument("--low", type=int, default=36); ap.add_argument("--high", type=int, default=84)
    ap.add_argument("--step", type=int, default=12)
    ap.add_argument("--note", type=float, default=2.0); ap.add_argument("--gap", type=float, default=1.0)
    a = ap.parse_args()
    with wave.open(a.stem, "rb") as w:
        rate, nch, width = w.getframerate(), w.getnchannels(), w.getsampwidth()
        raw = w.readframes(w.getnframes())
    if width != 2:
        raise SystemExit("export the stem as 16-bit WAV")
    x = np.frombuffer(raw, dtype="<i2").reshape(-1, nch).mean(axis=1)
    loud = np.nonzero(np.abs(x) > 0.01 * 32768)[0]
    if len(loud) == 0:
        raise SystemExit(f"{a.stem} is silent")
    start = loud[0]
    period = int((a.note + a.gap) * rate)
    os.makedirs(a.out_dir, exist_ok=True)
    for i, key in enumerate(sweep_notes(a.low, a.high, a.step)):
        seg = x[start + i * period: start + (i + 1) * period]
        if len(seg) == 0:
            print(f"stem ends before key {key}; stopping"); break
        nz = np.nonzero(np.abs(seg) > 0.001 * 32768)[0]  # trim the silent tail
        seg = seg[: (nz[-1] + 1) if len(nz) else len(seg)]
        path = os.path.join(a.out_dir, f"{key}.wav")
        with wave.open(path, "wb") as w:
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
            w.writeframes(seg.astype("<i2").tobytes())
        print(f"key {key}: {len(seg) / rate:.2f}s -> {path}")


if __name__ == "__main__":
    main()
