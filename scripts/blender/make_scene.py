# Authors test/blender/meadow3d.blend, the 3D mode's test scene: a pilgrim-like character, a textured ground
# plane and a crate prop. Run headless (from the repo root):
#   blender -b --factory-startup --python scripts/blender/make_scene.py
# Then export it for the engine with scripts/blender/export.py (see scripts/blender/README.md).
import bpy
import math
import os

OUT = os.path.join(os.getcwd(), "test", "blender", "meadow3d.blend")

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene


def material(name, rgba, image=None):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = rgba
    mat.diffuse_color = rgba
    if image is not None:
        tex = mat.node_tree.nodes.new("ShaderNodeTexImage")
        tex.image = image
        tex.interpolation = "Closest"
        mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def pixel_image(name, w, h, fn):
    """An image from fn(x, y) -> (r, g, b) in 0..1, packed into the .blend."""
    img = bpy.data.images.new(name, w, h, alpha=True)
    px = []
    for y in range(h):
        for x in range(w):
            r, g, b = fn(x, y)
            px += [r, g, b, 1.0]
    img.pixels = px
    img.pack()
    return img


def add(obj_op, name, mat, **kw):
    obj_op(**kw)
    obj = bpy.context.active_object
    obj.name = name
    obj.data.name = name
    obj.data.materials.append(mat)
    return obj


# --- character: robe, legs, arms, head, hat and nose, joined into one mesh, rigged and animated ---------------------
# Each part is bound to one bone (the robe blends from hips to spine), so the actions below move it rigidly.
robe = material("robe", (0.42, 0.42, 0.55, 1.0))
skin = material("skin", (0.90, 0.72, 0.58, 1.0))
hat = material("hat", (0.08, 0.08, 0.08, 1.0))
boots = material("boots", (0.25, 0.16, 0.10, 1.0))


def part(obj, bone):
    """Bind every vertex of a part to one bone (the robe gets a hips/spine blend instead, below)."""
    if bone:
        g = obj.vertex_groups.new(name=bone)
        g.add(range(len(obj.data.vertices)), 1.0, "REPLACE")
    return obj


robe_obj = add(bpy.ops.mesh.primitive_cone_add, "robe", robe, vertices=12, radius1=0.42, radius2=0.22, depth=0.62,
               location=(0, 0, 0.71))
hips_g = robe_obj.vertex_groups.new(name="hips")
spine_g = robe_obj.vertex_groups.new(name="spine")
for v in robe_obj.data.vertices:
    z = robe_obj.matrix_world @ v.co
    w = max(0.0, min(1.0, (0.95 - z.z) / 0.5))
    hips_g.add([v.index], w, "REPLACE")
    spine_g.add([v.index], 1.0 - w, "REPLACE")
parts = [
    robe_obj,
    part(add(bpy.ops.mesh.primitive_cylinder_add, "leg_L", boots, vertices=8, radius=0.075, depth=0.46, location=(0, 0.13, 0.23)), "leg_L"),
    part(add(bpy.ops.mesh.primitive_cylinder_add, "leg_R", boots, vertices=8, radius=0.075, depth=0.46, location=(0, -0.13, 0.23)), "leg_R"),
    part(add(bpy.ops.mesh.primitive_cylinder_add, "arm_L", robe, vertices=8, radius=0.06, depth=0.42, location=(0, 0.3, 0.72)), "arm_L"),
    part(add(bpy.ops.mesh.primitive_cylinder_add, "arm_R", robe, vertices=8, radius=0.06, depth=0.42, location=(0, -0.3, 0.72)), "arm_R"),
    part(add(bpy.ops.mesh.primitive_uv_sphere_add, "head", skin, segments=12, ring_count=8, radius=0.22, location=(0, 0, 1.2)), "head"),
    part(add(bpy.ops.mesh.primitive_cylinder_add, "brim", hat, vertices=12, radius=0.36, depth=0.04, location=(0, 0, 1.36)), "head"),
    part(add(bpy.ops.mesh.primitive_cylinder_add, "crown", hat, vertices=12, radius=0.2, depth=0.22, location=(0, 0, 1.48)), "head"),
    # a nose so you can see which way he faces (+X, the direction the 2D sprite walks when moving right)
    part(add(bpy.ops.mesh.primitive_cone_add, "nose", skin, vertices=6, radius1=0.05, radius2=0.0, depth=0.12,
             location=(0.24, 0, 1.2), rotation=(0, math.pi / 2, 0)), "head"),
]
bpy.ops.object.select_all(action="DESELECT")
for p in parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()
pilgrim = bpy.context.active_object
pilgrim.name = "pilgrim3d"
pilgrim.data.name = "pilgrim3d"
pilgrim["bicycle_role"] = "character"  # export.py: placed by the game (model gene), not by the scene
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# Armature (Z up, facing +X): hips -> spine -> head, hips -> legs, spine -> arms.
bpy.ops.object.armature_add(enter_editmode=True, location=(0, 0, 0))
rig = bpy.context.active_object
rig.name = "pilgrim3d_rig"
rig.data.name = "pilgrim3d_rig"
eb = rig.data.edit_bones
eb.remove(eb[0])
BONES = [  # name, head, tail, parent
    ("hips", (0, 0, 0.46), (0, 0, 0.62), None),
    ("spine", (0, 0, 0.62), (0, 0, 1.0), "hips"),
    ("head", (0, 0, 1.0), (0, 0, 1.4), "spine"),
    ("leg_L", (0, 0.13, 0.46), (0, 0.13, 0.0), "hips"),
    ("leg_R", (0, -0.13, 0.46), (0, -0.13, 0.0), "hips"),
    ("arm_L", (0, 0.3, 0.93), (0, 0.3, 0.5), "spine"),
    ("arm_R", (0, -0.3, 0.93), (0, -0.3, 0.5), "spine"),
]
for name, head, tail, parent in BONES:
    b = eb.new(name)
    b.head, b.tail = head, tail
    if parent:
        b.parent = eb[parent]
bpy.ops.object.mode_set(mode="OBJECT")
pilgrim.parent = rig
mod = pilgrim.modifiers.new("rig", "ARMATURE")
mod.object = rig

# Actions: rotations are given about armature (world) axes and converted to each bone's local space.
from mathutils import Matrix, Quaternion, Vector  # noqa: E402

def key_pose(action_name, frames, fn):
    """fn(frame) -> {bone: (axis, degrees, lift)}; keys every frame 0..frames (the last equals the first)."""
    rig.animation_data_create()
    act = bpy.data.actions.new(action_name)
    act.use_fake_user = True
    rig.animation_data.action = act
    for f in range(frames + 1):
        pose = fn(f % frames)
        for pb in rig.pose.bones:
            pb.rotation_mode = "QUATERNION"
            axis, deg, lift = pose.get(pb.name, ((0, 0, 1), 0.0, 0.0))
            rest = pb.bone.matrix_local.to_3x3()
            world = Matrix.Rotation(math.radians(deg), 3, Vector(axis))
            pb.rotation_quaternion = (rest.inverted() @ world @ rest).to_quaternion()
            pb.location = rest.inverted() @ Vector((0, 0, lift))
            pb.keyframe_insert("rotation_quaternion", frame=f + 1)
            pb.keyframe_insert("location", frame=f + 1)
    act["bicycle_frames"] = frames
    return act

def walk(f):
    s = math.sin(2 * math.pi * f / 24)
    return {
        "leg_L": ((0, 1, 0), 32 * s, 0.0), "leg_R": ((0, 1, 0), -32 * s, 0.0),
        "arm_L": ((0, 1, 0), -28 * s, 0.0), "arm_R": ((0, 1, 0), 28 * s, 0.0),
        "hips": ((0, 0, 1), 0.0, 0.035 * abs(math.sin(2 * math.pi * f / 24))),
        "spine": ((0, 0, 1), 6 * s, 0.0), "head": ((0, 1, 0), 4.0, 0.0),
    }

def idle(f):
    s = math.sin(2 * math.pi * f / 48)
    return {
        "spine": ((1, 0, 0), 2.5 * s, 0.0), "head": ((0, 1, 0), -8 + 5 * s, 0.0),
        "arm_L": ((1, 0, 0), -10 + 3 * s, 0.0), "arm_R": ((1, 0, 0), 10 - 3 * s, 0.0),
        "hips": ((0, 0, 1), 0.0, 0.008 * s),
    }

key_pose("walk", 24, walk)
key_pose("idle", 48, idle)
rig.animation_data.action = bpy.data.actions["idle"]

# --- ground: a 60 x 10 plane with a tiled grass texture -----------------------------------------------------------
def grass(x, y):
    v = 0.30 + 0.12 * (((x * 7 + y * 13) % 5) / 4.0)
    return (0.18 * v / 0.3, v, 0.12 * v / 0.3)

grass_img = pixel_image("grass", 16, 16, grass)
bpy.ops.mesh.primitive_grid_add(x_subdivisions=30, y_subdivisions=5, size=1, location=(0, 0, 0))
ground = bpy.context.active_object
ground.name = "ground"
ground.data.name = "ground"
ground.scale = (60, 10, 1)
bpy.ops.object.transform_apply(scale=True)
ground.data.materials.append(material("grass", (1, 1, 1, 1), grass_img))
# tile the texture once per 2 x 2 units
uv = ground.data.uv_layers.active.data
for loop in ground.data.loops:
    co = ground.data.vertices[loop.vertex_index].co
    uv[loop.index].uv = (co.x / 2.0, co.y / 2.0)

# --- prop: a crate with a plank texture ---------------------------------------------------------------------------
def planks(x, y):
    edge = x in (0, 15) or y in (0, 15) or y % 5 == 0
    return (0.30, 0.18, 0.08) if edge else (0.62, 0.42, 0.20)

crate_img = pixel_image("planks", 16, 16, planks)
crate = add(bpy.ops.mesh.primitive_cube_add, "crate", material("crate", (1, 1, 1, 1), crate_img), size=0.8, location=(4, -0.8, 0.4))
crate.rotation_euler = (0, 0, math.radians(20))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=OUT, compress=True)
print("wrote", OUT)
