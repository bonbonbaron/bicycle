// 3D mode: OpenGL 3.3 core through SDL3. The scene's camera box picks what the 3D camera follows; entities with a
// model (setModel, the Lua "model" gene) are drawn as meshes (.bmesh, Edgebreaker-compressed, from Blender via
// scripts/blender/export.py), other entities as camera-facing billboards of their current sprite frame, and static
// scenery (addStaticModel) around them. Windows, the HUD and dialogue are PixelRenderer's 2D framebuffer composited
// over the 3D world as an overlay.
//
// Cells -> world: X = cells / 16 (one 8-pixel-wide sprite = 1 unit), Y = height above the ground in pixel rows / 8,
// Z = 0 for models, slightly behind for billboards. The ground row is the lowest entity bottom in view.
#include "v/sdl/PixelRenderer.h"
#include "v/common/Image.h"
#include "data/Mesh.h"
#include "data/Obj.h"
#include "data/Skin.h"
#include "data/BImg.h"
#include "data/Pack.h"
#include "m/World.h"
#include "m/Box.h"
#include "m/Rect.h"
#include "Constants.h"
#include "c/Physics3D.h"
#include "m/Math3D.h"
#include <SDL3/SDL.h>
#include <array>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include "m/EntitySlots.h"

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
  GL_RENDERER = 0x1F01, GL_FRAMEBUFFER = 0x8D40, GL_DEPTH_ATTACHMENT = 0x8D00, GL_DEPTH_COMPONENT = 0x1902,
  GL_DEPTH_COMPONENT24 = 0x81A6, GL_NONE = 0, GL_POLYGON_OFFSET_FILL = 0x8037, GL_LINEAR = 0x2601,
  GL_TEXTURE_COMPARE_MODE = 0x884C, GL_COMPARE_REF_TO_TEXTURE = 0x884E, GL_TEXTURE_COMPARE_FUNC = 0x884D,
  GL_CLAMP_TO_BORDER = 0x812D, GL_TEXTURE_BORDER_COLOR = 0x1004, GL_TEXTURE1 = 0x84C1, GL_FRAMEBUFFER_COMPLETE = 0x8CD5,
  GL_CULL_FACE = 0x0B44, GL_BACK = 0x0405,
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
  X( void, DeleteVertexArrays, GLsizei, const GLuint* ) \
  X( void, DeleteTextures, GLsizei, const GLuint* ) \
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
  X( const unsigned char*, GetString, GLenum ) \
  X( void, GenFramebuffers, GLsizei, GLuint* ) \
  X( void, BindFramebuffer, GLenum, GLuint ) \
  X( void, FramebufferTexture2D, GLenum, GLenum, GLenum, GLuint, GLint ) \
  X( GLenum, CheckFramebufferStatus, GLenum ) \
  X( void, DrawBuffer, GLenum ) \
  X( void, ReadBuffer, GLenum ) \
  X( void, PolygonOffset, GLfloat, GLfloat ) \
  X( void, TexParameterfv, GLenum, GLenum, const GLfloat* ) \
  X( void, Uniform3fv, GLint, GLsizei, const GLfloat* ) \
  X( void, Uniform1f, GLint, GLfloat ) \
  X( void, Uniform2fv, GLint, GLsizei, const GLfloat* )

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
layout(location = 3) in vec4 aJoints;   // skinned meshes: joint indices
layout(location = 4) in vec4 aWeights;  // and weights (sum 1)
uniform mat4 uMvp;
uniform mat4 uModel;
uniform int uSkinned;
uniform mat4 uBones[32];  // skin::MAX_JOINTS
uniform mat4 uLightVP;    // true 3D: world -> shadow map clip space
out vec3 vN;
out vec2 vUv;
out vec3 vWorld;
out vec4 vLight;
void main() {
  vec4 p = vec4(aPos, 1.0);
  vec3 n = aNrm;
  if (uSkinned == 1) {  // GPU skinning: blend the 4 joints' skinning matrices
    mat4 s = aWeights.x * uBones[int(aJoints.x)] + aWeights.y * uBones[int(aJoints.y)]
           + aWeights.z * uBones[int(aJoints.z)] + aWeights.w * uBones[int(aJoints.w)];
    p = s * p;
    n = mat3(s) * n;
  }
  gl_Position = uMvp * p;
  vN = mat3(uModel) * n;
  vUv = aUv;
  vec4 w = uModel * p;
  vWorld = w.xyz;
  vLight = uLightVP * w;
})";

const char* FS = R"(#version 330 core
in vec3 vN;
in vec2 vUv;
in vec3 vWorld;
in vec4 vLight;
uniform vec4 uColor;
uniform int uTex;
uniform int uLit;
uniform sampler2D tex;
// True 3D (uWorld3D == 1): directional light (uLightDir: the way it travels) with a 3x3 PCF shadow map, ambient,
// distance fog. Otherwise the 2.5D side view's fixed light.
uniform int uWorld3D;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uAmbient;
uniform int uShadows;
uniform sampler2DShadow uShadowMap;
uniform float uShadowTexel;
uniform vec3 uCamPos;
uniform vec3 uFogColor;
uniform vec2 uFog;  // near, far
out vec4 o;
float shadow(vec3 n) {
  vec3 p = vLight.xyz / vLight.w * 0.5 + 0.5;
  if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0 || p.z > 1.0) return 1.0;
  float bias = max(0.0025 * (1.0 - dot(n, -uLightDir)), 0.0008);
  float s = 0.0;
  for (int y = -1; y <= 1; ++y)
    for (int x = -1; x <= 1; ++x) s += texture(uShadowMap, vec3(p.xy + vec2(x, y) * uShadowTexel, p.z - bias));
  return s / 9.0;
}
void main() {
  vec4 c = uColor;
  if (uTex == 1) c *= texture(tex, vec2(vUv.x, 1.0 - vUv.y));  // uv v points up (OBJ); images are stored top row first
  if (c.a < 0.5) discard;
  if (uWorld3D == 0) {
    float l = 1.0;
    if (uLit == 1) l = 0.45 + 0.55 * max(dot(normalize(vN), normalize(vec3(0.4, 0.8, 0.5))), 0.0);
    o = vec4(c.rgb * l, c.a);
    return;
  }
  vec3 rgb = c.rgb;
  float sh = uShadows == 1 ? shadow(normalize(vN)) : 1.0;
  if (uLit == 1) {
    vec3 n = normalize(vN);
    if (!gl_FrontFacing) n = -n;
    float d = max(dot(n, -uLightDir), 0.0);
    rgb = c.rgb * (uAmbient + (1.0 - uAmbient) * d * sh * uLightColor);
  }
  else {
    rgb = c.rgb * mix(uAmbient + 0.25, 1.0, sh);  // billboards: shadowed but unlit
  }
  float f = clamp((length(vWorld - uCamPos) - uFog.x) / max(uFog.y - uFog.x, 0.001), 0.0, 1.0);
  o = vec4(mix(rgb, uFogColor, f), c.a);
})";

const char* DEPTH_FS = R"(#version 330 core
in vec2 vUv;
uniform vec4 uColor;
uniform int uTex;
uniform sampler2D tex;
void main() {
  if (uTex == 1 && texture(tex, vec2(vUv.x, 1.0 - vUv.y)).a < 0.5) discard;
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
  std::shared_ptr<const Skin> skin;  // skinned meshes: skeleton + actions (.bskin)
  struct Range { int first{}, count{}; std::array<float, 4> rgba{}; GLuint tex{}; };
  std::vector<Range> ranges;  // one per material
  m3::Vec3 center{};          // bounding sphere (model space), for frustum culling
  float radius{};
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

    // 3D UI: drawn into the overlay (composited over the GL world with alpha), soft and glossy to sit over a lit
    // scene: translucent gradient panels with a soft shadow and a light rim, a pill-shaped bar with a gradient fill
    // colored by fraction (green -> amber -> red) and a glass highlight.
    void drawBar( WinHandle win, int y, int x, const BarSpec& bar ) override {
      const Win* w = static_cast<Win*>( win );
      if ( !w || ( w->attr & ATTR_INVISIBLE ) ) return;
      int px = ( w->x + x ) * CELL_W + 6;
      const int top = ( w->y + y ) * CELL_H + 2;
      const float frac = std::clamp( static_cast<float>( bar.curr ) / std::max( 1, bar.max ), 0.0f, 1.0f );
      const int bw = std::max( 24, bar.len * CELL_W * 2 ), bh = 16;
      const int labelW = bar.label.empty() ? 0 : measureString( bar.label, UI_TEXT_PX, true ) + 10;
      const std::string nums = std::to_string( bar.curr ) + "/" + std::to_string( bar.max );
      const int numsW = measureString( nums, UI_TEXT_PX ) + 10;
      // A translucent plate behind the whole meter so it reads over any background.
      gradientRoundRect( nullptr, px - 6, top - 4, labelW + bw + numsW + 12, bh + 8, 8, 0xA0202838u, 0xA0101420u );
      if ( !bar.label.empty() ) {
        drawString( w, px, top, bar.label, 0xFFFFFFFFu, UI_TEXT_PX, true );
        px += labelW;
      }
      if ( bar.frameSprite || bar.fillSprite ) {
        if ( bar.fillSprite ) drawSprite( w, *bar.fillSprite, px, top, bw, bh, frac );
        if ( bar.frameSprite ) drawSprite( w, *bar.frameSprite, px, top, bw, bh );
      }
      else {
        const float r = bh / 2.0f;
        gradientRoundRect( w, px, top, bw, bh, bh / 2, 0xFF0A0C12u, 0xFF262A36u );  // recessed track
        const uint32_t c0 = frac > 0.5f ? 0xFF7CF07Cu : frac > 0.25f ? 0xFFFFD860u : 0xFFFF7A60u;
        const uint32_t c1 = frac > 0.5f ? 0xFF1E8E3Au : frac > 0.25f ? 0xFFC07810u : 0xFFB01E1Eu;
        const int fw = bar.curr > 0 ? std::max( bh, static_cast<int>( bw * frac + 0.5f ) ) : 0;
        if ( fw > 0 ) {
          gradientRoundRect( w, px + 1, top + 1, fw - 2, bh - 2, static_cast<int>( r ) - 1, c0, c1 );
          gradientRoundRect( w, px + 4, top + 2, std::max( 0, fw - 8 ), bh / 2 - 2, bh / 4, 0x90FFFFFFu, 0x10FFFFFFu );  // glass
        }
        roundRect( w, px, top, bw, bh, bh / 2, 0xFFD8E4FFu, 1 );  // rim
      }
      px += bw + 10;
      drawString( w, px, top, nums, 0xFFFFFFFFu, UI_TEXT_PX );
    }

    void drawMenuCursor( WinHandle win, int y, int x, const std::string& ) override {
      const Win* w = static_cast<Win*>( win );
      if ( !w || ( w->attr & ATTR_INVISIBLE ) ) return;
      // An anti-aliased glowing diamond.
      const float cx = ( w->x + x ) * CELL_W + 10.5f, cy = ( w->y + y ) * CELL_H + 8.0f;
      for ( int yy = static_cast<int>( cy ) - 9; yy <= static_cast<int>( cy ) + 9; ++yy ) {
        for ( int xx = static_cast<int>( cx ) - 9; xx <= static_cast<int>( cx ) + 9; ++xx ) {
          const float d = std::abs( xx + 0.5f - cx ) + std::abs( yy + 0.5f - cy );
          if ( d < 9 ) blendPixel( xx, yy, 0xFF80B0FFu, static_cast<unsigned>( std::clamp( ( 9 - d ) * 18.0f, 0.0f, 90.0f ) ) );
          if ( d < 5.5f ) blendPixel( xx, yy, 0xFFF0F6FFu, static_cast<unsigned>( std::clamp( ( 5.5f - d ) * 255.0f, 0.0f, 255.0f ) ) );
        }
      }
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
      if ( m ) _models.set( entity, m );
      return m != nullptr;
    }

    // anim:set(tag) -> the model's action (Genome.lua maps the 2D strip tag to an action name).
    void setModelAction( Entity entity, const std::string& action ) override {
      auto& st = _motion[entity];
      if ( st.action != action ) {
        st.action = action;
        st.since = now();
      }
    }

    auto addStaticModel( const std::string& path ) -> bool override {
      auto m = loadMesh( path );
      if ( m ) _statics.push_back( m );
      return m != nullptr;
    }

    void forgetEntity( Entity entity ) override {
      _models.erase( entity );
      _motion.erase( entity );
    }

    // The old scene's scenery and any mesh no model uses any more (its GL objects and its SCENE-pool skin) go.
    void onSceneUnloaded() override {
      PixelRenderer::onSceneUnloaded();
      _statics.clear();
      _models.pruneStale();  // ids freed without forgetEntity: let their meshes go too
      _motion.pruneStale();
      for ( auto it = _meshes.begin(); it != _meshes.end(); ) {
        if ( it->second.use_count() > 1 ) {
          ++it;
          continue;
        }
        GpuMesh& g = *it->second;
        for ( const auto& r : g.ranges ) {
          if ( r.tex ) gl.DeleteTextures( 1, &r.tex );
        }
        if ( g.vbo ) gl.DeleteBuffers( 1, &g.vbo );
        if ( g.vao ) gl.DeleteVertexArrays( 1, &g.vao );
        it = _meshes.erase( it );
      }
    }

  protected:
    void drawPanelBody( const Win& w ) override {
      const int x = winPx( w ), y = winPy( w ), pw = w.w * CELL_W, ph = w.h * CELL_H;
      for ( int i = 8; i >= 1; i -= 1 ) {  // soft shadow
        roundRect( nullptr, x - i + 6, y - i + 8, pw + 2 * i, ph + 2 * i, 14 + i, 0x0C000000u );
      }
      gradientRoundRect( nullptr, x, y, pw, ph, 12, 0xE6303C5Cu, 0xE6141A2Cu );
    }
    void drawPanelFrame( const Win& w ) override {
      const int x = winPx( w ), y = winPy( w ), pw = w.w * CELL_W, ph = w.h * CELL_H;
      roundRect( nullptr, x, y, pw, ph, 12, 0xFFA8C0F0u, 2 );
      roundRect( nullptr, x + 3, y + 3, pw - 6, ph - 6, 9, 0x40FFFFFFu, 1 );
    }
    void drawRowHighlight( const Win& w, int px, int py, int pw, int ph ) override {
      gradientRoundRect( &w, px - 2, py, pw + 4, ph, ph / 2, 0xC05A86E0u, 0xC0304E9Au );
    }

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
      _depthProg = program( VS, DEPTH_FS );
      // The color texture is on unit 0 and the shadow map on unit 1, always (two sampler types may not share a unit).
      gl.UseProgram( _prog );
      gl.Uniform1i( gl.GetUniformLocation( _prog, "tex" ), 0 );
      gl.Uniform1i( gl.GetUniformLocation( _prog, "uShadowMap" ), 1 );
      // Shadow map: a depth texture compared in the shader (sampler2DShadow, hardware PCF where available).
      gl.GenTextures( 1, &_shadowTex );
      gl.BindTexture( GL_TEXTURE_2D, _shadowTex );
      gl.TexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_SIZE, SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
      const float border[4] = { 1, 1, 1, 1 };
      gl.TexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
      gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
      gl.GenFramebuffers( 1, &_shadowFbo );
      gl.BindFramebuffer( GL_FRAMEBUFFER, _shadowFbo );
      gl.FramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowTex, 0 );
      gl.DrawBuffer( GL_NONE );
      gl.ReadBuffer( GL_NONE );
      _shadowOk = gl.CheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE;
      gl.BindFramebuffer( GL_FRAMEBUFFER, 0 );
      if ( !_shadowOk ) std::fprintf( stderr, "bicycle 3D: no shadow map framebuffer; shadows off\n" );
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
      auto& w3 = phys3d::World3D::get();
      const m3::Vec3 sky = w3.active() ? w3.view().sky : m3::Vec3{ 0.45f, 0.65f, 0.90f };
      gl.ClearColor( sky.x, sky.y, sky.z, 1.0f );
      gl.Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
      if ( _haveScene ) {
        if ( w3.active() ) drawWorld3D();
        else drawWorld();
      }
      drawOverlay();
      if ( screenshotDue() ) readBack();
      SDL_GL_SwapWindow( _window );
      _drawn.clear();
      _haveScene = false;
    }

    void captureFrame( gfx::Pixels& out ) override { out = _capture; }

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

    // Meshes: pos3 nrm3 uv2 joints4 weights4.
    static void meshLayout() {
      constexpr GLsizei S = 16 * sizeof( float );
      for ( GLuint a = 0; a < 5; ++a ) {
        static const int n[5] = { 3, 3, 2, 4, 4 }, off[5] = { 0, 3, 6, 8, 12 };
        gl.VertexAttribPointer( a, n[a], GL_FLOAT, 0, S, reinterpret_cast<void*>( off[a] * sizeof( float ) ) );
        gl.EnableVertexAttribArray( a );
      }
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
      gfx::Pixels rgba( static_cast<size_t>( img.w ) * img.h );
      for ( int y = 0; y < img.h; ++y )
        for ( int x = 0; x < img.w; ++x ) rgba[static_cast<size_t>( y ) * img.w + x] = img.pixel( x, y );
      GLuint t{};
      gl.GenTextures( 1, &t );
      texParams( t, GL_REPEAT );
      gl.PixelStorei( GL_UNPACK_ALIGNMENT, 1 );
      gl.TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, img.w, img.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data() );
      return t;
    }

    // Dev builds load meshes straight from the exporter's OBJ (+ .mtl), uncompressed; texture names are mapped to
    // the .bimg next to the OBJ, as the packer does.
    auto loadLooseObj( const std::string& bmeshPath ) -> std::optional<MeshData> {
      if ( bmeshPath.size() < 6 || bmeshPath.compare( bmeshPath.size() - 6, 6, ".bmesh" ) != 0 ) return std::nullopt;
      const std::string obj = bmeshPath.substr( 0, bmeshPath.size() - 6 ) + ".obj";
      MeshData m;
      std::string err;
      if ( !readObj( obj, m, err ) ) return std::nullopt;
      const std::string gameDir = "./" + _gameName + "/";
      std::string dir = obj.substr( 0, obj.find_last_of( '/' ) + 1 );
      if ( dir.rfind( gameDir, 0 ) == 0 ) dir = dir.substr( gameDir.size() );
      for ( auto& mat : m.materials ) {
        if ( mat.texture.empty() ) continue;
        std::string base = mat.texture.substr( mat.texture.find_last_of( "/\\" ) + 1 );
        base = base.substr( 0, base.find_last_of( '.' ) ) + ".bimg";
        mat.texture = assets::exists( gameDir + dir + base ) ? dir + base : "";
      }
      return m;
    }

    auto loadMesh( const std::string& path ) -> std::shared_ptr<GpuMesh> {
      if ( auto it = _meshes.find( path ); it != _meshes.end() ) return it->second;
      std::optional<MeshData> m;
      {
        auto bytes = assets::read( path, mem::TEMPORARY );  // packed .bmesh (release builds)
        if ( bytes ) m = mesh::decode( *bytes );
      }
      if ( !m ) m = loadLooseObj( path );  // dev builds: the .obj the Blender exporter wrote
      if ( !m ) {
        std::fprintf( stderr, "bicycle 3D: can't load mesh %s (no .bmesh in the pack and no loose .obj)\n", path.c_str() );
        return nullptr;
      }
      // Skinned? The .bskin next to it: in the pack (quantized) or the exporter's loose float file. Packed meshes
      // carry their joints/weights; a loose OBJ gets them from the .bskin's per-OBJ-vertex table.
      auto g = std::make_shared<GpuMesh>();
      if ( path.size() > 6 ) {
        auto sb = assets::read( path.substr( 0, path.size() - 6 ) + ".bskin", mem::TEMPORARY );
        auto sk = sb ? skin::decode( *sb ) : std::nullopt;
        if ( sk ) {
          if ( m->joints.empty() && !applySkinVertices( *m, *sk ) ) {
            std::fprintf( stderr, "bicycle 3D: %s's .bskin doesn't match its vertices\n", path.c_str() );
          }
          sk->vJoints.clear();
          sk->vWeights.clear();
          g->skin = std::make_shared<const Skin>( std::move( *sk ) );
        }
      }
      const bool skinned = g->skin && m->joints.size() == 4 * m->nVerts();
      if ( !skinned ) g->skin = nullptr;
      // Expand to one vertex per corner (normals and uvs are per corner), grouped by material.
      mem::Vector<float, mem::GRAPHIC> verts;
      const size_t nMat = std::max<size_t>( 1, m->materials.size() );
      for ( size_t mat = 0; mat < nMat; ++mat ) {
        GpuMesh::Range r;
        r.first = static_cast<int>( verts.size() / 16 );
        for ( size_t t = 0; t < m->nTris(); ++t ) {
          if ( ( m->triMat.empty() ? 0 : m->triMat[t] ) != mat ) continue;
          for ( int c = 0; c < 3; ++c ) {
            const size_t corner = 3 * t + c, v = m->tris[corner];
            verts.insert( verts.end(), { m->pos[3 * v], m->pos[3 * v + 1], m->pos[3 * v + 2] } );
            if ( m->normals.empty() ) verts.insert( verts.end(), { 0, 1, 0 } );
            else verts.insert( verts.end(), { m->normals[3 * corner], m->normals[3 * corner + 1], m->normals[3 * corner + 2] } );
            if ( m->uvs.empty() ) verts.insert( verts.end(), { 0, 0 } );
            else verts.insert( verts.end(), { m->uvs[2 * corner], m->uvs[2 * corner + 1] } );
            if ( skinned ) {
              for ( int k = 0; k < 4; ++k ) verts.push_back( m->joints[4 * v + k] );
              for ( int k = 0; k < 4; ++k ) verts.push_back( m->weights[4 * v + k] / 255.0f );
            }
            else {
              verts.insert( verts.end(), { 0, 0, 0, 0, 1, 0, 0, 0 } );
            }
          }
        }
        r.count = static_cast<int>( verts.size() / 16 ) - r.first;
        if ( mat < m->materials.size() ) {
          const auto& mm = m->materials[mat];
          r.rgba = mm.rgba;
          if ( !mm.texture.empty() ) {
            auto tb = assets::read( "./" + _gameName + "/" + mm.texture, mem::TEMPORARY );
            auto ti = tb ? BImg::parse( *tb ) : std::nullopt;
            if ( ti ) r.tex = texture( *ti );
          }
        }
        else {
          r.rgba = { 0.8f, 0.8f, 0.8f, 1.0f };
        }
        if ( r.count > 0 ) g->ranges.push_back( r );
      }
      {
        m3::Vec3 lo{ 1e30f, 1e30f, 1e30f }, hi{ -1e30f, -1e30f, -1e30f };
        for ( size_t v = 0; v < m->nVerts(); ++v ) {
          const m3::Vec3 p{ m->pos[3 * v], m->pos[3 * v + 1], m->pos[3 * v + 2] };
          lo = { std::min( lo.x, p.x ), std::min( lo.y, p.y ), std::min( lo.z, p.z ) };
          hi = { std::max( hi.x, p.x ), std::max( hi.y, p.y ), std::max( hi.z, p.z ) };
        }
        g->center = ( lo + hi ) * 0.5f;
        g->radius = m3::length( hi - lo ) * 0.5f;
      }
      gl.GenVertexArrays( 1, &g->vao );
      gl.GenBuffers( 1, &g->vbo );
      gl.BindVertexArray( g->vao );
      gl.BindBuffer( GL_ARRAY_BUFFER, g->vbo );
      gl.BufferData( GL_ARRAY_BUFFER, verts.size() * sizeof( float ), verts.data(), GL_STATIC_DRAW );
      meshLayout();
      _meshes[path] = g;
      return g;
    }

    void drawMesh( const GpuMesh& m, const Mat4& viewProj, const Mat4& model, const float* bones = nullptr, GLuint prog = 0 ) {
      if ( !prog ) prog = _prog;
      const Mat4 mvp = mul( viewProj, model );
      gl.Uniform1i( gl.GetUniformLocation( prog, "uSkinned" ), bones && m.skin ? 1 : 0 );
      if ( bones && m.skin ) {
        gl.UniformMatrix4fv( gl.GetUniformLocation( prog, "uBones" ), static_cast<GLsizei>( m.skin->joints.size() ), 0, bones );
      }
      gl.UniformMatrix4fv( gl.GetUniformLocation( prog, "uMvp" ), 1, 0, mvp.data() );
      gl.UniformMatrix4fv( gl.GetUniformLocation( prog, "uModel" ), 1, 0, model.data() );
      gl.Uniform1i( gl.GetUniformLocation( prog, "uLit" ), 1 );
      gl.BindVertexArray( m.vao );
      for ( const auto& r : m.ranges ) {
        gl.Uniform4fv( gl.GetUniformLocation( prog, "uColor" ), 1, r.rgba.data() );
        gl.Uniform1i( gl.GetUniformLocation( prog, "uTex" ), r.tex ? 1 : 0 );
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
      gl.Uniform1i( gl.GetUniformLocation( _prog, "uWorld3D" ), 0 );
      gl.ActiveTexture( GL_TEXTURE0 );
      gl.Uniform1i( gl.GetUniformLocation( _prog, "tex" ), 0 );
      for ( const auto& s : _statics ) drawMesh( *s, viewProj, identity() );

      float ground = 0;  // lowest entity bottom in view = ground level
      for ( const auto& b : _drawn ) ground = std::max( ground, b.bottom );
      ++_tick;
      for ( const auto& b : _drawn ) {
        const float x = b.x / CELLS_PER_UNIT_X;
        const float y = std::max( 0.0f, ground - b.bottom ) / ROWS_PER_UNIT_Y;
        if ( auto* model = _models.find( b.entity ) ) {
          // Face the way it last moved; bob while walking.
          auto& st = _motion[b.entity];
          if ( st.seen && x < st.x - 1e-4f ) st.facing = 3.14159265f;
          if ( st.seen && x > st.x + 1e-4f ) st.facing = 0.0f;
          const bool moving = st.seen && std::fabs( x - st.x ) > 1e-4f;
          st.walk = moving ? st.walk + 1 : 0;
          st.x = x;
          st.seen = true;
          st.still = moving ? 0 : st.still + 1;
          const GpuMesh& gm = **model;
          if ( gm.skin ) {
            // Skeletal animation: the action anim:set() chose (setModelAction). "walk" plays while it actually
            // moves; after ~0.3 s standing still it shows the idle action instead (the 2D strips have no idle).
            std::string act = st.action;
            if ( act == "walk" && st.still > 9 ) act = "idle";
            static const char* forced = std::getenv( "BICYCLE_MODEL_ACTION" );  // screenshots/tests: pin an action
            if ( forced && *forced ) act = forced;
            const SkinAction* a = gm.skin->action( act );
            if ( !a && !gm.skin->actions.empty() ) a = &gm.skin->actions.front();
            float bones[16 * skin::MAX_JOINTS];
            if ( a ) skin::pose( *gm.skin, *a, static_cast<float>( now() - st.since ), bones );
            else skin::restPose( *gm.skin, bones );
            drawMesh( gm, viewProj, mul( translate( x, y, 0.3f ), rotateY( st.facing ) ), bones );
            ++_stats.skinnedDraws;
          }
          else {
            const float bob = moving ? 0.06f * std::fabs( std::sin( st.walk * 0.5f ) ) : 0.0f;  // unrigged: bob
            drawMesh( gm, viewProj, mul( translate( x, y + bob, 0.3f ), rotateY( st.facing ) ) );
          }
          continue;
        }
        if ( !b.pixels ) continue;  // text art has no 3D form
        // A billboard of the current frame: one image pixel = 1/8 unit; src rect cells -> pixels (2x1 cells each),
        // offset to the image's place in the sprite atlas. Quads are batched into one draw below.
        const auto at = _atlas.entry( b.pixels );
        const float aw = static_cast<float>( _atlas.width() ), ah = static_cast<float>( _atlas.height() );
        const float u0 = ( at.x + b.src.x / 2.0f ) / aw, u1 = ( at.x + ( b.src.x + b.src.w ) / 2.0f ) / aw;
        const float t0 = ( at.y + static_cast<float>( b.src.y ) ) / ah, t1 = ( at.y + static_cast<float>( b.src.y + b.src.h ) ) / ah;
        const float w = b.w / 2 / ROWS_PER_UNIT_Y, h = b.h / ROWS_PER_UNIT_Y, z = -0.8f;
        // uv v points up in the shader's convention: v = 1 - row fraction
        const float quad[] = {
          x - w / 2, y,     z, 0, 0, 1, u0, 1 - t1,
          x + w / 2, y,     z, 0, 0, 1, u1, 1 - t1,
          x - w / 2, y + h, z, 0, 0, 1, u0, 1 - t0,
          x + w / 2, y,     z, 0, 0, 1, u1, 1 - t1,
          x + w / 2, y + h, z, 0, 0, 1, u1, 1 - t0,
          x - w / 2, y + h, z, 0, 0, 1, u0, 1 - t0,
        };
        _bbVerts.insert( _bbVerts.end(), std::begin( quad ), std::end( quad ) );
      }
      // Every billboard in one draw with the atlas bound (uploaded again only when it was re-planned).
      if ( !_bbVerts.empty() ) {
        if ( _atlasTexVersion != _atlas.version() ) {
          if ( !_atlasTex ) gl.GenTextures( 1, &_atlasTex );
          texParams( _atlasTex, GL_CLAMP_TO_EDGE );
          gl.PixelStorei( GL_UNPACK_ALIGNMENT, 4 );
          gl.TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, _atlas.width(), _atlas.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, _atlas.pixels().data() );
          _atlasTexVersion = _atlas.version();
        }
        gl.BindVertexArray( _quadVao );
        gl.BindBuffer( GL_ARRAY_BUFFER, _quadVbo );
        gl.BufferData( GL_ARRAY_BUFFER, _bbVerts.size() * sizeof( float ), _bbVerts.data(), GL_DYNAMIC_DRAW );
        const Mat4 id = identity();
        gl.UniformMatrix4fv( gl.GetUniformLocation( _prog, "uMvp" ), 1, 0, viewProj.data() );
        gl.UniformMatrix4fv( gl.GetUniformLocation( _prog, "uModel" ), 1, 0, id.data() );
        const float white[4] = { 1, 1, 1, 1 };
        gl.Uniform4fv( gl.GetUniformLocation( _prog, "uColor" ), 1, white );
        gl.Uniform1i( gl.GetUniformLocation( _prog, "uTex" ), 1 );
        gl.Uniform1i( gl.GetUniformLocation( _prog, "uLit" ), 0 );
        gl.Uniform1i( gl.GetUniformLocation( _prog, "uSkinned" ), 0 );
        gl.BindTexture( GL_TEXTURE_2D, _atlasTex );
        gl.DrawArrays( GL_TRIANGLES, 0, static_cast<GLsizei>( _bbVerts.size() / 8 ) );
        ++_stats.billboardDraws;
        _stats.billboards += _bbVerts.size() / 48;
        _bbVerts.clear();
      }
      gl.Viewport( 0, 0, _fbW, _fbH );
    }


    // --- true 3D (a camera rig is active: include/c/Physics3D.h) -------------------------------------------------------
    struct Item { const GpuMesh* mesh; m3::Mat4 model; bool skinned; std::array<float, 16 * skin::MAX_JOINTS>* bones; };

    auto modelMatrix( const phys3d::Transform& t ) -> m3::Mat4 {
      const m3::Quat q = t.useQuat ? t.rot * m3::axisAngle( { 0, 1, 0 }, t.modelYaw )
                                   : m3::axisAngle( { 0, 1, 0 }, t.yaw + t.modelYaw ) * m3::axisAngle( { 1, 0, 0 }, t.pitch ) * m3::axisAngle( { 0, 0, 1 }, t.roll );
      return m3::trs( t.pos, q, t.scale );
    }

    void setWorldUniforms( const phys3d::View& v, const m3::Vec3& eye, const m3::Mat4& lightVP, bool shadows ) {
      auto u = [&]( const char* n ) { return gl.GetUniformLocation( _prog, n ); };
      gl.Uniform1i( u( "uWorld3D" ), 1 );
      const m3::Vec3 ld = m3::normalize( v.lightDir );
      gl.Uniform3fv( u( "uLightDir" ), 1, &ld.x );
      gl.Uniform3fv( u( "uLightColor" ), 1, &v.lightColor.x );
      gl.Uniform1f( u( "uAmbient" ), v.ambient );
      gl.Uniform1i( u( "uShadows" ), shadows ? 1 : 0 );
      gl.Uniform1i( u( "uShadowMap" ), 1 );
      gl.Uniform1f( u( "uShadowTexel" ), 1.0f / SHADOW_SIZE );
      gl.Uniform3fv( u( "uCamPos" ), 1, &eye.x );
      gl.Uniform3fv( u( "uFogColor" ), 1, &v.fogColor.x );
      const float fog[2] = { v.fogNear, v.fogFar };
      gl.Uniform2fv( u( "uFog" ), 1, fog );
      gl.UniformMatrix4fv( u( "uLightVP" ), 1, 0, lightVP.data() );
    }

    void drawWorld3D() {
      auto& w3 = phys3d::World3D::get();
      const auto& v = w3.view();
      const auto& cam = w3.transform( w3.cameraEntity() );
      const int vw = _fbW, vh = _fbH;
      const m3::Vec3 eye = cam.pos, fwd = m3::forward( cam.yaw, cam.pitch );
      const m3::Mat4 view = m3::lookAt( eye, eye + fwd );
      const m3::Mat4 proj = m3::perspective( v.fovDeg * m3::PI / 180.f, static_cast<float>( vw ) / std::max( 1, vh ), v.nearZ, v.farZ );
      const m3::Mat4 viewProj = m3::mul( proj, view );
      const m3::Planes planes = m3::frustumPlanes( viewProj );
      _stats3d = {};
      ++_tick;

      // What to draw: the level's static meshes, then every visible 3D entity's model (posed if rigged).
      _items.clear();
      for ( const auto& st : _statics ) _items.push_back( { st.get(), m3::identity(), false, nullptr } );
      size_t nPosed = 0;
      _models.forEach( [&]( Entity entity, std::shared_ptr<GpuMesh>& mesh ) {
        const auto& t = w3.transform( entity );
        if ( !t.on || !t.visible ) return;
        Item it{ mesh.get(), modelMatrix( t ), false, nullptr };
        auto& st = _motion[entity];
        const float dx = t.pos.x - st.x, dz = t.pos.z - st.z3;
        const bool moving = st.seen && dx * dx + dz * dz > 1e-6f;
        st.x = t.pos.x;
        st.z3 = t.pos.z;
        st.seen = true;
        st.still = moving ? 0 : st.still + 1;
        if ( mesh->skin ) {
          std::string act = st.action;
          if ( act == "walk" && st.still > 3 ) act = "idle";
          if ( act == "idle" && moving ) act = "walk";
          static const char* forced = std::getenv( "BICYCLE_MODEL_ACTION" );
          if ( forced && *forced ) act = forced;
          if ( act != st.playing ) { st.playing = act; st.since = now(); }
          const SkinAction* a = mesh->skin->action( act );
          if ( !a && !mesh->skin->actions.empty() ) a = &mesh->skin->actions.front();
          if ( _poses.size() <= nPosed ) _poses.resize( nPosed + 1 );
          auto& bones = _poses[nPosed++];
          if ( a ) skin::pose( *mesh->skin, *a, static_cast<float>( now() - st.since ), bones.data() );
          else skin::restPose( *mesh->skin, bones.data() );
          it.skinned = true;
          it.bones = &bones;
        }
        _items.push_back( it );
      } );
      auto worldSphere = [&]( const Item& it, m3::Vec3& c, float& r ) {
        c = m3::transformPoint( it.model, it.mesh->center );
        const float sx = m3::length( { it.model[0], it.model[1], it.model[2] } ), sy = m3::length( { it.model[4], it.model[5], it.model[6] } ),
                    sz = m3::length( { it.model[8], it.model[9], it.model[10] } );
        r = it.mesh->radius * std::max( { sx, sy, sz } );
      };

      // Shadow pass: an orthographic light view around what the camera looks at.
      static const bool noShadows = std::getenv( "BICYCLE_NO_SHADOWS" ) != nullptr;  // debugging / slow GPUs
      const bool shadows = v.shadows && _shadowOk && !noShadows;
      m3::Mat4 lightVP = m3::identity();
      if ( shadows ) {
        const phys3d::Transform* focus = w3.rig().target ? &w3.transform( w3.rig().target ) : nullptr;
        const m3::Vec3 center = focus && focus->on ? focus->pos + fwd * 6.f : eye + fwd * 12.f;
        const m3::Vec3 ld = m3::normalize( v.lightDir );
        const m3::Vec3 up = std::fabs( ld.y ) > 0.99f ? m3::Vec3{ 0, 0, 1 } : m3::Vec3{ 0, 1, 0 };
        const float ext = SHADOW_EXTENT;
        // Snap the center to shadow-map texels so the shadows don't shimmer as the camera moves.
        const m3::Mat4 lv0 = m3::lookAt( { 0, 0, 0 }, ld, up );
        m3::Vec3 lc = m3::transformPoint( lv0, center );
        const float texel = 2 * ext / SHADOW_SIZE;
        lc.x = std::floor( lc.x / texel ) * texel;
        lc.y = std::floor( lc.y / texel ) * texel;
        const m3::Mat4 lightProj = m3::ortho( lc.x - ext, lc.x + ext, lc.y - ext, lc.y + ext, -lc.z - 80, -lc.z + 80 );
        lightVP = m3::mul( lightProj, lv0 );
        const m3::Planes lp = m3::frustumPlanes( lightVP );
        gl.BindFramebuffer( GL_FRAMEBUFFER, _shadowFbo );
        gl.Viewport( 0, 0, SHADOW_SIZE, SHADOW_SIZE );
        gl.Clear( GL_DEPTH_BUFFER_BIT );
        gl.Enable( GL_DEPTH_TEST );
        gl.DepthFunc( GL_LEQUAL );
        gl.Enable( GL_POLYGON_OFFSET_FILL );
        gl.PolygonOffset( 1.5f, 3.0f );
        gl.UseProgram( _depthProg );
        gl.ActiveTexture( GL_TEXTURE0 );
        gl.Uniform1i( gl.GetUniformLocation( _depthProg, "tex" ), 0 );
        gl.UniformMatrix4fv( gl.GetUniformLocation( _depthProg, "uLightVP" ), 1, 0, lightVP.data() );
        for ( const auto& it : _items ) {
          m3::Vec3 c;
          float r;
          worldSphere( it, c, r );
          if ( !m3::sphereVisible( lp, c, r ) ) continue;
          drawMesh( *it.mesh, lightVP, it.model, it.skinned ? it.bones->data() : nullptr, _depthProg );
        }
        gl.Disable( GL_POLYGON_OFFSET_FILL );
        gl.BindFramebuffer( GL_FRAMEBUFFER, 0 );
      }

      // Main pass.
      gl.Viewport( 0, 0, vw, vh );
      gl.Enable( GL_DEPTH_TEST );
      gl.DepthFunc( GL_LEQUAL );
      gl.Disable( GL_BLEND );
      gl.UseProgram( _prog );
      gl.ActiveTexture( GL_TEXTURE1 );
      gl.BindTexture( GL_TEXTURE_2D, _shadowTex );
      gl.ActiveTexture( GL_TEXTURE0 );
      gl.Uniform1i( gl.GetUniformLocation( _prog, "tex" ), 0 );
      setWorldUniforms( v, eye, lightVP, shadows );
      for ( const auto& it : _items ) {
        m3::Vec3 c;
        float r;
        worldSphere( it, c, r );
        if ( !m3::sphereVisible( planes, c, r ) ) { ++_stats3d.culled; continue; }
        drawMesh( *it.mesh, viewProj, it.model, it.skinned ? it.bones->data() : nullptr );
        ++_stats3d.meshes;
      }

      // Billboards: 3D entities with a sprite and no model, upright and turned to the camera (cylindrical).
      const m3::Vec3 right = m3::flatRight( cam.yaw );
      for ( Entity e = 1; e < NUM_SUPPORTED_ENTITIES; ++e ) {
        const auto& t = w3.transform( e );
        if ( !t.on || !t.visible || _models.contains( e ) || e == w3.cameraEntity() ) continue;
        const auto& img = World::get<Image>( e );
        if ( !img.pixels ) continue;
        Rect src = World::get<Rect>( e );
        if ( src.w <= 0 ) src = Rect( 0, 0, img.size.w >> FIXEDPT_DEC_BITS, img.size.h >> FIXEDPT_DEC_BITS );
        const float pw = src.w / 2.0f, ph = static_cast<float>( src.h );  // pixels (a pixel is 2x1 cells)
        const float w = pw * v.billboardPixel * t.scale.x, h = ph * v.billboardPixel * t.scale.y;
        if ( !m3::sphereVisible( planes, t.pos + m3::Vec3{ 0, h / 2, 0 }, std::max( w, h ) ) ) { ++_stats3d.culled; continue; }
        const auto at = _atlas.entry( img.pixels );
        const float aw = static_cast<float>( _atlas.width() ), ah = static_cast<float>( _atlas.height() );
        const float u0 = ( at.x + src.x / 2.0f ) / aw, u1 = ( at.x + ( src.x + src.w ) / 2.0f ) / aw;
        const float t0 = ( at.y + static_cast<float>( src.y ) ) / ah, t1 = ( at.y + static_cast<float>( src.y + src.h ) ) / ah;
        const m3::Vec3 a = t.pos - right * ( w / 2 ), b = t.pos + right * ( w / 2 );
        const m3::Vec3 n = -m3::flatForward( cam.yaw );
        auto vtx = [&]( m3::Vec3 p, float uu, float vv ) {
          _bbVerts.insert( _bbVerts.end(), { p.x, p.y, p.z, n.x, n.y, n.z, uu, vv } );
        };
        const m3::Vec3 up{ 0, h, 0 };
        vtx( a, u0, 1 - t1 ); vtx( b, u1, 1 - t1 ); vtx( a + up, u0, 1 - t0 );
        vtx( b, u1, 1 - t1 ); vtx( b + up, u1, 1 - t0 ); vtx( a + up, u0, 1 - t0 );
        ++_stats3d.billboards;
      }
      if ( !_bbVerts.empty() ) {
        if ( _atlasTexVersion != _atlas.version() ) {
          if ( !_atlasTex ) gl.GenTextures( 1, &_atlasTex );
          texParams( _atlasTex, GL_CLAMP_TO_EDGE );
          gl.PixelStorei( GL_UNPACK_ALIGNMENT, 4 );
          gl.TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, _atlas.width(), _atlas.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, _atlas.pixels().data() );
          _atlasTexVersion = _atlas.version();
        }
        gl.BindVertexArray( _quadVao );
        gl.BindBuffer( GL_ARRAY_BUFFER, _quadVbo );
        gl.BufferData( GL_ARRAY_BUFFER, _bbVerts.size() * sizeof( float ), _bbVerts.data(), GL_DYNAMIC_DRAW );
        const m3::Mat4 id = m3::identity();
        gl.UniformMatrix4fv( gl.GetUniformLocation( _prog, "uMvp" ), 1, 0, viewProj.data() );
        gl.UniformMatrix4fv( gl.GetUniformLocation( _prog, "uModel" ), 1, 0, id.data() );
        const float white[4] = { 1, 1, 1, 1 };
        gl.Uniform4fv( gl.GetUniformLocation( _prog, "uColor" ), 1, white );
        gl.Uniform1i( gl.GetUniformLocation( _prog, "uTex" ), 1 );
        gl.Uniform1i( gl.GetUniformLocation( _prog, "uLit" ), 0 );
        gl.Uniform1i( gl.GetUniformLocation( _prog, "uSkinned" ), 0 );
        gl.BindTexture( GL_TEXTURE_2D, _atlasTex );
        gl.DrawArrays( GL_TRIANGLES, 0, static_cast<GLsizei>( _bbVerts.size() / 8 ) );
        _bbVerts.clear();
      }
      if ( v.crosshair ) {
        const int cx = _fbW / 2, cy = _fbH / 2;
        for ( int d = -9; d <= 9; ++d ) {
          if ( std::abs( d ) < 3 ) continue;
          for ( int k = -1; k <= 1; ++k ) {
            blendPixel( cx + d, cy + k, k == 0 ? 0xFFFFFFFFu : 0xFF000000u, 200 );
            blendPixel( cx + k, cy + d, k == 0 ? 0xFFFFFFFFu : 0xFF000000u, 200 );
          }
        }
      }
      if ( std::getenv( "BICYCLE_FRAME_STATS" ) && _tick % 30 == 0 ) {
        std::fprintf( stderr, "3D: %zu meshes drawn, %zu culled, %zu billboards, %zu level tris\n", _stats3d.meshes,
                      _stats3d.culled, _stats3d.billboards, w3.levelTriangles() );
      }
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
      gfx::Pixels px( static_cast<size_t>( _fbW ) * _fbH );
      gl.PixelStorei( GL_PACK_ALIGNMENT, 4 );
      gl.ReadPixels( 0, 0, _fbW, _fbH, GL_BGRA, GL_UNSIGNED_BYTE, px.data() );
      _capture.resize( px.size() );
      for ( int y = 0; y < _fbH; ++y )  // GL rows are bottom first
        for ( int x = 0; x < _fbW; ++x ) _capture[static_cast<size_t>( y ) * _fbW + x] = px[static_cast<size_t>( _fbH - 1 - y ) * _fbW + x] | 0xFF000000u;
    }

    static GL gl;
    SDL_GLContext _ctx{};
    static constexpr int SHADOW_SIZE = 2048;
    static constexpr float SHADOW_EXTENT = 22.f;  // meters each side of the shadow focus
    std::vector<Item> _items;
    std::vector<std::array<float, 16 * skin::MAX_JOINTS>> _poses;
    GLuint _depthProg{}, _shadowTex{}, _shadowFbo{};
    bool _shadowOk{};
    struct Stats3D { size_t meshes{}, culled{}, billboards{}; } _stats3d;
    GLuint _prog{}, _overlayProg{}, _quadVao{}, _quadVbo{}, _overlayVao{}, _overlayVbo{}, _overlayTex{};
    Box _cam{};
    bool _haveScene{};
    Win* _sceneWin{};
    std::vector<Billboard> _drawn;
    std::map<std::string, std::shared_ptr<GpuMesh>> _meshes;
    EntitySlots<std::shared_ptr<GpuMesh>> _models;  // flat, by entity id (generation-checked)
    std::vector<std::shared_ptr<GpuMesh>> _statics;
    mem::Vector<float, mem::GRAPHIC> _bbVerts;  // this frame's billboard quads (GL upload staging)
    GLuint _atlasTex{};
    uint32_t _atlasTexVersion{};
    struct { size_t billboardDraws{}, billboards{}, skinnedDraws{}; } _stats;
    static auto now() -> double {
      return std::chrono::duration<double>( std::chrono::steady_clock::now().time_since_epoch() ).count();
    }
    // walk: frames moving in a row; still: frames standing; action: the model's action (setModelAction), and
    // when it started, in seconds.
    struct Motion { float x{}; float facing{}; int walk{}; int still{}; bool seen{}; std::string action{ "idle" }; double since{}; float z3{}; std::string playing; };
    EntitySlots<Motion> _motion;
    gfx::Pixels _capture;
    long _tick{};
};

GL Renderer3D::gl;

auto renderer() -> Renderer& {
  static Renderer3D r;
  return r;
}

}  // namespace gfx
