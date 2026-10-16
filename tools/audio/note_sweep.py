#!/usr/bin/env python3
"""Writes a MIDI "note sweep" for sampling synths you can't run on the box (VSTs in FL Studio, LMMS synths).

Load it into each synth channel, export that channel's audio (FL: File > Export > Wave, with "Split mixer
tracks" or the channel soloed; LMMS: File > Export, or `lmms render`), then cut it into per-note samples:
  slice_sweep.py stem.wav kit/NNN_name/   (same --low/--high/--step/--note/--gap as here)

Usage: note_sweep.py OUT.mid [--low 36] [--high 84] [--step 12] [--note 2.0] [--gap 1.0] [--vel 100]
"""
import argparse
import mido


def sweep_notes(low, high, step):
    return list(range(low, high + 1, step))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out")
    ap.add_argument("--low", type=int, default=36); ap.add_argument("--high", type=int, default=84)
    ap.add_argument("--step", type=int, default=12)
    ap.add_argument("--note", type=float, default=2.0, help="seconds each note is held")
    ap.add_argument("--gap", type=float, default=1.0, help="seconds of silence after each (release tail)")
    ap.add_argument("--vel", type=int, default=100)
    a = ap.parse_args()
    mid = mido.MidiFile(ticks_per_beat=480)
    tr = mido.MidiTrack(); mid.tracks.append(tr)
    tr.append(mido.MetaMessage("set_tempo", tempo=500000))  # 120 bpm: 960 ticks per second
    tps = 960
    for i, key in enumerate(sweep_notes(a.low, a.high, a.step)):
        tr.append(mido.Message("note_on", note=key, velocity=a.vel, time=0 if i == 0 else int(a.gap * tps)))
        tr.append(mido.Message("note_off", note=key, velocity=0, time=int(a.note * tps)))
    tr.append(mido.MetaMessage("end_of_track", time=int(a.gap * tps)))
    mid.save(a.out)
    print(f"wrote {a.out}: keys {sweep_notes(a.low, a.high, a.step)}, {a.note}s on + {a.gap}s gap each")


if __name__ == "__main__":
    main()
