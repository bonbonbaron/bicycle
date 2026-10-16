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
# Armatures: modifiers are applied, so a rigged mesh exports in its current pose; skeletal animation isn't
# exported yet (see scripts/blender/README.md).
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


def export_object(obj, depsgraph):
    role = obj.get("bicycle_role", "static")
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
    if any(m.type == "ARMATURE" for m in obj.modifiers):
        print("bicycle export: %s is rigged; exported in its current pose (skeletal animation isn't exported)" % name)
    ev.to_mesh_clear()
    return {"name": name, "role": role}


depsgraph = bpy.context.evaluated_depsgraph_get()
objects = [export_object(o, depsgraph) for o in bpy.context.scene.objects if o.type == "MESH"]
with open(os.path.join(OUT, "scene3d.lua"), "w") as f:
    f.write("-- written by scripts/blender/export.py from %s\nreturn {\n  objects = {\n" % os.path.basename(bpy.data.filepath))
    for o in objects:
        f.write('    { name = "%s", mesh = "models/%s", role = "%s" },\n' % (o["name"], o["name"], o["role"]))
    f.write("  }\n}\n")
print("bicycle export: wrote %d objects to %s" % (len(objects), OUT))
