# Exports a .blend's meshes for bicycle's 3D mode. Run headless (from the repo root):
#   blender -b test/blender/meadow3d.blend --python scripts/blender/export.py -- [out_dir]
# out_dir defaults to test/genome/models. For each mesh object it writes:
#   <object>.obj + <object>.mtl   triangulated geometry with per-corner normals and UVs, Y up (Blender's Z up is
#                                 converted: x, y, z -> x, z, -y), materials as usemtl groups
#   <image>.bimg                  each image texture, true-color .bimg (include/data/BImg.h), top row first
#   scene3d.lua                   { objects = { { name, mesh, role }, ... } }: role "character" objects are in their
#                                 own space (origin at the feet, facing +X) for the game to place with a model gene;
#                                 everything else ("static") has its world transform baked in.
# bicycle-pack (the 'assets' build target) then turns each .obj into an Edgebreaker-compressed .bmesh.
#   <object>.bskin                (rigged meshes: an Armature modifier) the skeleton (hierarchy, inverse bind
#                                 matrices), per-vertex joints/weights (the 4 largest, per OBJ vertex) and every
#                                 action's keyframes, sampled per frame (include/data/Skin.h; float, uncompressed:
#                                 dev builds read it as is, bicycle-pack quantizes and deflates it).
# Rigged meshes are exported in their rest (bind) pose.
import bpy
import bmesh
import os
import struct
import sys

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = os.path.abspath(argv[0] if argv else os.path.join(os.getcwd(), "test", "genome", "models"))
os.makedirs(OUT, exist_ok=True)


def yup(v):
    return (v[0], v[2], -v[1])


def write_bimg(image, path):
    w, h = image.size
    px = list(image.pixels)  # RGBA floats, bottom row first
    rows = []
    for y in range(h - 1, -1, -1):
        row = bytearray()
        for x in range(w):
            i = 4 * (y * w + x)
            r, g, b, a = (max(0, min(255, int(round(c * 255)))) for c in px[i:i + 4])
            row += struct.pack("<I", r | (g << 8) | (b << 16) | (a << 24))
        rows.append(bytes(row))
    with open(path, "wb") as f:
        f.write(b"BIMG" + struct.pack("<4H", w, h, 0, 0) + b"".join(rows))


def material_info(mat):
    """(diffuse rgba, image or None) from a material's Principled BSDF."""
    rgba = tuple(mat.diffuse_color) if mat else (0.8, 0.8, 0.8, 1.0)
    image = None
    if mat and mat.use_nodes:
        bsdf = next((n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
        if bsdf:
            base = bsdf.inputs["Base Color"]
            rgba = tuple(base.default_value)
            if base.is_linked and base.links[0].from_node.type == "TEX_IMAGE":
                image = base.links[0].from_node.image
                rgba = (1.0, 1.0, 1.0, rgba[3])
    return rgba, image


C = None  # Z up -> Y up, as a 4x4 (set below, once mathutils is imported)


def rig_of(obj):
    m = next((m for m in obj.modifiers if m.type == "ARMATURE" and m.object), None)
    return m.object if m else None


def ordered_bones(arm):
    """Bones with every parent before its children."""
    out = []
    def visit(b):
        out.append(b)
        for c in b.children:
            visit(c)
    for b in arm.data.bones:
        if b.parent is None:
            visit(b)
    return out


def flat(m):  # column major
    return [m[r][c] for c in range(4) for r in range(4)]


def export_skin(obj, rig, path):
    from mathutils import Matrix
    bones = ordered_bones(rig)
    index = {b.name: i for i, b in enumerate(bones)}
    mesh_from_rig = obj.matrix_world.inverted() @ rig.matrix_world
    Ci = C.inverted()
    def to_mesh_yup(m):
        return C @ mesh_from_rig @ m @ Ci
    out = bytearray(b"BSKN")
    actions = [a for a in bpy.data.actions if any(fc.data_path.startswith("pose.bones") for fc in a.fcurves)]
    nverts = len(obj.data.vertices)
    out += struct.pack("<BBHHI", 1, 2, len(bones), len(actions), nverts)
    for b in bones:
        name = b.name.encode()
        out += struct.pack("<B", len(name)) + name
        out += struct.pack("<h", index[b.parent.name] if b.parent else -1)
        out += struct.pack("<16f", *flat(to_mesh_yup(b.matrix_local).inverted()))
    # Per vertex: the 4 strongest bone influences, normalized, as u8 summing to 255.
    groups = {g.index: g.name for g in obj.vertex_groups}
    for v in obj.data.vertices:
        inf = sorted(((g.weight, index[groups[g.group]]) for g in v.groups
                      if groups.get(g.group) in index and g.weight > 0), reverse=True)[:4]
        if not inf:
            inf = [(1.0, 0)]
        total = sum(w for w, _ in inf)
        q = [int(round(255 * w / total)) for w, _ in inf]
        q[0] += 255 - sum(q)
        joints = [j for _, j in inf] + [0] * (4 - len(inf))
        q += [0] * (4 - len(q))
        out += struct.pack("<4B4B", *joints, *q)
    scene = bpy.context.scene
    saved = rig.animation_data.action if rig.animation_data else None
    rig.data.pose_position = "POSE"
    for act in actions:
        rig.animation_data.action = act
        start, end = (int(x) for x in act.frame_range)
        frames = int(act.get("bicycle_frames", end - start))  # last key repeats the first for looping
        fps = scene.render.fps / scene.render.fps_base
        name = act.name.encode()
        out += struct.pack("<B", len(name)) + name + struct.pack("<fH", fps, frames)
        for f in range(frames):
            scene.frame_set(start + f)
            glob = [to_mesh_yup(rig.pose.bones[b.name].matrix) for b in bones]
            for i, b in enumerate(bones):
                local = glob[index[b.parent.name]].inverted() @ glob[i] if b.parent else glob[i]
                t, q, _ = local.decompose()
                out += struct.pack("<3f4f", t.x, t.y, t.z, q.x, q.y, q.z, q.w)
    if saved:
        rig.animation_data.action = saved
    rig.data.pose_position = "REST"
    with open(path, "wb") as f:
        f.write(bytes(out))
    print("bicycle export: %s: %d joints, %d actions (%s), %d skinned verts" % (
        os.path.basename(path), len(bones), len(actions), ", ".join(a.name for a in actions), nverts))


def export_object(obj, depsgraph):
    role = obj.get("bicycle_role", "static")
    rig = rig_of(obj)
    if rig:
        rig.data.pose_position = "REST"  # export the mesh in its bind pose
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
    ev = obj.evaluated_get(depsgraph)
    mesh = ev.to_mesh()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    if role != "character":
        bm.transform(obj.matrix_world)  # static objects: bake the world transform
    bm.to_mesh(mesh)
    bm.free()
    mesh.calc_loop_triangles()
    uv_layer = mesh.uv_layers.active
    normals = mesh.corner_normals

    name = obj.name
    lines = ["# bicycle export of %s" % name, "mtllib %s.mtl" % name]
    for v in mesh.vertices:
        lines.append("v %.6f %.6f %.6f" % yup(v.co))
    for loop in mesh.loops:
        if uv_layer:
            u, v = uv_layer.data[loop.index].uv
            lines.append("vt %.6f %.6f" % (u, v))
    for loop in mesh.loops:
        lines.append("vn %.6f %.6f %.6f" % yup(normals[loop.index].vector))
    by_mat = {}
    for poly in mesh.polygons:
        by_mat.setdefault(poly.material_index, []).append(poly)
    mtl = []
    textures = 0
    for mi, polys in sorted(by_mat.items()):
        mat = obj.material_slots[mi].material if mi < len(obj.material_slots) else None
        mname = mat.name if mat else "default"
        rgba, image = material_info(mat)
        mtl += ["newmtl %s" % mname, "Kd %.4f %.4f %.4f" % rgba[:3], "d %.4f" % rgba[3]]
        if image:
            write_bimg(image, os.path.join(OUT, image.name + ".bimg"))
            mtl.append("map_Kd %s.png" % image.name)  # bicycle-pack uses the .bimg next to it
            textures += 1
        mtl.append("")
        lines.append("usemtl %s" % mname)
        for poly in polys:
            corners = []
            for li in poly.loop_indices:
                vi = mesh.loops[li].vertex_index + 1
                corners.append("%d/%s/%d" % (vi, (li + 1) if uv_layer else "", li + 1))
            lines.append("f " + " ".join(corners))
    with open(os.path.join(OUT, name + ".obj"), "w") as f:
        f.write("\n".join(lines) + "\n")
    with open(os.path.join(OUT, name + ".mtl"), "w") as f:
        f.write("\n".join(mtl))
    print("bicycle export: %s (%s): %d verts, %d tris, %d materials, %d textures" % (
        name, role, len(mesh.vertices), len(mesh.polygons), len(by_mat), textures))
    ev.to_mesh_clear()
    if rig:
        export_skin(obj, rig, os.path.join(OUT, name + ".bskin"))
    return {"name": name, "role": role}


from mathutils import Matrix  # noqa: E402
C = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))
depsgraph = bpy.context.evaluated_depsgraph_get()
objects = [export_object(o, depsgraph) for o in bpy.context.scene.objects if o.type == "MESH"]
with open(os.path.join(OUT, "scene3d.lua"), "w") as f:
    f.write("-- written by scripts/blender/export.py from %s\nreturn {\n  objects = {\n" % os.path.basename(bpy.data.filepath))
    for o in objects:
        f.write('    { name = "%s", mesh = "models/%s", role = "%s" },\n' % (o["name"], o["name"], o["role"]))
    f.write("  }\n}\n")
print("bicycle export: wrote %d objects to %s" % (len(objects), OUT))
