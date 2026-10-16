// Loose Wavefront OBJ (+ .mtl) loading: the packer turns OBJs into .bmesh, and dev builds load them directly.
#include "data/Obj.h"
#include "data/Pack.h"
#include <map>
#include <sstream>

namespace {
auto dirOf( const std::string& p ) -> std::string {
  const auto s = p.find_last_of( "/\\" );
  return s == std::string::npos ? "" : p.substr( 0, s + 1 );
}

auto readMtl( const std::string& path, std::map<std::string, MeshMaterial>& mats ) -> void {
  auto bytes = assets::readFile( path, mem::TEMPORARY );
  std::istringstream f( bytes ? std::string( *bytes ) : std::string() );
  std::string line;
  MeshMaterial* cur = nullptr;
  while ( std::getline( f, line ) ) {
    std::istringstream ss( line );
    std::string tag;
    ss >> tag;
    if ( tag == "newmtl" ) {
      std::string name;
      std::getline( ss >> std::ws, name );
      cur = &mats[name];
      cur->name = name;
    }
    else if ( cur && tag == "Kd" ) ss >> cur->rgba[0] >> cur->rgba[1] >> cur->rgba[2];
    else if ( cur && tag == "d" ) ss >> cur->rgba[3];
    else if ( cur && tag == "map_Kd" ) {
      std::string rest;
      std::getline( ss >> std::ws, rest );
      // Options like "-s 1 1 1" may precede the file; the file name is the last token.
      const auto sp = rest.find_last_of( ' ' );
      cur->texture = sp == std::string::npos ? rest : rest.substr( sp + 1 );
    }
  }
}
}  // namespace

auto readObj( const std::string& objPath, MeshData& out, std::string& err ) -> bool {
  auto bytes = assets::readFile( objPath, mem::TEMPORARY );
  if ( !bytes ) { err = "can't open " + objPath; return false; }
  std::istringstream f{ std::string( *bytes ) };
  std::vector<float> v, vt, vn;
  std::map<std::string, MeshMaterial> mtlLib;
  std::map<std::string, uint8_t> matIdx;
  uint8_t curMat = 0;
  bool anyUv = false, anyNormal = false;
  struct Corner { int v, t, n; };
  std::vector<Corner> corners;
  std::vector<uint8_t> triMat;
  std::string line;
  while ( std::getline( f, line ) ) {
    std::istringstream ss( line );
    std::string tag;
    ss >> tag;
    if ( tag == "v" ) { float x, y, z; ss >> x >> y >> z; v.insert( v.end(), { x, y, z } ); }
    else if ( tag == "vt" ) { float a, b; ss >> a >> b; vt.insert( vt.end(), { a, b } ); }
    else if ( tag == "vn" ) { float x, y, z; ss >> x >> y >> z; vn.insert( vn.end(), { x, y, z } ); }
    else if ( tag == "mtllib" ) { std::string name; std::getline( ss >> std::ws, name ); readMtl( dirOf( objPath ) + name, mtlLib ); }
    else if ( tag == "usemtl" ) {
      std::string name;
      std::getline( ss >> std::ws, name );
      auto it = matIdx.find( name );
      if ( it == matIdx.end() ) {
        it = matIdx.emplace( name, static_cast<uint8_t>( out.materials.size() ) ).first;
        MeshMaterial m = mtlLib.count( name ) ? mtlLib[name] : MeshMaterial{};
        m.name = name;
        out.materials.push_back( m );
      }
      curMat = it->second;
    }
    else if ( tag == "f" ) {
      std::vector<Corner> poly;
      std::string tok;
      while ( ss >> tok ) {
        Corner c{ -1, -1, -1 };
        int* fields[3] = { &c.v, &c.t, &c.n };
        std::stringstream ts( tok );
        std::string part;
        for ( int i = 0; i < 3 && std::getline( ts, part, '/' ); ++i ) {
          if ( !part.empty() ) {
            int idx = std::stoi( part );
            const int count = i == 0 ? static_cast<int>( v.size() / 3 ) : i == 1 ? static_cast<int>( vt.size() / 2 ) : static_cast<int>( vn.size() / 3 );
            *fields[i] = idx < 0 ? count + idx : idx - 1;
          }
        }
        anyUv |= c.t >= 0;
        anyNormal |= c.n >= 0;
        poly.push_back( c );
      }
      for ( size_t i = 2; i < poly.size(); ++i ) {  // fan-triangulate
        corners.insert( corners.end(), { poly[0], poly[i - 1], poly[i] } );
        triMat.push_back( curMat );
      }
    }
  }
  if ( corners.empty() ) { err = objPath + " has no faces"; return false; }
  if ( out.materials.empty() ) out.materials.push_back( MeshMaterial{ "default" } );
  // Keep only referenced vertices.
  std::vector<int32_t> remap( v.size() / 3, -1 );
  for ( auto& c : corners ) {
    if ( c.v < 0 || c.v >= static_cast<int>( remap.size() ) ) { err = "bad vertex index"; return false; }
    if ( remap[c.v] < 0 ) {
      remap[c.v] = static_cast<int32_t>( out.pos.size() / 3 );
      out.srcVertex.push_back( static_cast<uint32_t>( c.v ) );
      out.pos.insert( out.pos.end(), { v[3 * c.v], v[3 * c.v + 1], v[3 * c.v + 2] } );
    }
    out.tris.push_back( remap[c.v] );
    if ( anyNormal ) {
      if ( c.n >= 0 ) out.normals.insert( out.normals.end(), { vn[3 * c.n], vn[3 * c.n + 1], vn[3 * c.n + 2] } );
      else out.normals.insert( out.normals.end(), { 0, 0, 1 } );
    }
    if ( anyUv ) {
      if ( c.t >= 0 ) out.uvs.insert( out.uvs.end(), { vt[2 * c.t], vt[2 * c.t + 1] } );
      else out.uvs.insert( out.uvs.end(), { 0, 0 } );
    }
  }
  out.triMat.assign( triMat.begin(), triMat.end() );
  return true;
}

auto applySkinVertices( MeshData& m, const Skin& s ) -> bool {
  const size_t nObj = s.vJoints.size() / 4;
  if ( nObj == 0 || m.srcVertex.size() != m.nVerts() ) return false;
  m.joints.resize( 4 * m.nVerts() );
  m.weights.resize( 4 * m.nVerts() );
  for ( size_t v = 0; v < m.nVerts(); ++v ) {
    const uint32_t src = m.srcVertex[v];
    if ( src >= nObj ) return false;
    for ( int k = 0; k < 4; ++k ) {
      m.joints[4 * v + k] = s.vJoints[4 * src + k];
      m.weights[4 * v + k] = s.vWeights[4 * src + k];
    }
  }
  return true;
}
