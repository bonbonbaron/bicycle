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


# --- character: robe (cone), head (sphere), hat brim (cylinder) and crown, joined into one mesh ------------------
robe = material("robe", (0.42, 0.42, 0.55, 1.0))
skin = material("skin", (0.90, 0.72, 0.58, 1.0))
hat = material("hat", (0.08, 0.08, 0.08, 1.0))
parts = [
    add(bpy.ops.mesh.primitive_cone_add, "robe", robe, vertices=12, radius1=0.45, radius2=0.22, depth=1.0, location=(0, 0, 0.5)),
    add(bpy.ops.mesh.primitive_uv_sphere_add, "head", skin, segments=12, ring_count=8, radius=0.22, location=(0, 0, 1.2)),
    add(bpy.ops.mesh.primitive_cylinder_add, "brim", hat, vertices=12, radius=0.36, depth=0.04, location=(0, 0, 1.36)),
    add(bpy.ops.mesh.primitive_cylinder_add, "crown", hat, vertices=12, radius=0.2, depth=0.22, location=(0, 0, 1.48)),
    # a nose so you can see which way he faces (+X, the direction the 2D sprite walks when moving right)
    add(bpy.ops.mesh.primitive_cone_add, "nose", skin, vertices=6, radius1=0.05, radius2=0.0, depth=0.12,
        location=(0.24, 0, 1.2), rotation=(0, math.pi / 2, 0)),
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
