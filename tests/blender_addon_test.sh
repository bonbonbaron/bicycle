#!/bin/sh
# The Blender add-on (tools/blender) headless; skipped unless BICYCLE_BLENDER points at a blender binary.
#   tests/blender_addon_test.sh <repo root>   (ctest: blender-addon)
cd "$1" || exit 1
[ -n "${BICYCLE_BLENDER:-}" ] && [ -x "$BICYCLE_BLENDER" ] || { echo "blender-addon: set BICYCLE_BLENDER to run it; skipping"; exit 77; }
out=$("$BICYCLE_BLENDER" -b plat3d/blender/plat3d.blend --factory-startup --python-exit-code 1 --python tests/blender_addon_test.py 2>&1)
rc=$?
echo "$out" | grep -a '^ok\|^FAIL\|blender-addon\|Error\|Traceback'
exit $rc
