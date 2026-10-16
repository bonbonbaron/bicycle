#!/bin/sh
# End-to-end audio pipeline check (needs python3-mido, python3-numpy, fluidsynth, midish):
# synth samples + theme -> midish round trip -> midi_prep -> note sweep render/slice -> make_sf2 -> fluidsynth render.
set -e
here=$(cd "$(dirname "$0")" && pwd); repo=$(cd "$here/../.." && pwd); tmp=$(mktemp -d)
python3 "$repo/test/audio/make_demo_audio.py" > /dev/null
printf 'import "%s"\nexport "%s"\n' "$repo/test/audio/meadow.mid" "$tmp/midish.mid" | midish -b
python3 "$here/note_sweep.py" "$tmp/sweep.mid" > /dev/null
fluidsynth -ni -F "$tmp/stem.wav" -r 44100 /usr/share/sounds/sf2/TimGM6mb.sf2 "$tmp/sweep.mid" > /dev/null 2>&1
mkdir -p "$tmp/kit" && cp -r "$repo/test/audio/kit/"* "$tmp/kit/"
python3 "$here/slice_sweep.py" "$tmp/stem.wav" "$tmp/kit/001_swept_piano" > /dev/null
python3 "$here/make_sf2.py" "$tmp/kit" "$tmp/game.sf2"
python3 "$here/midi_prep.py" "$tmp/midish.mid" "$tmp/song.mid" --sf2 "$tmp/game.sf2"
fluidsynth -ni -F "$tmp/song.wav" -r 44100 "$tmp/game.sf2" "$tmp/song.mid" > /dev/null 2>&1
python3 - "$tmp/song.wav" <<'PY'
import sys, wave, numpy as np
w = wave.open(sys.argv[1]); x = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2") / 32768
rms = float(np.sqrt(np.mean(x ** 2)))
print(f"rendered {w.getnframes() / w.getframerate():.1f}s, RMS {rms:.4f}")
sys.exit(0 if rms > 0.005 else "FAIL: render is silent")
PY
echo "AUDIO PIPELINE OK"
rm -rf "$tmp"
