# Builds the true-3D demo levels headless (Blender 4.x):
#   blender -b --factory-startup --python scripts/blender/make_level.py -- plat   -> plat3d/blender/plat3d.blend
#   blender -b --factory-startup --python scripts/blender/make_level.py -- fps    -> fps3d/blender/fps3d.blend
# then export (every Blender scene is a level, genome/level/<scene>.lua):
#   blender -b plat3d/blender/plat3d.blend --python scripts/blender/export.py
# Level geometry is authored here in game coordinates (meters, Y up; converted to Blender's Z up) as separate
# objects (so the renderer can frustum-cull them), box-mapped UVs at TEX_SCALE texture repeats per meter, with
# small procedural textures (written as .bimg by the exporter). Genome models (bicycle_role = "character": the
# coin, the moving platform, targets...) live in a Blender scene "models" that isn't a level. The entities (genome
# instances: coins, doors, the hero...) and spawn points are placed with the bicycle add-on's Python API
# (tools/blender/bicycle_blender.py), exactly what its Add entity / Add spawn point / Add path point buttons do.
import bpy
import bmesh
import math
import os
import sys

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
WHICH = argv[0] if argv else "plat"
ROOT = os.getcwd()
sys.path.insert(0, os.path.join(ROOT, "tools", "blender"))
import bicycle_blender as B  # noqa: E402

bpy.ops.wm.read_factory_settings(use_empty=True)
B.register()
TARGET = [None]  # the Blender scene objects are added to


def new_scene(name, level=True):
    sc = bpy.data.scenes.new(name)
    sc.bicycle.export_level = level
    TARGET[0] = sc
    return sc


def g2b(x, y, z):  # game (Y up) -> Blender (Z up)
    return (x, -z, y)


def texture(name, w, h, fn):
    img = bpy.data.images.new(name, w, h, alpha=True)
    px = []
    for y in range(h - 1, -1, -1):  # Blender rows are bottom first; fn gets top-first rows
        for x in range(w):
            r, g, b = fn(x, h - 1 - y)
            px += [r / 255, g / 255, b / 255, 1.0]
    img.pixels.foreach_set(px)
    img.update()
    img.pack()  # keep the generated pixels in the .blend
    return img


def material(name, rgba=(0.8, 0.8, 0.8, 1), img=None):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = rgba
    if img:
        t = m.node_tree.nodes.new("ShaderNodeTexImage")
        t.image = img
        m.node_tree.links.new(t.outputs["Color"], bsdf.inputs["Base Color"])
    return m


def noise(x, y, s):
    return ((x * 73856093) ^ (y * 19349663) ^ (s * 83492791)) % 97 / 97.0


def grass(x, y):
    n = noise(x, y, 1)
    return (int(70 + 40 * n), int(140 + 50 * n), int(50 + 25 * n))


def stone(x, y):
    n = noise(x, y, 2)
    edge = x % 16 == 0 or y % 16 == 0
    v = int(120 + 50 * n) if not edge else 80
    return (v, v, int(v * 1.05))


def bricks(x, y):
    row = y // 4
    mortar = y % 4 == 3 or (x + (4 if row % 2 else 0)) % 8 == 7
    n = noise(x, y, 3)
    return (190, 185, 175) if mortar else (int(150 + 40 * n), int(60 + 20 * n), int(45 + 15 * n))


def concrete(x, y):
    n = noise(x, y, 4)
    v = int(140 + 30 * n)
    return (v, v, v)


def wood(x, y):
    n = noise(x, y // 3, 5)
    edge = x % 16 in (0, 15) or y % 16 in (0, 15)
    return (90, 60, 30) if edge else (int(160 + 40 * n), int(110 + 25 * n), int(60 + 15 * n))


TEX_SCALE = 0.5  # texture repeats per meter


def box_uvs(bm, uv):
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        for l in f.loops:
            c = l.vert.co
            if ax == 0:
                l[uv].uv = (c.y * TEX_SCALE, c.z * TEX_SCALE)
            elif ax == 1:
                l[uv].uv = (c.x * TEX_SCALE, c.z * TEX_SCALE)
            else:
                l[uv].uv = (c.x * TEX_SCALE, c.y * TEX_SCALE)


def finish(name, bm, mat, role="static"):
    uv = bm.loops.layers.uv.new("UVMap")
    box_uvs(bm, uv)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    TARGET[0].collection.objects.link(ob)
    me.materials.append(mat)
    ob["bicycle_role"] = role
    return ob


def add_box(bm, lo, hi):
    """Axis-aligned box from game-space corners lo, hi (world coordinates baked in the mesh)."""
    a, b = g2b(*lo), g2b(*hi)
    mn = [min(a[i], b[i]) for i in range(3)]
    mx = [max(a[i], b[i]) for i in range(3)]
    r = bmesh.ops.create_cube(bm, size=1.0)
    for v in r["verts"]:
        v.co = [mn[i] + (v.co[i] + 0.5) * (mx[i] - mn[i]) for i in range(3)]
    bm.normal_update()


def box(name, lo, hi, mat, role="static"):
    bm = bmesh.new()
    add_box(bm, lo, hi)
    return finish(name, bm, mat, role)


def ramp(name, x0, x1, z0, z1, h, mat):
    """A wedge rising along +x (game) from height 0 at x0 to h at x1, across z0..z1."""
    bm = bmesh.new()
    pts = [(x0, 0, z0), (x1, 0, z0), (x1, h, z0), (x0, 0, z1), (x1, 0, z1), (x1, h, z1)]
    vs = [bm.verts.new(g2b(*p)) for p in pts]
    for f in [(0, 2, 1), (3, 4, 5), (0, 3, 5, 2), (0, 1, 4, 3), (1, 2, 5, 4)]:
        bm.faces.new([vs[i] for i in f])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return finish(name, bm, mat)


def stairs(name, x0, z0, z1, steps, rise, run, mat):
    bm = bmesh.new()
    for i in range(steps):
        add_box(bm, (x0 + i * run, 0, z0), (x0 + (i + 1) * run, (i + 1) * rise, z1))
    return finish(name, bm, mat)


def cylinder(name, r, h, mat, segs=20, role="character", y0=0.0):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=segs, radius1=r, radius2=r, depth=h)
    for v in bm.verts:
        v.co.z += h / 2 + y0
    return finish(name, bm, mat, role)


tex_grass = texture("grass3d", 16, 16, grass)
tex_stone = texture("stone3d", 32, 32, stone)
tex_wood = texture("wood3d", 16, 16, wood)



def game(x, y, z):  # a game-space point as a Blender location
    return g2b(x, y, z)


def place(scene, genome, name, x, y, z, yaw=0.0, **params):
    return B.add_entity(scene, genome, game(x, y, z), params=params, name=name, rotation_z=math.radians(yaw))


def spawn(scene, name, x, y, z, yaw=0.0):
    return B.add_spawn(scene, name, game(x, y, z), rotation_z=math.radians(yaw))


def save():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=OUT)


def multi_box(name, boxes, mat, role="static"):
    bm = bmesh.new()
    for lo, hi in boxes:
        add_box(bm, lo, hi)
    return finish(name, bm, mat, role)


if WHICH == "plat":
    OUT = os.path.join(ROOT, "plat3d", "blender", "plat3d.blend")
    first = bpy.context.scene
    m_grass = material("grass", img=tex_grass)
    m_stone = material("stone", img=tex_stone)
    m_wood = material("wood", img=tex_wood)
    m_gold = material("gold", (1.0, 0.78, 0.15, 1))
    m_red = material("red", (0.85, 0.2, 0.2, 1))
    m_plat = material("platform", (0.3, 0.55, 0.95, 1))
    # Genome models (own space, origin at the bottom center): genome/body/model/.
    models = new_scene("models", level=False)
    cylinder("coin3d", 0.35, 0.08, m_gold, segs=24)
    bpy.data.objects["coin3d"].rotation_euler = (math.pi / 2, 0, 0)  # stands on edge, faces -Y(blender)
    b = bmesh.new()
    add_box(b, (-1.5, -0.25, -1.5), (1.5, 0.25, 1.5))
    finish("mover3d", b, m_plat, role="character")
    cylinder("flag3d", 0.06, 2.0, m_red)
    # The courtyard.
    yard = new_scene("courtyard")
    box("ground", (-24, -1, -24), (24, 0, 24), m_grass)
    # A path of platforms rising toward a tower.
    box("plat_a", (3, 0, -4), (6, 1.0, -1), m_stone)
    box("plat_b", (7.5, 0, -7), (10.5, 2.0, -4), m_stone)
    box("plat_c", (11, 0, -12), (14, 3.0, -9), m_stone)
    box("tower", (5, 0, -18), (10, 4.2, -13), m_stone)
    ramp("ramp", -10, -2, 2, 6, 2.5, m_wood)         # ~17 degrees
    box("ramp_top", (-2, 0, 2), (2, 2.5, 6), m_wood)
    stairs("stairs", -12, -8, -5, 8, 0.25, 0.5, m_stone)
    box("stairs_top", (-8, 0, -8), (-5, 2.0, -5), m_stone)
    box("wall_n", (-24, 0, -24.5), (24, 3, -24), m_stone)
    box("wall_s", (-24, 0, 24), (24, 3, 24.5), m_stone)
    box("wall_w", (-24.5, 0, -24), (-24, 3, 24), m_stone)
    box("wall_e", (24, 0, -24), (24.5, 3, 24), m_stone)
    for i, (x, z) in enumerate([(-16, 12), (14, 14), (18, -4), (-18, -16)]):  # trees: trunk + canopy (decor)
        cylinder("trunk%d" % i, 0.35, 2.6, m_wood, role="static").location = g2b(x, 0, z)
        b = bmesh.new()
        bmesh.ops.create_icosphere(b, subdivisions=2, radius=1.7)
        for v in b.verts:
            v.co.z += 3.4
        o = finish("canopy%d" % i, b, material("leaves%d" % i, (0.2, 0.55 + 0.05 * i, 0.2, 1)), role="decor")
        o.location = g2b(x, 0, z)
    # The bonus room: a stone floor, wooden walls and three blocks to jump on.
    bonus = new_scene("bonus")
    box("bonus_floor", (-7, -0.5, -7), (7, 0, 7), m_stone)
    multi_box("bonus_walls", [((-7.5, 0, -7.5), (7.5, 4, -7)), ((-7.5, 0, 7), (7.5, 4, 7.5)),
                              ((-7.5, 0, -7), (-7, 4, 7)), ((7, 0, -7), (7.5, 4, 7))], m_wood)
    multi_box("bonus_blocks", [((-4, 0, -1), (-2, 1, 1)), ((2, 0, -1), (4, 2, 1)), ((-1, 0, -4), (1, 3, -2))], m_stone)
    bpy.data.scenes.remove(first)
    save()  # so the add-on finds plat3d/genome next to the .blend
    B.catalog(refresh=True)
    # Entities and spawn points (what the Bicycle panel's buttons do).
    spawn(yard, "default", 4.5, 0.5, 9)
    spawn(yard, "from_bonus", 7.5, 4.4, -15.2)
    place(yard, "hero", "hero", 4.5, 0.5, 9)
    place(yard, "camera", "camera", 4.5, 3, 16, **{"camera.target": "hero"})
    for i, (x, y, z) in enumerate([(4.5, 1.6, -2.5), (9, 2.6, -5.5), (12.5, 3.6, -10.5), (-6, 1.2, 4), (0, 3.1, 4),
                                   (-9, 1.2, -6.5), (-6.5, 2.6, -6.5), (0, 2.2, -10), (-14, 0.6, 14)]):
        place(yard, "coin", "coin%d" % (i + 1), x, y, z)
    mover = place(yard, "mover", "mover", 2, 0.75, -14)
    B.add_path_point(mover, game(2, 4.0, -14))
    place(yard, "flag", "flag_to_bonus", 7.5, 4.2, -17.5, **{"door.to": "bonus", "door.spawn": "entry"})
    spawn(bonus, "entry", 0, 0.1, 2.5)
    # The hero and camera persist, so arriving through the flag they come along; these only matter when the game
    # starts here (PLAT3D_SCENE=bonus).
    place(bonus, "hero", "hero", 0, 0.1, 2.5)
    place(bonus, "camera", "camera", 0, 3, 9, **{"camera.target": "hero"})
    for i in range(8):
        a = i * math.pi / 4
        place(bonus, "coin", "bonus_coin%d" % (i + 1), 4.5 * math.cos(a), 0.6, 4.5 * math.sin(a))
    place(bonus, "coin", "bonus_coin9", 0, 3.6, -3)
    place(bonus, "flag", "flag_to_courtyard", 0, 0, -6, **{"door.to": "courtyard", "door.spawn": "from_bonus"})
else:
    OUT = os.path.join(ROOT, "fps3d", "blender", "fps3d.blend")
    first = bpy.context.scene
    tex_brick = texture("brick3d", 32, 32, bricks)
    tex_conc = texture("concrete3d", 16, 16, concrete)
    m_brick = material("brick", img=tex_brick)
    m_conc = material("concrete", img=tex_conc)
    m_wood = material("crate", img=tex_wood)
    m_metal = material("metal", (0.45, 0.48, 0.52, 1))
    m_red = material("target_red", (0.9, 0.12, 0.1, 1))
    m_white = material("target_white", (0.95, 0.95, 0.95, 1))
    models = new_scene("models", level=False)
    # Target (own space: a disc with a bullseye on a post, facing +Z game): one mesh, three materials.
    b = bmesh.new()
    bmesh.ops.create_cone(b, cap_ends=True, segments=24, radius1=0.45, radius2=0.45, depth=0.08)
    for v in b.verts:
        v.co = (v.co.x, v.co.z, v.co.y + 1.5)  # disc in the XZ(blender) plane -> faces -Y blender = +Z game
    old = set(b.faces)
    r = bmesh.ops.create_cone(b, cap_ends=True, segments=24, radius1=0.25, radius2=0.25, depth=0.1)
    for v in r["verts"]:
        v.co = (v.co.x, v.co.z - 0.02, v.co.y + 1.5)
    for f in set(b.faces) - old:
        f.material_index = 1  # the bullseye: white
    old = set(b.faces)
    add_box(b, (-0.05, 0, -0.05), (0.05, 1.05, 0.05))
    for f in set(b.faces) - old:
        f.material_index = 2  # the post: metal
    t = finish("target3d", b, m_red, role="character")
    t.data.materials.append(m_white)
    t.data.materials.append(m_metal)
    b = bmesh.new()  # a hit spark
    bmesh.ops.create_icosphere(b, subdivisions=1, radius=0.08)
    finish("spark3d", b, material("spark", (1.0, 0.85, 0.3, 1)), role="character")
    b = bmesh.new()  # the first-person gun: a body and a barrel along -Z (game), origin at the grip
    add_box(b, (-0.04, -0.06, -0.30), (0.04, 0.04, 0.05))
    add_box(b, (-0.02, -0.01, -0.55), (0.02, 0.025, -0.30))
    add_box(b, (-0.03, -0.16, -0.02), (0.03, -0.06, 0.04))
    finish("gun3d", b, material("gunmetal", (0.18, 0.19, 0.22, 1)), role="character")
    rng = new_scene("range")
    box("floor", (-16, -1, -16), (16, 0, 16), m_conc)
    box("ceiling_beam_a", (-16, 4.6, -1), (16, 5.0, 1), m_metal, role="decor")
    for name, lo, hi in [("wall_n", (-16, 0, -16.5), (16, 5, -16)), ("wall_s", (-16, 0, 16), (16, 5, 16.5)),
                         ("wall_w", (-16.5, 0, -16), (-16, 5, 16)), ("wall_e", (16, 0, -16), (16.5, 5, 16))]:
        box(name, lo, hi, m_brick)
    # Cover: crates, pillars, a dividing wall with a doorway, a raised catwalk with stairs.
    for i, (x, z, s) in enumerate([(-6, -4, 1.2), (-4.8, -4, 1.2), (-5.4, -4, 2.4), (5, 6, 1.4), (8, -9, 1.2), (-10, 9, 1.6)]):
        y0 = 1.2 if s == 2.4 else 0  # one crate stacked on two
        h = 1.2 if s == 2.4 else s
        box("crate%d" % i, (x - h / 2, y0, z - h / 2), (x + h / 2, y0 + h, z + h / 2), m_wood)
    for i, (x, z) in enumerate([(-8, -10), (8, 10), (0, 0)]):
        box("pillar%d" % i, (x - 0.6, 0, z - 0.6), (x + 0.6, 5, z + 0.6), m_brick)
    box("divider_a", (-16, 0, 4), (-5, 3, 4.5), m_brick)
    box("divider_b", (-2.5, 0, 4), (4, 3, 4.5), m_brick)
    box("catwalk", (9, 2.0, -16), (16, 2.3, -4), m_metal)
    stairs("catwalk_stairs", 1, -6, -4, 8, 0.25, 1.0, m_metal)  # x 1..9, up to 2.0
    bpy.data.scenes.remove(first)
    save()
    B.catalog(refresh=True)
    spawn(rng, "default", -3.75, 0.2, 12)
    place(rng, "player", "player", -3.75, 0.2, 12)
    place(rng, "camera", "camera", -3.75, 1.8, 12, **{"camera.target": "player"})
    for i, (x, z, yaw) in enumerate([(-3, -14, 0), (6, -12, 0), (-12, -2, 90), (12, 0, -90), (12.5, -10, -90),
                                     (-12, 12, 90), (-3, -3, 0), (8, 13, 180)]):
        place(rng, "target", "target%d" % (i + 1), x, 2.3 if z == -10 else 0, z, yaw)

save()
print("bicycle make_level: wrote", OUT)
