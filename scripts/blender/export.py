# Exports a .blend for bicycle's 3D mode, headless (from the repo root):
#   blender -b plat3d/blender/plat3d.blend --python scripts/blender/export.py [-- <game>/genome]
# The work is done by the bicycle add-on (tools/blender/bicycle_blender.py, the same code as its Export button):
# every Blender scene is a level, genome/level/<level>.lua (its meshes, the genomes placed in it with their
# per-instance parameters, its spawn points); level meshes go to genome/model/, genome models (bicycle_role
# "character") to genome/body/model/ (.obj + .mtl, .bimg textures, .bskin for rigged meshes). The genome folder
# defaults to <game>/genome for a .blend in <game>/blender/ (or the scene's setting in the Bicycle panel).
import os
import sys

import bpy

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "blender"))
import bicycle_blender  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
gdir = os.path.abspath(argv[0]) if argv else None
if gdir and os.path.basename(gdir) in ("model", "models"):  # old invocations passed <game>/genome/model
    gdir = os.path.dirname(gdir)
if not hasattr(bpy.types.Scene, "bicycle"):
    bicycle_blender.register()
paths = bicycle_blender.export_all(gdir)
print("bicycle export: wrote %s" % ", ".join(paths))
