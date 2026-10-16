#pragma once
// Reads a Wavefront OBJ (+ its .mtl) from a loose file. Texture paths are returned as written in the .mtl.
// Used by the packer (OBJ -> .bmesh) and by dev builds, which load meshes without packing.
#include "data/Mesh.h"
#include "data/Skin.h"
#include <string>

auto readObj( const std::string& objPath, MeshData& out, std::string& err ) -> bool;
// A skinned OBJ's sidecar (<name>.bskin next to it): copies its per-OBJ-vertex joints/weights onto the mesh's
// vertices (through MeshData::srcVertex). Returns false if the counts don't match.
auto applySkinVertices( MeshData& m, const Skin& s ) -> bool;
