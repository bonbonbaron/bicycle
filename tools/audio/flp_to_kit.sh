#!/bin/sh
# Runs flp_to_kit.py under Python 3.10 (PyFLP 2.2 doesn't work on newer Pythons) using uv:
#   pip install uv   (once)
#   tools/audio/flp_to_kit.sh PROJECT.flp KIT_DIR [--search DIR ...]
here=$(dirname "$0")
exec uv run -q --python 3.10 --with pyflp==2.2.1 --with mido python "$here/flp_to_kit.py" "$@"
