# The Blender add-on, headless: installs and enables tools/blender/bicycle_blender.py like a user would, then on a
# copy of plat3d.blend uses its operators and drop-downs (genome list, door.to scenes, door.spawn spawn points),
# adds a path point and exports, and checks the level file. Run (exit code 0 = OK):
#   blender -b plat3d/blender/plat3d.blend --factory-startup --python tests/blender_addon_test.py
import os
import shutil
import sys
import tempfile

import addon_utils
import bpy

root = os.getcwd()
fail = []


def check(cond, what):
    print(("ok   " if cond else "FAIL ") + what)
    if not cond:
        fail.append(what)


bpy.ops.preferences.addon_install(filepath=os.path.join(root, "tools", "blender", "bicycle_blender.py"), overwrite=True)
addon_utils.enable("bicycle_blender", default_set=True)
import bicycle_blender as B  # noqa: E402

# Work on a copy of the game so the test doesn't touch plat3d/.
tmp = tempfile.mkdtemp()
game = os.path.join(tmp, "plat3d")
shutil.copytree(os.path.join(root, "plat3d", "genome"), os.path.join(game, "genome"))
os.makedirs(os.path.join(game, "blender"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(game, "blender", "plat3d.blend"))
scene = bpy.data.scenes["courtyard"]
bpy.context.window.scene = scene if bpy.context.window else None
check(B.genome_dir(scene) == os.path.join(game, "genome"), "genome folder found next to the .blend")
bpy.ops.bicycle.refresh()
names = [i[0] for i in B.genome_items(scene.bicycle, bpy.context)]
check(names == ["coin", "flag", "hero", "mover"], "genome drop-down: %s" % names)

scene.bicycle.add_genome_choice = "flag"
scene.cursor.location = (1, 2, 3)
with bpy.context.temp_override(scene=scene):
    bpy.ops.bicycle.add_entity()
flag = bpy.context.view_layer.objects.active if bpy.context.view_layer.objects.active and bpy.context.view_layer.objects.active.bicycle.genome == "flag" else \
    max((o for o in scene.objects if o.bicycle.genome == "flag"), key=lambda o: o.name)
check(flag.bicycle.kind == "ENTITY" and tuple(flag.location) == (1, 2, 3), "Add entity puts a flag at the cursor")
check(flag.instance_collection is not None, "the flag shows its model (preview)")
params = {p.name: p for p in flag.bicycle.params}
check(sorted(params) == ["door.on", "door.spawn", "door.to"], "flag params: %s" % sorted(params))
scenes = [i[0] for i in B.param_items(params["door.to"], None)]
check(scenes == ["", "bonus", "courtyard"], "door.to drop-down: %s" % scenes)
params["door.to"].choice = "bonus"
spawns = [i[0] for i in B.param_items(params["door.spawn"], None)]
check(spawns == ["default", "entry"], "door.spawn drop-down follows door.to (bonus): %s" % spawns)
params["door.spawn"].choice = "entry"
params["door.to"].choice = "courtyard"
spawns = [i[0] for i in B.param_items(params["door.spawn"], None)]
check(spawns == ["default", "from_bonus"], "door.spawn for courtyard: %s" % spawns)
params["door.to"].choice = "bonus"

mover = bpy.data.objects["mover"]
with bpy.context.temp_override(scene=scene, active_object=mover, object=mover):
    bpy.ops.bicycle.add_path_point()
check(len(B.path_points(mover)) == 2, "Add path point adds a second point")
B.path_points(mover)[1].location.z += 0  # 1 m above the last one
with bpy.context.temp_override(scene=scene):
    scene.bicycle.spawn_name = "secret"
    bpy.ops.bicycle.add_spawn()


class Layout:  # stands in for the UI (no window in -b): records what the panel draws
    drawn = []

    def __getattr__(self, name):
        def call(*args, **kw):
            Layout.drawn.append((name, args, kw))
            return Layout()
        return call


class Ctx:
    def __init__(self, scene, obj):
        self.scene, self.active_object = scene, obj


for obj in (flag, mover, B.path_points(mover)[1], bpy.data.objects["ground"], None):
    panel = type("Panel", (), {"layout": Layout()})()
    B.BICYCLE_PT_panel.draw(panel, Ctx(scene, obj))
props = [a[1] for n, a, k in Layout.drawn if n == "prop"]
check(all(p in props for p in ("choice", "add_genome_choice", "genome_choice", "number", "flag")), "the panel draws (drop-downs: %s)" % sorted(set(props)))
bpy.ops.bicycle.export(all_scenes=True)
lvl = B.read_file(os.path.join(game, "genome", "level", "courtyard.lua"))
ents = {e["key"]: e for e in B.as_list(lvl["entities"])}
check(ents[flag.name]["door"] == {"on": "touch", "spawn": "entry", "to": "bonus"} and ents[flag.name]["pos"] == [1, 3, -2],
      "exported flag: %s" % ents[flag.name])
check(ents["mover"]["motion"]["path"] == [[0, 0, 0], [0, 3.25, 0], [0, 4.25, 0]], "exported mover path: %s" % ents["mover"]["motion"])
check("secret" in lvl["spawns"], "exported spawn point")
check(os.path.exists(os.path.join(game, "genome", "body", "model", "coin3d.obj")), "genome models -> body/model")
shutil.rmtree(tmp)
installed = B.__file__
addon_utils.disable("bicycle_blender")
if os.path.basename(os.path.dirname(installed)) == "addons" and os.path.dirname(os.path.abspath(installed)) != os.path.join(root, "tools", "blender"):
    os.remove(installed)  # the copy addon_install put in Blender's user add-ons folder
    shutil.rmtree(os.path.join(os.path.dirname(installed), "__pycache__"), ignore_errors=True)
print("blender-addon: %s" % ("OK" if not fail else "%d failed" % len(fail)))
sys.exit(1 if fail else 0)
