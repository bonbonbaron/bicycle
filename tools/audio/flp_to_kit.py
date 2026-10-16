#!/usr/bin/env python3
"""Turns an FL Studio project's channel rack into a make_sf2.py kit (uses PyFLP to read the .flp).

For each channel, in rack order, program = its position (0, 1, 2, ...):
  * Sampler / audio clip channels: copies the sample into kit/NNN_name/ with its root note
    (FL's keyboard root; C5 in FL = MIDI 60). FL keeps absolute sample paths, so if you moved the project
    from Windows/macOS, pass --search DIR to find the files by name.
  * Plugin channels (3x Osc, Sytrus, VSTs, ...): their sound can't be rendered outside FL Studio. The script
    makes kit/NNN_name/ with RENDER_ME.txt and a note_sweep.mid; render that in FL and run slice_sweep.py
    (steps in tools/audio/README.md). Until then, General MIDI program NNN stands in.
It also writes kit/programs.json (channel name -> program) for midi_prep.py --map-json.

PyFLP 2.2 needs Python 3.10 or older; flp_to_kit.sh runs this with one via uv.

Usage: flp_to_kit.py PROJECT.flp KIT_DIR [--search DIR ...]
"""
import argparse, json, os, re, shutil, sys
import pyflp
from pyflp.channel import Instrument, Sampler


def safe(name):
    return re.sub(r"[^A-Za-z0-9_-]+", "_", name).strip("_")[:24] or "channel"


def find_sample(path, search_dirs):
    if path and os.path.exists(path):
        return path
    base = os.path.basename(str(path).replace("\\", "/")) if path else None
    for d in search_dirs:
        for root, _, files in os.walk(d):
            if base in files:
                return os.path.join(root, base)
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("flp"); ap.add_argument("kit_dir")
    ap.add_argument("--search", action="append", default=[], help="folder to look for samples in")
    a = ap.parse_args()
    project = pyflp.parse(a.flp)
    os.makedirs(a.kit_dir, exist_ok=True)
    programs, todo = {}, []
    program = -1
    for ch in project.channels:
        if type(ch).__name__ in ("Automation", "Layer"):
            print(f"skipping {type(ch).__name__.lower()} channel '{ch.display_name or ch.name}' (no sound of its own)")
            continue
        program += 1
        if program > 127:
            sys.exit("more than 128 sound channels; split the project")
        name = ch.display_name or ch.name or ch.internal_name or f"channel{program}"
        folder = os.path.join(a.kit_dir, f"{program:03d}_{safe(name)}")
        programs[name] = program
        os.makedirs(folder, exist_ok=True)
        root = getattr(getattr(ch, "keyboard", None), "root_note", None)
        cfg = {"program": program, "name": name[:19]}
        if root is not None:
            cfg["root"] = int(root)
        if isinstance(ch, Sampler):
            src = find_sample(ch.sample_path, a.search)
            if src:
                ext = os.path.splitext(src)[1].lower()
                dst = os.path.join(folder, f"{cfg.get('root', 60)}.wav")
                if ext == ".wav":
                    shutil.copy(src, dst)
                    print(f"program {program}: sampler '{name}' <- {src}")
                else:
                    todo.append((program, name, f"convert {src} to WAV as {dst}"))
            else:
                why = f"sample not found ({ch.sample_path})" if ch.sample_path else "no sample loaded"
                todo.append((program, name, f"{why}; copy a WAV here as <root>.wav or pass --search"))
        elif isinstance(ch, Instrument):
            plugin = ch.internal_name or (type(ch.plugin).__name__ if ch.plugin is not None else "an unknown generator")
            sweep = os.path.join(folder, "note_sweep.mid")
            os.system(f'"{sys.executable}" "{os.path.join(os.path.dirname(__file__), "note_sweep.py")}" "{sweep}" > /dev/null')
            with open(os.path.join(folder, "RENDER_ME.txt"), "w") as f:
                f.write(f"'{name}' uses the plugin {plugin}, which only runs inside FL Studio.\n"
                        f"1. Drag note_sweep.mid onto this channel in FL Studio (or paste it into its piano roll).\n"
                        f"2. Solo the channel and File > Export > Wave file (16-bit), e.g. stem.wav.\n"
                        f"3. tools/audio/slice_sweep.py stem.wav {folder}\n")
            todo.append((program, name, f"plugin {plugin}: render note_sweep.mid in FL (see RENDER_ME.txt)"))
        else:
            todo.append((program, name, f"{type(ch).__name__} channel: export its sound as WAV into this folder"))
        with open(os.path.join(folder, "instrument.json"), "w") as f:
            json.dump(cfg, f, indent=2)
    with open(os.path.join(a.kit_dir, "programs.json"), "w") as f:
        json.dump(programs, f, indent=2)
    print(f"wrote {a.kit_dir}/programs.json ({len(programs)} channels)")
    for program, name, what in todo:
        print(f"TODO program {program} '{name}': {what}")


if __name__ == "__main__":
    main()
