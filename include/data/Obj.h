#pragma once
// Reads a Wavefront OBJ (+ its .mtl) from a loose file. Texture paths are returned as written in the .mtl.
// Used by the packer (OBJ -> .bmesh) and by dev builds, which load meshes without packing.
#include "data/Mesh.h"
#include <string>

auto readObj( const std::string& objPath, MeshData& out, std::string& err ) -> bool;
