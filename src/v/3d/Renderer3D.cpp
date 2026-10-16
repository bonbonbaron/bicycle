// 3D mode: OpenGL 3.3 core through SDL3. The scene's camera box picks what the 3D camera follows; entities with a
// model (setModel, the Lua "model" gene) are drawn as meshes (.bmesh, Edgebreaker-compressed, from Blender via
// tools/blender/export.py), other entities as camera-facing billboards of their current sprite frame, and static
// scenery (addStaticModel) around them. Windows, the HUD and dialogue are PixelRenderer's 2D framebuffer composited
// over the 3D world as an overlay.
//
// Cells -> world: X = cells / 16 (one 8-pixel-wide sprite = 1 unit), Y = height above the ground in pixel rows / 8,
// Z = 0 for models, slightly behind for billboards. The ground row is the lowest entity bottom in view.
#include "v/sdl/PixelRenderer.h"
#include "v/common/Image.h"
#include "data/Mesh.h"
#include "data/BImg.h"
#include "data/Pack.h"
#include "m/World.h"
#include "m/Box.h"
#include "m/Rect.h"
#include "Constants.h"
#include <SDL3/SDL.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <unordered_map>

#ifdef _WIN32
#define BGL_API __stdcall
#else
#define BGL_API
#endif

namespace gfx {
namespace {

// --- a minimal OpenGL 3.3 core loader (SDL_GL_GetProcAddress) --------------------------------------------------------
using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLfloat = float;
using GLbitfield = unsigned int;
using GLboolean = unsigned char;
using GLchar = char;
using GLsizeiptr = std::ptrdiff_t;

enum : GLenum {
  GL_COLOR_BUFFER_BIT = 0x4000, GL_DEPTH_BUFFER_BIT = 0x0100, GL_DEPTH_TEST = 0x0B71, GL_BLEND = 0x0BE2,
  GL_SRC_ALPHA = 0x0302, GL_ONE_MINUS_SRC_ALPHA = 0x0303, GL_ARRAY_BUFFER = 0x8892, GL_STATIC_DRAW = 0x88E4,
  GL_DYNAMIC_DRAW = 0x88E8, GL_FLOAT = 0x1406, GL_VERTEX_SHADER = 0x8B31, GL_FRAGMENT_SHADER = 0x8B30,
  GL_COMPILE_STATUS = 0x8B81, GL_LINK_STATUS = 0x8B82, GL_TEXTURE_2D = 0x0DE1, GL_RGBA = 0x1908, GL_BGRA = 0x80E1,
  GL_RGBA8 = 0x8058, GL_UNSIGNED_BYTE = 0x1401, GL_TEXTURE_MIN_FILTER = 0x2801, GL_TEXTURE_MAG_FILTER = 0x2800,
  GL_NEAREST = 0x2600, GL_TEXTURE_WRAP_S = 0x2802, GL_TEXTURE_WRAP_T = 0x2803, GL_REPEAT = 0x2901,
  GL_CLAMP_TO_EDGE = 0x812F, GL_TRIANGLES = 0x0004, GL_TRIANGLE_STRIP = 0x0005, GL_TEXTURE0 = 0x84C0,
  GL_LEQUAL = 0x0203, GL_PACK_ALIGNMENT = 0x0D05, GL_UNPACK_ALIGNMENT = 0x0CF5, GL_VERSION = 0x1F02,
  GL_RENDERER = 0x1F01,
};

#define BGL_FUNCS( X ) \
  X( void, ClearColor, GLfloat, GLfloat, GLfloat, GLfloat ) \
  X( void, Clear, GLbitfield ) \
  X( void, Viewport, GLint, GLint, GLsizei, GLsizei ) \
  X( void, Enable, GLenum ) \
  X( void, Disable, GLenum ) \
  X( void, BlendFunc, GLenum, GLenum ) \
  X( void, DepthFunc, GLenum ) \
  X( void, GenBuffers, GLsizei, GLuint* ) \
  X( void, DeleteBuffers, GLsizei, const GLuint* ) \
  X( void, BindBuffer, GLenum, GLuint ) \
  X( void, BufferData, GLenum, GLsizeiptr, const void*, GLenum ) \
  X( void, GenVertexArrays, GLsizei, GLuint* ) \
  X( void, BindVertexArray, GLuint ) \
  X( void, VertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const void* ) \
  X( void, EnableVertexAttribArray, GLuint ) \
  X( GLuint, CreateShader, GLenum ) \
  X( void, ShaderSource, GLuint, GLsizei, const GLchar* const*, const GLint* ) \
  X( void, CompileShader, GLuint ) \
  X( void, GetShaderiv, GLuint, GLenum, GLint* ) \
  X( void, GetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar* ) \
  X( GLuint, CreateProgram ) \
  X( void, AttachShader, GLuint, GLuint ) \
  X( void, LinkProgram, GLuint ) \
  X( void, GetProgramiv, GLuint, GLenum, GLint* ) \
  X( void, GetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar* ) \
  X( void, UseProgram, GLuint ) \
  X( GLint, GetUniformLocation, GLuint, const GLchar* ) \
  X( void, UniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat* ) \
  X( void, Uniform4fv, GLint, GLsizei, const GLfloat* ) \
  X( void, Uniform1i, GLint, GLint ) \
  X( void, GenTextures, GLsizei, GLuint* ) \
  X( void, BindTexture, GLenum, GLuint ) \
  X( void, TexImage2D, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void* ) \
  X( void, TexParameteri, GLenum, GLenum, GLint ) \
  X( void, ActiveTexture, GLenum ) \
  X( void, DrawArrays, GLenum, GLint, GLsizei ) \
  X( void, ReadPixels, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void* ) \
  X( void, PixelStorei, GLenum, GLint ) \
  X( const unsigned char*, GetString, GLenum )

struct GL {
#define BGL_DECL( ret, name, ... ) ret( BGL_API* name )( __VA_ARGS__ ) = nullptr;
  BGL_FUNCS( BGL_DECL )
#undef BGL_DECL
  auto load() -> bool {
    bool ok = true;
#define BGL_LOAD( ret, name, ... ) \
    name = reinterpret_cast<decltype( name )>( SDL_GL_GetProcAddress( "gl" #name ) ); \
    if ( !name ) { std::fprintf( stderr, "bicycle 3D: missing gl" #name "\n" ); ok = false; }
    BGL_FUNCS( BGL_LOAD )
#undef BGL_LOAD
    return ok;
  }
};

// --- small math ------------------------------------------------------------------------------------------------------
using Mat4 = std::array<float, 16>;  // column-major

auto mul( const Mat4& a, const Mat4& b ) -> Mat4 {
  Mat4 r{};
  for ( int c = 0; c < 4; ++c )
    for ( int rr = 0; rr < 4; ++rr )
      for ( int k = 0; k < 4; ++k ) r[c * 4 + rr] += a[k * 4 + rr] * b[c * 4 + k];
  return r;
}

auto identity() -> Mat4 { return { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }; }

auto translate( float x, float y, float z ) -> Mat4 {
  Mat4 m = identity();
  m[12] = x; m[13] = y; m[14] = z;
  return m;
}

auto rotateY( float a ) -> Mat4 {
  Mat4 m = identity();
  m[0] = std::cos( a ); m[2] = -std::sin( a ); m[8] = std::sin( a ); m[10] = std::cos( a );
  return m;
}

auto perspective( float fovY, float aspect, float n, float f ) -> Mat4 {
  const float t = 1.0f / std::tan( fovY / 2 );
  Mat4 m{};
  m[0] = t / aspect; m[5] = t; m[10] = ( f + n ) / ( n - f ); m[11] = -1; m[14] = 2 * f * n / ( n - f );
  return m;
}

auto lookAt( const float e[3], const float c[3] ) -> Mat4 {
  float f[3] = { c[0] - e[0], c[1] - e[1], c[2] - e[2] };
  float l = std::sqrt( f[0] * f[0] + f[1] * f[1] + f[2] * f[2] );
  for ( auto& v : f ) v /= l;
  const float up[3] = { 0, 1, 0 };
  float s[3] = { f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2], f[0] * up[1] - f[1] * up[0] };
  l = std::sqrt( s[0] * s[0] + s[1] * s[1] + s[2] * s[2] );
  for ( auto& v : s ) v /= l;
  const float u[3] = { s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2], s[0] * f[1] - s[1] * f[0] };
  Mat4 m = identity();
  m[0] = s[0]; m[4] = s[1]; m[8] = s[2];
  m[1] = u[0]; m[5] = u[1]; m[9] = u[2];
  m[2] = -f[0]; m[6] = -f[1]; m[10] = -f[2];
  m[12] = -( s[0] * e[0] + s[1] * e[1] + s[2] * e[2] );
  m[13] = -( u[0] * e[0] + u[1] * e[1] + u[2] * e[2] );
  m[14] = f[0] * e[0] + f[1] * e[1] + f[2] * e[2];
  return m;
}

const char* VS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNrm;
layout(location = 2) in vec2 aUv;
uniform mat4 uMvp;
uniform mat4 uModel;
out vec3 vN;
out vec2 vUv;
void main() {
  gl_Position = uMvp * vec4(aPos, 1.0);
  vN = mat3(uModel) * aNrm;
  vUv = aUv;
})";

const char* FS = R"(#version 330 core
in vec3 vN;
in vec2 vUv;
uniform vec4 uColor;
uniform int uTex;
uniform int uLit;
uniform sampler2D tex;
out vec4 o;
void main() {
  vec4 c = uColor;
  if (uTex == 1) c *= texture(tex, vec2(vUv.x, 1.0 - vUv.y));  // uv v points up (OBJ); images are stored top row first
  if (c.a < 0.5) discard;
  float l = 1.0;
  if (uLit == 1) l = 0.45 + 0.55 * max(dot(normalize(vN), normalize(vec3(0.4, 0.8, 0.5))), 0.0);
  o = vec4(c.rgb * l, c.a);
})";

const char* OVERLAY_VS = R"(#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vUv;
void main() {
  vUv = vec2(aPos.x * 0.5 + 0.5, 0.5 - aPos.y * 0.5);  // framebuffer rows are top first
  gl_Position = vec4(aPos, 0.0, 1.0);
})";

const char* OVERLAY_FS = R"(#version 330 core
in vec2 vUv;
uniform sampler2D tex;
out vec4 o;
void main() { o = texture(tex, vUv); })";

struct GpuMesh {
  GLuint vao{}, vbo{};
  struct Range { int first{}, count{}; std::array<float, 4> rgba{}; GLuint tex{}; };
  std::vector<Range> ranges;  // one per material
};

struct Billboard {
  Entity entity{};
  std::shared_ptr<const BImg> pixels;
  Rect src{};
  float x{}, bottom{}, w{}, h{};  // cells
};

}  // namespace

class Renderer3D final : public PixelRenderer {
  public:
    auto mode() const -> Mode override { return Mode::ThreeD; }

    void beginScene( WinHandle win, const Box& camBox ) override {
      _cam = camBox;
      _haveScene = true;
      _sceneWin = static_cast<Win*>( win );
      // The scene's window shows the 3D world: make its part of the overlay transparent.
      const int x0 = std::max( 0, _sceneWin->x * CELL_W ), y0 = std::max( 0, _sceneWin->y * CELL_H );
      const int x1 = std::min( _fbW, ( _sceneWin->x + _sceneWin->w ) * CELL_W );
      const int y1 = std::min( _fbH, ( _sceneWin->y + _sceneWin->h ) * CELL_H );
      for ( int y = y0; y < y1; ++y ) std::fill( _fb.begin() + y * _fbW + x0, _fb.begin() + y * _fbW + x1, 0u );
    }

    // Backgrounds are 2D art; the 3D world (sky color, ground and scenery models) replaces them.
    void drawLayer( WinHandle, const Image&, int, int, bool, int, int ) override {}

    void drawEntity( WinHandle, Entity entity, const Box& ) override {
      const auto& box = World::get<Box>( entity );
      const auto& img = World::get<Image>( entity );
      Billboard b;
      b.entity = entity;
      b.pixels = img.pixels;
      b.src = World::get<Rect>( entity );
      if ( b.src.w <= 0 ) b.src = Rect( 0, 0, img.size.w >> FIXEDPT_DEC_BITS, img.size.h >> FIXEDPT_DEC_BITS );
      b.w = b.src.w;
      b.h = b.src.h;
      b.x = static_cast<float>( box.pos.x ) / ( 1 << FIXEDPT_DEC_BITS ) + b.w / 2;
      b.bottom = static_cast<float>( box.pos.y ) / ( 1 << FIXEDPT_DEC_BITS ) + b.h;
      _drawn.push_back( b );
    }

    auto setModel( Entity entity, const std::string& path ) -> bool override {
      auto m = loadMesh( path );
      if ( m ) _models[entity] = m;
      return m != nullptr;
    }

    auto addStaticModel( const std::string& path ) -> bool override {
      auto m = loadMesh( path );
      if ( m ) _statics.push_back( m );
      return m != nullptr;
    }

  protected:
    auto windowFlags() const -> uint64_t override { return SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE; }
    auto frameClearColor() const -> uint32_t override { return 0u; }  // overlay: transparent where nothing's drawn

    void beforeWindowCreated() override {
      SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
      SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
      SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
      SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
      SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    }

    void onWindowCreated() override {
      _ctx = SDL_GL_CreateContext( _window );
      if ( !_ctx || !gl.load() ) {
        std::fprintf( stderr, "bicycle 3D: no OpenGL 3.3 context: %s\n", SDL_GetError() );
        std::exit( 1 );
      }
      SDL_GL_SetSwapInterval( 0 );
      if ( std::getenv( "BICYCLE_GL_INFO" ) ) {
        std::fprintf( stderr, "GL %s on %s\n", gl.GetString( GL_VERSION ), gl.GetString( GL_RENDERER ) );
      }
      _prog = program( VS, FS );
      _overlayProg = program( OVERLAY_VS, OVERLAY_FS );
      // Billboard quad: rewritten per sprite. Overlay quad: the whole screen.
      gl.GenVertexArrays( 1, &_quadVao );
      gl.GenBuffers( 1, &_quadVbo );
      gl.BindVertexArray( _quadVao );
      gl.BindBuffer( GL_ARRAY_BUFFER, _quadVbo );
      gl.BufferData( GL_ARRAY_BUFFER, 4 * 8 * sizeof( float ), nullptr, GL_DYNAMIC_DRAW );
      vertexLayout();
      const float screen[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
      gl.GenVertexArrays( 1, &_overlayVao );
      gl.GenBuffers( 1, &_overlayVbo );
      gl.BindVertexArray( _overlayVao );
      gl.BindBuffer( GL_ARRAY_BUFFER, _overlayVbo );
      gl.BufferData( GL_ARRAY_BUFFER, sizeof screen, screen, GL_STATIC_DRAW );
      gl.VertexAttribPointer( 0, 2, GL_FLOAT, 0, 2 * sizeof( float ), nullptr );
      gl.EnableVertexAttribArray( 0 );
      gl.GenTextures( 1, &_overlayTex );
      texParams( _overlayTex, GL_CLAMP_TO_EDGE );
    }

    void onWindowDestroyed() override {
      if ( _ctx ) SDL_GL_DestroyContext( _ctx );
      _ctx = nullptr;
    }

    void presentFramebuffer() override {
      gl.Viewport( 0, 0, _fbW, _fbH );
      gl.ClearColor( 0.45f, 0.65f, 0.90f, 1.0f );  // sky
      gl.Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
      if ( _haveScene ) drawWorld();
      drawOverlay();
      if ( screenshotDue() ) readBack();
      SDL_GL_SwapWindow( _window );
      _drawn.clear();
      _haveScene = false;
    }

    void captureFrame( std::vector<uint32_t>& out ) override { out = _capture; }

  private:
    static void vertexLayout() {
      // pos3 nrm3 uv2
      gl.VertexAttribPointer( 0, 3, GL_FLOAT, 0, 8 * sizeof( float ), nullptr );
      gl.VertexAttribPointer( 1, 3, GL_FLOAT, 0, 8 * sizeof( float ), reinterpret_cast<void*>( 3 * sizeof( float ) ) );
      gl.VertexAttribPointer( 2, 2, GL_FLOAT, 0, 8 * sizeof( float ), reinterpret_cast<void*>( 6 * sizeof( float ) ) );
      gl.EnableVertexAttribArray( 0 );
      gl.EnableVertexAttribArray( 1 );
      gl.EnableVertexAttribArray( 2 );
    }

    static void texParams( GLuint tex, GLenum wrap ) {
      gl.BindTexture( GL_TEXTURE_2D, tex );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap );
    }

    static auto program( const char* vs, const char* fs ) -> GLuint {
      auto compile = []( GLenum type, const char* src ) {
        GLuint s = gl.CreateShader( type );
        gl.ShaderSource( s, 1, &src, nullptr );
        gl.CompileShader( s );
        GLint ok{};
        gl.GetShaderiv( s, GL_COMPILE_STATUS, &ok );
        if ( !ok ) {
          char log[1024];
          gl.GetShaderInfoLog( s, sizeof log, nullptr, log );
          std::fprintf( stderr, "bicycle 3D: shader: %s\n", log );
        }
        return s;
      };
      GLuint p = gl.CreateProgram();
      gl.AttachShader( p, compile( GL_VERTEX_SHADER, vs ) );
      gl.AttachShader( p, compile( GL_FRAGMENT_SHADER, fs ) );
      gl.LinkProgram( p );
      GLint ok{};
      gl.GetProgramiv( p, GL_LINK_STATUS, &ok );
      if ( !ok ) {
        char log[1024];
        gl.GetProgramInfoLog( p, sizeof log, nullptr, log );
        std::fprintf( stderr, "bicycle 3D: link: %s\n", log );
      }
      return p;
    }

    auto texture( const BImg& img ) -> GLuint {
      std::vector<uint32_t> rgba( static_cast<size_t>( img.w ) * img.h );
      for ( int y = 0; y < img.h; ++y )
        for ( int x = 0; x < img.w; ++x ) rgba[static_cast<size_t>( y ) * img.w + x] = img.pixel( x, y );
      GLuint t{};
      gl.GenTextures( 1, &t );
      texParams( t, GL_REPEAT );
      gl.PixelStorei( GL_UNPACK_ALIGNMENT, 1 );
      gl.TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, img.w, img.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data() );
      return t;
    }

    auto spriteTexture( const std::shared_ptr<const BImg>& img ) -> GLuint {
      auto it = _spriteTex.find( img.get() );
      if ( it != _spriteTex.end() ) return it->second.second;
      const GLuint t = texture( *img );
      _spriteTex[img.get()] = { img, t };  // holding the image keeps its address unique
      return t;
    }

    auto loadMesh( const std::string& path ) -> std::shared_ptr<GpuMesh> {
      if ( auto it = _meshes.find( path ); it != _meshes.end() ) return it->second;
      auto bytes = assets::read( path );
      auto m = bytes ? mesh::decode( *bytes ) : std::nullopt;
      if ( !m ) {
        std::fprintf( stderr, "bicycle 3D: can't load mesh %s (build the 'assets' target: meshes are packed)\n", path.c_str() );
        return nullptr;
      }
      // Expand to one vertex per corner (normals and uvs are per corner), grouped by material.
      auto g = std::make_shared<GpuMesh>();
      std::vector<float> verts;
      const size_t nMat = std::max<size_t>( 1, m->materials.size() );
      for ( size_t mat = 0; mat < nMat; ++mat ) {
        GpuMesh::Range r;
        r.first = static_cast<int>( verts.size() / 8 );
        for ( size_t t = 0; t < m->nTris(); ++t ) {
          if ( ( m->triMat.empty() ? 0 : m->triMat[t] ) != mat ) continue;
          for ( int c = 0; c < 3; ++c ) {
            const size_t corner = 3 * t + c, v = m->tris[corner];
            verts.insert( verts.end(), { m->pos[3 * v], m->pos[3 * v + 1], m->pos[3 * v + 2] } );
            if ( m->normals.empty() ) verts.insert( verts.end(), { 0, 1, 0 } );
            else verts.insert( verts.end(), { m->normals[3 * corner], m->normals[3 * corner + 1], m->normals[3 * corner + 2] } );
            if ( m->uvs.empty() ) verts.insert( verts.end(), { 0, 0 } );
            else verts.insert( verts.end(), { m->uvs[2 * corner], m->uvs[2 * corner + 1] } );
          }
        }
        r.count = static_cast<int>( verts.size() / 8 ) - r.first;
        if ( mat < m->materials.size() ) {
          const auto& mm = m->materials[mat];
          r.rgba = mm.rgba;
          if ( !mm.texture.empty() ) {
            auto tb = assets::read( "./" + _gameName + "/" + mm.texture );
            auto ti = tb ? BImg::parse( *tb ) : std::nullopt;
            if ( ti ) r.tex = texture( *ti );
          }
        }
        else {
          r.rgba = { 0.8f, 0.8f, 0.8f, 1.0f };
        }
        if ( r.count > 0 ) g->ranges.push_back( r );
      }
      gl.GenVertexArrays( 1, &g->vao );
      gl.GenBuffers( 1, &g->vbo );
      gl.BindVertexArray( g->vao );
      gl.BindBuffer( GL_ARRAY_BUFFER, g->vbo );
      gl.BufferData( GL_ARRAY_BUFFER, verts.size() * sizeof( float ), verts.data(), GL_STATIC_DRAW );
      vertexLayout();
      _meshes[path] = g;
      return g;
    }

    void drawMesh( const GpuMesh& m, const Mat4& viewProj, const Mat4& model ) {
      const Mat4 mvp = mul( viewProj, model );
      gl.UniformMatrix4fv( gl.GetUniformLocation( _prog, "uMvp" ), 1, 0, mvp.data() );
      gl.UniformMatrix4fv( gl.GetUniformLocation( _prog, "uModel" ), 1, 0, model.data() );
      gl.Uniform1i( gl.GetUniformLocation( _prog, "uLit" ), 1 );
      gl.BindVertexArray( m.vao );
      for ( const auto& r : m.ranges ) {
        gl.Uniform4fv( gl.GetUniformLocation( _prog, "uColor" ), 1, r.rgba.data() );
        gl.Uniform1i( gl.GetUniformLocation( _prog, "uTex" ), r.tex ? 1 : 0 );
        gl.BindTexture( GL_TEXTURE_2D, r.tex );
        gl.DrawArrays( GL_TRIANGLES, r.first, r.count );
      }
    }

    void drawWorld() {
      constexpr float CELLS_PER_UNIT_X = 16.0f, ROWS_PER_UNIT_Y = 8.0f;
      const float camW = static_cast<float>( _cam.size.w ) / ( 1 << FIXEDPT_DEC_BITS );
      const float camCx = ( static_cast<float>( _cam.pos.x ) / ( 1 << FIXEDPT_DEC_BITS ) + camW / 2 ) / CELLS_PER_UNIT_X;
      // Viewport: the scene window's rectangle (GL's origin is bottom left).
      const int vx = _sceneWin->x * CELL_W, vw = _sceneWin->w * CELL_W, vh = _sceneWin->h * CELL_H;
      const int vy = _fbH - ( _sceneWin->y * CELL_H + vh );
      gl.Viewport( vx, vy, vw, vh );
      const float eye[3] = { camCx, 3.2f, 8.0f }, at[3] = { camCx, 0.7f, 0.0f };
      const Mat4 viewProj = mul( perspective( 0.85f, static_cast<float>( vw ) / std::max( 1, vh ), 0.1f, 200.0f ), lookAt( eye, at ) );

      gl.Enable( GL_DEPTH_TEST );
      gl.DepthFunc( GL_LEQUAL );
      gl.Disable( GL_BLEND );
      gl.UseProgram( _prog );
      gl.ActiveTexture( GL_TEXTURE0 );
      gl.Uniform1i( gl.GetUniformLocation( _prog, "tex" ), 0 );
      for ( const auto& s : _statics ) drawMesh( *s, viewProj, identity() );

      float ground = 0;  // lowest entity bottom in view = ground level
      for ( const auto& b : _drawn ) ground = std::max( ground, b.bottom );
      ++_tick;
      for ( const auto& b : _drawn ) {
        const float x = b.x / CELLS_PER_UNIT_X;
        const float y = std::max( 0.0f, ground - b.bottom ) / ROWS_PER_UNIT_Y;
        if ( auto it = _models.find( b.entity ); it != _models.end() ) {
          // Face the way it last moved; bob while walking.
          auto& st = _motion[b.entity];
          if ( st.seen && x < st.x - 1e-4f ) st.facing = 3.14159265f;
          if ( st.seen && x > st.x + 1e-4f ) st.facing = 0.0f;
          const bool moving = st.seen && std::fabs( x - st.x ) > 1e-4f;
          st.walk = moving ? st.walk + 1 : 0;
          st.x = x;
          st.seen = true;
          const float bob = moving ? 0.06f * std::fabs( std::sin( st.walk * 0.5f ) ) : 0.0f;
          drawMesh( *it->second, viewProj, mul( translate( x, y + bob, 0.3f ), rotateY( st.facing ) ) );
          continue;
        }
        if ( !b.pixels ) continue;  // text art has no 3D form
        // A billboard of the current frame: one image pixel = 1/8 unit; src rect cells -> pixels (2x1 cells each).
        const BImg& img = *b.pixels;
        const float u0 = ( b.src.x / 2.0f ) / img.w, u1 = ( ( b.src.x + b.src.w ) / 2.0f ) / img.w;
        const float t0 = static_cast<float>( b.src.y ) / img.h, t1 = static_cast<float>( b.src.y + b.src.h ) / img.h;
        const float w = b.w / 2 / ROWS_PER_UNIT_Y, h = b.h / ROWS_PER_UNIT_Y, z = -0.8f;
        // uv v points up in the shader's convention: v = 1 - row fraction
        const float quad[] = {
          x - w / 2, y,     z, 0, 0, 1, u0, 1 - t1,
          x + w / 2, y,     z, 0, 0, 1, u1, 1 - t1,
          x - w / 2, y + h, z, 0, 0, 1, u0, 1 - t0,
          x + w / 2, y + h, z, 0, 0, 1, u1, 1 - t0,
        };
        gl.BindVertexArray( _quadVao );
        gl.BindBuffer( GL_ARRAY_BUFFER, _quadVbo );
        gl.BufferData( GL_ARRAY_BUFFER, sizeof quad, quad, GL_DYNAMIC_DRAW );
        const Mat4 id = identity();
        gl.UniformMatrix4fv( gl.GetUniformLocation( _prog, "uMvp" ), 1, 0, viewProj.data() );
        gl.UniformMatrix4fv( gl.GetUniformLocation( _prog, "uModel" ), 1, 0, id.data() );
        const float white[4] = { 1, 1, 1, 1 };
        gl.Uniform4fv( gl.GetUniformLocation( _prog, "uColor" ), 1, white );
        gl.Uniform1i( gl.GetUniformLocation( _prog, "uTex" ), 1 );
        gl.Uniform1i( gl.GetUniformLocation( _prog, "uLit" ), 0 );
        gl.BindTexture( GL_TEXTURE_2D, spriteTexture( b.pixels ) );
        gl.DrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
      }
      gl.Viewport( 0, 0, _fbW, _fbH );
    }

    void drawOverlay() {
      gl.Disable( GL_DEPTH_TEST );
      gl.Enable( GL_BLEND );
      gl.BlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
      gl.UseProgram( _overlayProg );
      gl.ActiveTexture( GL_TEXTURE0 );
      gl.BindTexture( GL_TEXTURE_2D, _overlayTex );
      gl.PixelStorei( GL_UNPACK_ALIGNMENT, 4 );
      gl.TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, _fbW, _fbH, 0, GL_BGRA, GL_UNSIGNED_BYTE, _fb.data() );  // ARGB words
      gl.Uniform1i( gl.GetUniformLocation( _overlayProg, "tex" ), 0 );
      gl.BindVertexArray( _overlayVao );
      gl.DrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    }

    void readBack() {
      std::vector<uint32_t> px( static_cast<size_t>( _fbW ) * _fbH );
      gl.PixelStorei( GL_PACK_ALIGNMENT, 4 );
      gl.ReadPixels( 0, 0, _fbW, _fbH, GL_BGRA, GL_UNSIGNED_BYTE, px.data() );
      _capture.resize( px.size() );
      for ( int y = 0; y < _fbH; ++y )  // GL rows are bottom first
        for ( int x = 0; x < _fbW; ++x ) _capture[static_cast<size_t>( y ) * _fbW + x] = px[static_cast<size_t>( _fbH - 1 - y ) * _fbW + x] | 0xFF000000u;
    }

    static GL gl;
    SDL_GLContext _ctx{};
    GLuint _prog{}, _overlayProg{}, _quadVao{}, _quadVbo{}, _overlayVao{}, _overlayVbo{}, _overlayTex{};
    Box _cam{};
    bool _haveScene{};
    Win* _sceneWin{};
    std::vector<Billboard> _drawn;
    std::map<std::string, std::shared_ptr<GpuMesh>> _meshes;
    std::unordered_map<Entity, std::shared_ptr<GpuMesh>> _models;
    std::vector<std::shared_ptr<GpuMesh>> _statics;
    std::map<const BImg*, std::pair<std::shared_ptr<const BImg>, GLuint>> _spriteTex;
    struct Motion { float x{}; float facing{}; int walk{}; bool seen{}; };
    std::unordered_map<Entity, Motion> _motion;
    std::vector<uint32_t> _capture;
    long _tick{};
};

GL Renderer3D::gl;

auto renderer() -> Renderer& {
  static Renderer3D r;
  return r;
}

}  // namespace gfx
