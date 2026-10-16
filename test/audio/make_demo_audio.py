#!/usr/bin/env python3
"""Generates the demo's audio sources: synthesized WAV samples (test/audio/kit/) and a looping theme
(test/audio/meadow.mid), standing in for what you'd export from FL Studio, LMMS or midish.
Then build the game's files:
  tools/audio/make_sf2.py test/audio/kit test/genome/music/meadow.sf2 --name meadow
  tools/audio/midi_prep.py test/audio/meadow.mid test/genome/music/meadow.mid --sf2 test/genome/music/meadow.sf2"""
import json, math, os, random, wave
import numpy as np
import mido

HERE = os.path.dirname(os.path.abspath(__file__))
RATE = 22050
random.seed(1)


def write(path, sig):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    sig = np.clip(sig / max(1e-9, np.max(np.abs(sig))) * 0.8, -1, 1)
    with wave.open(path, "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes((sig * 32767).astype("<i2").tobytes())


def hz(key):
    return 440.0 * 2 ** ((key - 69) / 12)


def t(sec):
    return np.arange(int(RATE * sec)) / RATE


def pluck(key, sec=1.2):  # Karplus-Strong string
    n = int(RATE / hz(key)); buf = np.random.uniform(-1, 1, n); out = np.zeros(int(RATE * sec))
    for i in range(len(out)):
        out[i] = buf[i % n]; buf[i % n] = 0.5 * (buf[i % n] + buf[(i + 1) % n]) * 0.996
    return out


def saw(key, sec):
    x = t(sec); f = hz(key)
    return sum(np.sin(2 * np.pi * f * k * x) / k for k in range(1, 12))


kit = os.path.join(HERE, "kit")
np.random.seed(1)
for name, key in (("C3", 48), ("C4", 60), ("C5", 72)):          # multi-sampled lead
    write(f"{kit}/000_lead/{name}.wav", pluck(key))
bass = saw(36, 1.0) * np.minimum(1, t(1.0) * 200)
write(f"{kit}/033_bass/C2.wav", bass)
with open(f"{kit}/033_bass/instrument.json", "w") as f:
    json.dump({"name": "Saw Bass", "loop": True, "loop_start": 0.2, "loop_end": 0.8, "release": 0.15}, f, indent=2)
pad = sum(np.sin(2 * np.pi * hz(60) * r * t(2.0)) for r in (1, 1.003, 2.0, 0.997)) * np.minimum(1, t(2.0) * 4)
write(f"{kit}/048_pad/C4.wav", pad)
with open(f"{kit}/048_pad/instrument.json", "w") as f:
    json.dump({"name": "Soft Pad", "loop": True, "loop_start": 0.5, "loop_end": 1.8, "release": 0.6}, f, indent=2)
# Sound effects: one sound per key, unpitched.
x = t(0.5)
coin = (np.sin(2 * np.pi * 988 * x) * (x < 0.08) + np.sin(2 * np.pi * 1319 * x) * (x >= 0.08)) * np.exp(-x * 6)
write(f"{kit}/120_sfx/72.wav", coin)
x = t(0.35)
bump = (np.sin(2 * np.pi * (120 - 160 * x) * x) + 0.3 * np.random.uniform(-1, 1, len(x))) * np.exp(-x * 14)
write(f"{kit}/120_sfx/60.wav", bump)
x = t(0.8)
neigh = np.sin(2 * np.pi * (600 + 250 * np.sin(2 * np.pi * 9 * x)) * x) * np.exp(-x * 3) * np.minimum(1, x * 30)
write(f"{kit}/120_sfx/64.wav", neigh)
x = t(0.4)
hurt = np.sign(np.sin(2 * np.pi * (330 - 400 * x) * x)) * 0.6 * np.exp(-x * 7)       # descending square "oof"
write(f"{kit}/120_sfx/55.wav", hurt)
x = t(0.07)
blip = np.sin(2 * np.pi * 1760 * x) * np.exp(-x * 30)                                  # dialogue page blip
write(f"{kit}/120_sfx/76.wav", blip)
with open(f"{kit}/120_sfx/instrument.json", "w") as f:
    json.dump({"name": "Game SFX", "kit": True}, f, indent=2)

# Theme: 8 bars, 100 bpm. Lead on channel 0 (program 0), bass on 1 (program 33), pad on 2 (program 48).
mid = mido.MidiFile(ticks_per_beat=480)
B = 480
chords = [(60, 64, 67), (57, 60, 64), (53, 57, 60), (55, 59, 62)] * 2
melody = [72, 71, 67, 69, 72, 74, 76, 74, 72, 69, 65, 67, 69, 71, 72, 67]
for ch, prog in ((0, 0), (1, 33), (2, 48)):
    tr = mido.MidiTrack(); mid.tracks.append(tr)
    if ch == 0:
        tr.append(mido.MetaMessage("set_tempo", tempo=mido.bpm2tempo(100)))
    tr.append(mido.Message("program_change", channel=ch, program=prog))
    events = []
    for bar, chord in enumerate(chords):
        start = bar * 4 * B
        if ch == 0:
            for i in range(2):
                events.append((start + i * 2 * B, melody[(bar * 2 + i) % len(melody)], 2 * B - 40, 100))
        elif ch == 1:
            for beat in range(4):
                events.append((start + beat * B, chord[0] - 24, B - 60, 90))
        else:
            for k in chord:
                events.append((start, k, 4 * B - 20, 60))
    msgs = []
    for s, k, d, v in events:
        msgs.append((s, mido.Message("note_on", channel=ch, note=k, velocity=v)))
        msgs.append((s + d, mido.Message("note_off", channel=ch, note=k, velocity=0)))
    msgs.sort(key=lambda m: (m[0], m[1].type == "note_on"))
    now = 0
    for at, m in msgs:
        tr.append(m.copy(time=at - now)); now = at
mid.save(os.path.join(HERE, "meadow.mid"))
print("wrote kit/ and meadow.mid")
