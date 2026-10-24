// 3D mode: OpenGL 3.3 core through SDL3. The scene's camera box picks what the 3D camera follows; entities with a
// model (setModel, the Lua "model" gene) are drawn as meshes (.bmesh, Edgebreaker-compressed, from Blender via
// scripts/blender/export.py), other entities as camera-facing billboards of their current sprite frame, and static
// scenery (addStaticModel) around them. Windows, the HUD and dialogue are PixelRenderer's 2D framebuffer composited
// over the 3D world as an overlay.
//
// Cells -> world: X = cells / 16 (one 8-pixel-wide sprite = 1 unit), Y = height above the ground in pixel rows / 8,
// Z = 0 for models, slightly behind for billboards. The ground row is the lowest entity bottom in view.
#include "v/sdl/PixelRenderer.h"
#include <cstring>
#include <unordered_map>
#include <string>
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
  GL_UNSIGNED_INT = 0x1405, GL_CLAMP_TO_EDGE = 0x812F, GL_TRIANGLES = 0x0004, GL_TRIANGLE_STRIP = 0x0005, GL_TEXTURE0 = 0x84C0,
  GL_LEQUAL = 0x0203, GL_PACK_ALIGNMENT = 0x0D05, GL_UNPACK_ALIGNMENT = 0x0CF5, GL_VERSION = 0x1F02,
  GL_RENDERER = 0x1F01, GL_FRAMEBUFFER = 0x8D40, GL_DEPTH_ATTACHMENT = 0x8D00, GL_DEPTH_COMPONENT = 0x1902,
  GL_DEPTH_COMPONENT24 = 0x81A6, GL_NONE = 0, GL_POLYGON_OFFSET_FILL = 0x8037, GL_LINEAR = 0x2601,
  GL_TEXTURE_COMPARE_MODE = 0x884C, GL_COMPARE_REF_TO_TEXTURE = 0x884E, GL_TEXTURE_COMPARE_FUNC = 0x884D,
  GL_CLAMP_TO_BORDER = 0x812D, GL_TEXTURE_BORDER_COLOR = 0x1004, GL_TEXTURE1 = 0x84C1, GL_FRAMEBUFFER_COMPLETE = 0x8CD5,
  GL_CULL_FACE = 0x0B44, GL_BACK = 0x0405, GL_DEPTH_COMPONENT16 = 0x81A5, GL_UNSIGNED_SHORT = 0x1403, GL_EXTENSIONS = 0x1F03,
  GL_READ_FRAMEBUFFER = 0x8CA8, GL_DRAW_FRAMEBUFFER = 0x8CA9, GL_ACTIVE_UNIFORMS = 0x8B86,
  GL_MAX_VERTEX_UNIFORM_VECTORS = 0x8DFB, GL_MAX_FRAGMENT_UNIFORM_VECTORS = 0x8DFD, GL_MAX_VARYING_VECTORS = 0x8DFC,
  GL_DEPTH_BITS = 0x0D56, GL_SHADING_LANGUAGE_VERSION = 0x8B8C,
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
  X( GLboolean, IsTexture, GLuint ) \
  X( void, DepthMask, GLboolean ) \
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
  X( void, Finish ) \
  X( void, TexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void* ) \
  X( void, GenRenderbuffers, GLsizei, GLuint* ) \
  X( void, BindRenderbuffer, GLenum, GLuint ) \
  X( void, RenderbufferStorage, GLenum, GLenum, GLsizei, GLsizei ) \
  X( void, FramebufferRenderbuffer, GLenum, GLenum, GLenum, GLuint ) \
  X( void, BlitFramebuffer, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum ) \
  X( void, ReadBuffer, GLenum ) \
  X( void, PolygonOffset, GLfloat, GLfloat ) \
  X( void, TexParameterfv, GLenum, GLenum, const GLfloat* ) \
  X( void, Uniform3fv, GLint, GLsizei, const GLfloat* ) \
  X( void, Uniform1f, GLint, GLfloat ) \
  X( void, Uniform2fv, GLint, GLsizei, const GLfloat* ) \
  X( void, DisableVertexAttribArray, GLuint ) \
  X( void, BindAttribLocation, GLuint, GLuint, const GLchar* ) \
  X( GLenum, GetError ) \
  X( void, GetIntegerv, GLenum, GLint* ) \
  X( void, GetActiveUniform, GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar* )

struct GL {
#define BGL_DECL( ret, name, ... ) ret( BGL_API* name )( __VA_ARGS__ ) = nullptr;
  BGL_FUNCS( BGL_DECL )
#undef BGL_DECL
  // Not in OpenGL ES 2.0 (the GLES2 path does without them).
  static auto optional( const char* n ) -> bool {
    for ( const char* o : { "GenVertexArrays", "DeleteVertexArrays", "BindVertexArray", "DrawBuffer", "ReadBuffer", "TexParameterfv", "BlitFramebuffer" } )
      if ( std::strcmp( n, o ) == 0 ) return true;
    return false;
  }
  auto load() -> bool {
    bool ok = true;
#define BGL_LOAD( ret, name, ... ) \
    name = reinterpret_cast<decltype( name )>( SDL_GL_GetProcAddress( "gl" #name ) ); \
    if ( !name && !optional( #name ) ) { std::fprintf( stderr, "bicycle 3D: missing gl" #name "\n" ); ok = false; }
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
    n = mat3(s[0].xyz, s[1].xyz, s[2].xyz) * n;
  }
  gl_Position = uMvp * p;
  vN = mat3(uModel[0].xyz, uModel[1].xyz, uModel[2].xyz) * n;
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
uniform int uShadowTaps;  // 1 (low), 4 (med), 9 (high): depth compares per pixel, each bilinear-filtered where supported
uniform vec3 uCamPos;
uniform vec3 uFogColor;
uniform vec2 uFog;  // near, far
out vec4 o;
float tap(vec2 uv, float z) { return texture(uShadowMap, vec3(uv, z)); }
float shadow(vec3 n) {
  vec3 p = vLight.xyz / vLight.w * 0.5 + 0.5;
  if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0 || p.z > 1.0) return 1.0;
  float bias = max(0.0025 * (1.0 - dot(n, -uLightDir)), 0.0008);
  float z = p.z - bias;
  if (uShadowTaps == 1) return tap(p.xy, z);
  if (uShadowTaps == 4) {
    float h = 0.5 * uShadowTexel;
    return 0.25 * (tap(p.xy + vec2(-h, -h), z) + tap(p.xy + vec2(h, -h), z) + tap(p.xy + vec2(-h, h), z) + tap(p.xy + vec2(h, h), z));
  }
  float s = 0.0;
  for (int y = -1; y <= 1; ++y)
    for (int x = -1; x <= 1; ++x) s += tap(p.xy + vec2(x, y) * uShadowTexel, z);
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

// Blob shadows: soft dark circles on the ground under modelled bodies when there's no shadow map (GLES2 / Pi Zero,
// BICYCLE_SHADOWS=off). All of a frame's blobs are one triangle list in world space: pos.xyz + alpha in w, uv.
const char* BLOB_VS = R"(#version 330 core
layout(location = 0) in vec4 aPos;
layout(location = 2) in vec2 aUv;
uniform mat4 uMvp;
out vec2 vUv;
out float vA;
void main() {
  vUv = aUv;
  vA = aPos.w;
  gl_Position = uMvp * vec4(aPos.xyz, 1.0);
})";
const char* BLOB_FS = R"(#version 330 core
in vec2 vUv;
in float vA;
uniform sampler2D tex;
out vec4 o;
void main() { o = vec4(0.0, 0.0, 0.0, texture(tex, vUv).a * vA); })";

const char* OVERLAY_VS = R"(#version 330 core
layout(location = 0) in vec2 aPos;
uniform vec2 uUvScale;  // the screen's part of the texture (GLES2 without NPOT textures: a power-of-two texture)
out vec2 vUv;
void main() {
  vUv = vec2(aPos.x * 0.5 + 0.5, 0.5 - aPos.y * 0.5) * uUvScale;  // framebuffer rows are top first
  gl_Position = vec4(aPos, 0.0, 1.0);
})";

const char* OVERLAY_FS = R"(#version 330 core
in vec2 vUv;
uniform sampler2D tex;
out vec4 o;
void main() { o = texture(tex, vUv); })";

// OpenGL ES 2.0 (Pi Zero / Zero 2 / 3: vc4). vc4's vertex shader compiler fails register allocation on the
// GL 3.3 shader above (32 bone matrices = 512 uniform floats, a 4-matrix blend, the light matrix): it rejects the
// shader when it's first drawn ("Failed to register allocate", no GL error) and nothing 3D is drawn. So GLES2 has
// its own small shaders: a static one (MVP + model, N.L per vertex, fog per pixel from the interpolated eye offset; no shadow map) and a skinned one with
// at most ES2_BONES joints as 3 rows each. Skinned meshes are skinned on the CPU by default (vc4 may still reject
// the skinned shader, and that can't be detected); BICYCLE_GLES2_GPU_SKIN=1 tries the skinned shader.
constexpr int ES2_BONES = 24;
const char* VS_ES2 = R"(#version 100
precision highp float;
attribute vec3 aPos;
attribute vec3 aNrm;
attribute vec2 aUv;
uniform mat4 uMvp;
uniform mat4 uModel;
uniform vec3 uCamPos;
uniform vec3 uLightDir;
uniform mediump int uWorld3D;  // shared with the fragment shader, so the same precision
varying float vNdl;  // N.L per vertex (exact for flat faces under a directional light)
varying vec2 vUv;
varying vec3 vRel;   // world position - eye; interpolates exactly, so the fog distance is per pixel
#ifdef SHADOWS
uniform mat4 uLightMvp;  // light view-projection x model, multiplied on the CPU
varying vec4 vLight;
#endif
void main() {
  vec4 p = vec4(aPos, 1.0);
  gl_Position = uMvp * p;
#ifdef SHADOWS
  vLight = uLightMvp * p;
#endif
  vec3 n = normalize(mat3(uModel[0].xyz, uModel[1].xyz, uModel[2].xyz) * aNrm);
  vNdl = dot(n, uWorld3D == 1 ? -uLightDir : vec3(0.3980, 0.7960, 0.4975));
  vUv = aUv;
  vRel = (uModel * p).xyz - uCamPos;
})";
const char* VS_ES2_SKIN = R"(#version 100
precision highp float;
attribute vec3 aPos;
attribute vec3 aNrm;
attribute vec2 aUv;
attribute vec4 aJoints;
attribute vec4 aWeights;
uniform mat4 uMvp;
uniform mat4 uModel;
uniform vec3 uCamPos;
uniform vec3 uLightDir;
uniform mediump int uWorld3D;
uniform vec4 uBones[72];  // ES2_BONES x 3 rows of each joint's skinning matrix
varying float vNdl;
varying vec2 vUv;
varying vec3 vRel;
void main() {
  ivec4 j = ivec4(aJoints) * 3;
  vec4 r0 = aWeights.x * uBones[j.x] + aWeights.y * uBones[j.y] + aWeights.z * uBones[j.z] + aWeights.w * uBones[j.w];
  vec4 r1 = aWeights.x * uBones[j.x + 1] + aWeights.y * uBones[j.y + 1] + aWeights.z * uBones[j.z + 1] + aWeights.w * uBones[j.w + 1];
  vec4 r2 = aWeights.x * uBones[j.x + 2] + aWeights.y * uBones[j.y + 2] + aWeights.z * uBones[j.z + 2] + aWeights.w * uBones[j.w + 2];
  vec4 p = vec4(aPos, 1.0);
  p = vec4(dot(r0, p), dot(r1, p), dot(r2, p), 1.0);
  vec3 n = vec3(dot(r0.xyz, aNrm), dot(r1.xyz, aNrm), dot(r2.xyz, aNrm));
  gl_Position = uMvp * p;
  n = normalize(mat3(uModel[0].xyz, uModel[1].xyz, uModel[2].xyz) * n);
  vNdl = dot(n, uWorld3D == 1 ? -uLightDir : vec3(0.3980, 0.7960, 0.4975));
  vUv = aUv;
  vRel = (uModel * p).xyz - uCamPos;
})";
const char* FS_ES2 = R"(#version 100
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;  // texture coordinates and the fog distance at full precision where the GPU has it
#else
precision mediump float;
#endif
varying float vNdl;
varying vec2 vUv;
varying vec3 vRel;
uniform int uView;  // BICYCLE_GL_VIEW: 0 normal, 1 uv (fract), 2 tex (raw texture), 3 lit (lighting only), 4 fog
#ifdef SHADOWS
varying vec4 vLight;
uniform sampler2D uShadowMap;  // OES_depth_texture: depth in .r; compared here (no shadow samplers in GLES2)
float shadow() {
  vec3 q = vLight.xyz / vLight.w * 0.5 + 0.5;
  if (q.x < 0.0 || q.x > 1.0 || q.y < 0.0 || q.y > 1.0 || q.z > 1.0) return 1.0;
  float bias = max(0.003 * (1.0 - max(vNdl, 0.0)), 0.0012);
  return q.z - bias > texture2D(uShadowMap, q.xy).r ? 0.0 : 1.0;
}
#else
float shadow() { return 1.0; }
#endif
uniform vec4 uColor;
uniform int uTex;
uniform int uLit;
uniform sampler2D tex;
uniform mediump int uWorld3D;
uniform vec3 uLightColor;
uniform float uAmbient;
uniform vec3 uFogColor;
uniform vec2 uFog;  // near, far
void main() {
  vec4 c = uColor;
  if (uTex == 1) c *= texture2D(tex, vec2(vUv.x, 1.0 - vUv.y));
  if (c.a < 0.5) discard;
  if (uView == 1) { gl_FragColor = vec4(fract(vUv), uTex == 1 ? 1.0 : 0.0, 1.0); return; }
  if (uView == 2) { gl_FragColor = uTex == 1 ? texture2D(tex, vec2(vUv.x, 1.0 - vUv.y)) : vec4(1.0, 0.0, 1.0, 1.0); return; }
  if (uWorld3D == 0) {
    float l = 1.0;
    if (uLit == 1) l = 0.45 + 0.55 * max(vNdl, 0.0);
    gl_FragColor = vec4(c.rgb * l, c.a);
    return;
  }
  vec3 rgb = c.rgb;
  float sh = shadow();
  vec3 light = vec3(mix(uAmbient + 0.25, 1.0, sh));  // billboards: shadowed but unlit
  if (uLit == 1) light = uAmbient + (1.0 - uAmbient) * max(gl_FrontFacing ? vNdl : -vNdl, 0.0) * sh * uLightColor;
  rgb = c.rgb * light;
  float f = clamp((length(vRel) - uFog.x) / max(uFog.y - uFog.x, 0.001), 0.0, 1.0);
  if (uView == 3) { gl_FragColor = vec4(light, 1.0); return; }
  if (uView == 4) { gl_FragColor = vec4(vec3(f), 1.0); return; }
  gl_FragColor = vec4(mix(rgb, uFogColor, f), c.a);
})";
// GLES2 shadow map pass: position only (plus the uv for cut-out textures).
const char* VS_ES2_DEPTH = R"(#version 100
attribute vec3 aPos;
attribute vec2 aUv;
uniform mat4 uMvp;
varying vec2 vUv;
void main() { vUv = aUv; gl_Position = uMvp * vec4(aPos, 1.0); })";
const char* FS_ES2_DEPTH = R"(#version 100
precision mediump float;
varying vec2 vUv;
uniform int uTex;
uniform sampler2D tex;
void main() {
  if (uTex == 1 && texture2D(tex, vec2(vUv.x, 1.0 - vUv.y)).a < 0.5) discard;
  gl_FragColor = vec4(1.0);
})";
// The last resort when a GLES2 program doesn't link: unlit, untextured-or-textured, no fog. Logged loudly.
const char* VS_ES2_MIN = R"(#version 100
attribute vec3 aPos;
attribute vec2 aUv;
uniform mat4 uMvp;
varying vec2 vUv;
void main() { vUv = aUv; gl_Position = uMvp * vec4(aPos, 1.0); })";
const char* FS_ES2_MIN = R"(#version 100
precision mediump float;
varying vec2 vUv;
uniform vec4 uColor;
uniform int uTex;
uniform sampler2D tex;
void main() {
  vec4 c = uColor;
  if (uTex == 1) c *= texture2D(tex, vec2(vUv.x, 1.0 - vUv.y));
  if (c.a < 0.5) discard;
  gl_FragColor = c;
})";

struct GpuMesh {
  GLuint vao{}, vbo{};
  std::shared_ptr<const Skin> skin;  // skinned meshes: skeleton + actions (.bskin)
  // GLES2 CPU skinning: the bind-pose vertices, and a dynamic buffer the skinned ones are written to per draw.
  std::vector<float> bindVerts;
  mutable std::vector<float> skinVerts;
  mutable GLuint skinVbo{};
  struct Range { int first{}, count{}; std::array<float, 4> rgba{}; GLuint tex{}; std::string texPath; };
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
    void drawWidget( WinHandle win, int y, int x, Align align, const Widget& w ) override {
      if ( w.kind == Widget::BAR ) drawMeter( win, y, x, w );
      else drawLabel( win, y, x, align, w.text );
    }

  private:
    void drawMeter( WinHandle win, int y, int x, const Widget& bar ) {
      const Win* w = static_cast<Win*>( win );
      if ( !w || ( w->attr & ATTR_INVISIBLE ) ) return;
      int px = ( w->x + x ) * CELL_W + 6;
      const int top = ( w->y + y ) * CELL_H + 2;
      const float frac = std::clamp( static_cast<float>( bar.curr ) / std::max( 1, bar.max ), 0.0f, 1.0f );
      const int bw = std::max( 24, bar.len * CELL_W * 2 ), bh = 16;
      const int labelW = bar.text.empty() ? 0 : measureString( bar.text, UI_TEXT_PX, true ) + 10;
      const std::string nums = std::to_string( bar.curr ) + "/" + std::to_string( bar.max );
      const int numsW = measureString( nums, UI_TEXT_PX ) + 10;
      // A translucent plate behind the whole meter so it reads over any background.
      gradientRoundRect( nullptr, px - 6, top - 4, labelW + bw + numsW + 12, bh + 8, 8, 0xA0202838u, 0xA0101420u );
      if ( !bar.text.empty() ) {
        drawString( w, px, top, bar.text, 0xFFFFFFFFu, UI_TEXT_PX, true );
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

    // A HUD label on the same translucent plate as the meter's.
    void drawLabel( WinHandle win, int y, int x, Align align, const std::string& s ) {
      const Win* w = static_cast<Win*>( win );
      if ( !w || ( w->attr & ATTR_INVISIBLE ) || s.empty() ) return;
      const int tw = measureString( s, UI_TEXT_PX );
      const int anchor = ( w->x + x ) * CELL_W;
      const int px = align == Align::End ? anchor - 12 - tw : align == Align::Center ? anchor - tw / 2 : anchor + 6;
      const int top = ( w->y + y ) * CELL_H + 2;
      gradientRoundRect( nullptr, px - 6, top - 4, tw + 12, 16 + 8, 8, 0xA0202838u, 0xA0101420u );
      drawString( w, px, top, s, 0xFFFFFFFFu, UI_TEXT_PX );
    }

  public:
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
        if ( g.vao && gl.DeleteVertexArrays ) gl.DeleteVertexArrays( 1, &g.vao );
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

    // Context tiers, best first: OpenGL 3.3 core; OpenGL ES 3.0+ (Raspberry Pi 4 v3d: GLSL 300 es, VAOs, shadow map);
    // OpenGL ES 2.0 (Pi Zero vc4: GLSL 100, no VAOs, no shadow map). BICYCLE_GL=core|gles3|gles2 picks the first tier
    // to try (BICYCLE_GLES2=1 or -DBICYCLE_GLES2=ON means gles2). A failed tier's window is destroyed and recreated
    // before the next is tried (X11/Wayland may fix a window's visual/config to the first context attempted).
    enum Tier { CORE, GLES3, GLES2 };
    static auto tierName( int t ) -> const char* { return t == CORE ? "OpenGL 3.3 core" : t == GLES3 ? "OpenGL ES 3.0" : "OpenGL ES 2.0"; }
    static void contextAttributes( int t ) {
      s_tier = t;
      s_es2 = t == GLES2;
      SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, t == GLES2 ? 2 : 3 );
      SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, t == CORE ? 3 : 0 );
      SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, t == CORE ? SDL_GL_CONTEXT_PROFILE_CORE : SDL_GL_CONTEXT_PROFILE_ES );
    }
    void beforeWindowCreated() override {
      int t = CORE;
#ifdef BICYCLE_GLES2
      t = GLES2;
#endif
      if ( const char* e = std::getenv( "BICYCLE_GLES2" ) ) t = ( *e && *e != '0' ) ? GLES2 : CORE;
      if ( const char* e = std::getenv( "BICYCLE_GL" ) ) {
        const std::string v = e;
        if ( v == "core" ) t = CORE;
        else if ( v == "gles3" ) t = GLES3;
        else if ( v == "gles2" ) t = GLES2;
        else if ( !v.empty() ) std::fprintf( stderr, "bicycle 3D: BICYCLE_GL=%s unknown (core|gles3|gles2)\n", e );
      }
      contextAttributes( t );
      SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
      SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    }

    void onWindowCreated() override {
      for ( ;; ) {
        _ctx = SDL_GL_CreateContext( _window );
        if ( _ctx && gl.load() ) break;
        std::fprintf( stderr, "bicycle 3D: no %s context (%s)\n", tierName( s_tier ), SDL_GetError() );
        if ( _ctx ) SDL_GL_DestroyContext( _ctx );
        _ctx = nullptr;
        if ( s_tier == GLES2 ) {
          std::fprintf( stderr, "bicycle 3D: no OpenGL 3.3, ES 3.0 or ES 2.0 context\n" );
          std::exit( 1 );
        }
        // Next tier, on a fresh window.
        const std::string title = SDL_GetWindowTitle( _window );
        int w{}, h{};
        SDL_GetWindowSize( _window, &w, &h );
        SDL_DestroyWindow( _window );
        contextAttributes( s_tier + 1 );
        _window = SDL_CreateWindow( title.c_str(), w, h, windowFlags() );
        if ( !_window ) {
          std::fprintf( stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError() );
          std::exit( 1 );
        }
        SDL_GetWindowSizeInPixels( _window, &_fbW, &_fbH );
      }
      std::fprintf( stderr, "bicycle 3D: %s tier (GL %s on %s)\n", tierName( s_tier ), gl.GetString( GL_VERSION ), gl.GetString( GL_RENDERER ) );
      SDL_GL_SetSwapInterval( 0 );
      if ( s_tier != CORE ) glInfo();
      if ( s_es2 ) {
        bool ok = false;
        _prog = program( VS_ES2, FS_ES2, true, &ok );
        if ( !ok ) {
          std::fprintf( stderr, "bicycle 3D: *** the GLES2 shader failed; falling back to the minimal unlit shader ***\n" );
          _prog = program( VS_ES2_MIN, FS_ES2_MIN, true, &ok );
          if ( !ok ) std::fprintf( stderr, "bicycle 3D: *** even the minimal GLES2 shader failed: nothing 3D will draw ***\n" );
        }
        const char* g = std::getenv( "BICYCLE_GLES2_GPU_SKIN" );
        if ( g && *g && *g != '0' ) {
          bool sok = false;
          _skinProg = program( VS_ES2_SKIN, FS_ES2, true, &sok );
          if ( !sok ) _skinProg = 0;
        }
        std::fprintf( stderr, "bicycle 3D: GLES2 skinning on the %s\n", _skinProg ? "GPU (BICYCLE_GLES2_GPU_SKIN)" : "CPU" );
        _plainProg = _prog;
        _depthProg = 0;
        // Shadows (opt-in, BICYCLE_SHADOWS=low|med|high): OES_depth_texture map, a separate shader variant with the
        // light matrix and one manual depth compare, so the plain variant stays as small as it is.
        const char* ext = reinterpret_cast<const char*>( gl.GetString( GL_EXTENSIONS ) );
        if ( ext && std::strstr( ext, "GL_OES_depth_texture" ) ) {
          auto withShadows = []( const char* src ) {  // "#define SHADOWS" right after the #version line
            std::string t = src;
            return t.insert( t.find( '\n' ) + 1, "#define SHADOWS 1\n" );
          };
          const std::string vs = withShadows( VS_ES2 ), fs = withShadows( FS_ES2 );
          bool sok = false, dok = false;
          _shadowProg = program( vs.c_str(), fs.c_str(), true, &sok );
          _depthProg = program( VS_ES2_DEPTH, FS_ES2_DEPTH, true, &dok );
          if ( !sok || !dok ) _shadowProg = _depthProg = 0;
        }
      }
      else {
        _prog = program( VS, FS );
        _depthProg = program( VS, DEPTH_FS );
      }
      if ( s_tier != CORE || glDebug() ) {
        reportUniforms( "main", _prog );
        if ( _skinProg ) reportUniforms( "skinned", _skinProg );
        if ( _shadowProg ) reportUniforms( "shadowed", _shadowProg );
      }
      // The color texture is on unit 0 and the shadow map on unit 1, always (two sampler types may not share a unit).
      for ( GLuint pr : { _prog, _skinProg, _shadowProg } ) {
        if ( !pr ) continue;
        gl.UseProgram( pr );
        gl.Uniform1i( gl.GetUniformLocation( pr, "tex" ), 0 );
        gl.Uniform1i( gl.GetUniformLocation( pr, "uShadowMap" ), 1 );
      }
      gl.UseProgram( _prog );
      // Shadow map: a depth texture compared in the shader (sampler2DShadow, hardware PCF where available), made on
      // first use at the quality's size (ensureShadowMap). GLES2: none (no depth textures or shadow samplers).
      _shadowOk = !s_es2 || _shadowProg;
      {
        const char* r = reinterpret_cast<const char*>( gl.GetString( GL_RENDERER ) );
        const std::string rs = r ? r : "";
        // Tile-based / software GPUs where a 2048 9-tap map costs a lot: Raspberry Pi (v3d, vc4), Mali, llvmpipe.
        bool slow = false;
        for ( const char* k : { "V3D", "v3d", "VC4", "vc4", "Mali", "llvmpipe", "softpipe", "Adreno", "PowerVR" } )
          if ( rs.find( k ) != std::string::npos ) slow = true;
        _autoQuality = slow ? Q_LOW : Q_HIGH;
        // GLES2 (vc4): off unless asked for. The map pass redraws the casters whenever the light view or one of them
        // moves (no separate statics map without depth blits), and the fill rate of a Zero 2 is the limit already.
        if ( s_es2 ) _autoQuality = Q_OFF;
        const char* ext = s_tier != CORE ? reinterpret_cast<const char*>( gl.GetString( GL_EXTENSIONS ) ) : nullptr;
        s_bgraExt = ext && std::strstr( ext, "GL_EXT_texture_format_BGRA8888" );
        // Strict GLES2 without OES_texture_npot: a non-power-of-two texture may be incomplete (samples black), so
        // the screen-sized overlay goes into a power-of-two texture.
        s_npot = !s_es2 || ( ext && std::strstr( ext, "GL_OES_texture_npot" ) );
        if ( !s_npot ) std::fprintf( stderr, "bicycle 3D: no GL_OES_texture_npot: power-of-two overlay texture\n" );
      }
      _envQuality = parseQuality( std::getenv( "BICYCLE_SHADOWS" ) );
      if ( std::getenv( "BICYCLE_NO_SHADOWS" ) ) _envQuality = Q_OFF;
      s_fps = std::getenv( "BICYCLE_FPS" ) && *std::getenv( "BICYCLE_FPS" ) && *std::getenv( "BICYCLE_FPS" ) != '0';
      if ( _shadowOk )
        std::fprintf( stderr, "bicycle 3D: shadows %s%s\n", qualityName( _envQuality >= 0 ? _envQuality : _autoQuality ),
                      _envQuality >= 0 ? " (BICYCLE_SHADOWS)" : " (auto; BICYCLE_SHADOWS=off|low|med|high)" );
      _overlayProg = program( OVERLAY_VS, OVERLAY_FS );
      // Blob shadows: the program, a dynamic buffer and the prerendered soft circle (64x64, alpha falls off smoothly).
      _blobProg = program( BLOB_VS, BLOB_FS );
      if ( !s_es2 ) gl.GenVertexArrays( 1, &_blobVao );
      gl.GenBuffers( 1, &_blobVbo );
      if ( !s_es2 ) gl.BindVertexArray( _blobVao );
      gl.BindBuffer( GL_ARRAY_BUFFER, _blobVbo );
      blobLayout();
      {
        constexpr int N = 64;
        std::vector<uint32_t> px( N * N );
        for ( int y = 0; y < N; ++y )
          for ( int x = 0; x < N; ++x ) {
            const float dx = ( x + 0.5f ) / N * 2 - 1, dy = ( y + 0.5f ) / N * 2 - 1;
            const float r = std::sqrt( dx * dx + dy * dy );
            const float a = std::clamp( ( 1.0f - r ) / 0.5f, 0.0f, 1.0f );
            const float soft = a * a * ( 3 - 2 * a );  // an even dark middle, a soft rim over the outer half, 0 at the edge
            px[static_cast<std::size_t>( y ) * N + x] = static_cast<uint32_t>( std::lround( soft * 255 ) ) << 24;  // ARGB
          }
        gl.GenTextures( 1, &_blobTex );
        texParams( _blobTex, GL_CLAMP_TO_EDGE );
        gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        gl.PixelStorei( GL_UNPACK_ALIGNMENT, 4 );
        uploadBGRA( N, N, px.data() );
      }
      if ( const char* b = std::getenv( "BICYCLE_BLOB_SHADOWS" ) ) s_blobs = *b ? ( *b == '0' ? 0 : 1 ) : -1;
      // Billboard quad: rewritten per sprite. Overlay quad: the whole screen.
      if ( !s_es2 ) gl.GenVertexArrays( 1, &_quadVao );
      gl.GenBuffers( 1, &_quadVbo );
      if ( !s_es2 ) gl.BindVertexArray( _quadVao );
      gl.BindBuffer( GL_ARRAY_BUFFER, _quadVbo );
      gl.BufferData( GL_ARRAY_BUFFER, 4 * 8 * sizeof( float ), nullptr, GL_DYNAMIC_DRAW );
      vertexLayout();
      const float screen[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
      if ( !s_es2 ) gl.GenVertexArrays( 1, &_overlayVao );
      gl.GenBuffers( 1, &_overlayVbo );
      if ( !s_es2 ) gl.BindVertexArray( _overlayVao );
      gl.BindBuffer( GL_ARRAY_BUFFER, _overlayVbo );
      gl.BufferData( GL_ARRAY_BUFFER, sizeof screen, screen, GL_STATIC_DRAW );
      overlayLayout();
      gl.GenTextures( 1, &_overlayTex );
      texParams( _overlayTex, GL_CLAMP_TO_EDGE );
      glCheck( "setup" );
    }

    // --- diagnostics (GLES tiers always; BICYCLE_GL_DEBUG=1 adds the first frames' draw counts and camera) -------
    static auto glDebug() -> bool {
      static const bool on = [] { const char* e = std::getenv( "BICYCLE_GL_DEBUG" ); return e && *e && *e != '0'; }();
      return on;
    }
    static void glCheck( const char* where ) {
      for ( int i = 0; i < 8; ++i ) {
        const GLenum e = gl.GetError();
        if ( e == 0 ) break;
        std::fprintf( stderr, "bicycle 3D: GL error 0x%04x after %s\n", e, where );
      }
    }
    static void glInfo() {
      auto str = []( GLenum n ) { const char* s = reinterpret_cast<const char*>( gl.GetString( n ) ); return s ? s : "?"; };
      GLint vu{}, fu{}, vv{}, db{};
      gl.GetIntegerv( GL_MAX_VERTEX_UNIFORM_VECTORS, &vu );
      gl.GetIntegerv( GL_MAX_FRAGMENT_UNIFORM_VECTORS, &fu );
      gl.GetIntegerv( GL_MAX_VARYING_VECTORS, &vv );
      gl.GetIntegerv( GL_DEPTH_BITS, &db );
      gl.GetError();  // GL_DEPTH_BITS isn't core in every profile
      std::fprintf( stderr, "bicycle 3D: GL_RENDERER %s | GL_VERSION %s | GLSL %s\n", str( GL_RENDERER ), str( GL_VERSION ),
                    str( GL_SHADING_LANGUAGE_VERSION ) );
      std::fprintf( stderr, "bicycle 3D: max uniform vectors vertex %d fragment %d, varyings %d, depth bits %d\n", vu, fu, vv, db );
      std::fprintf( stderr, "bicycle 3D: GL_EXTENSIONS %s\n", str( GL_EXTENSIONS ) );
    }
    // The active uniforms of a program and their total size in vec4 slots (vc4: a vertex shader's uniforms and
    // temporaries must fit its register file; the GL 3.3 shader's 32 bone matrices alone are 128 vec4s).
    static void reportUniforms( const char* name, GLuint p ) {
      GLint n{};
      gl.GetProgramiv( p, GL_ACTIVE_UNIFORMS, &n );
      int vec4s = 0, floats = 0;
      for ( GLint i = 0; i < n; ++i ) {
        char nm[64];
        GLint size{};
        GLenum type{};
        gl.GetActiveUniform( p, static_cast<GLuint>( i ), sizeof nm, nullptr, &size, &type, nm );
        int comps = 4, slots = 1;
        switch ( type ) {
          case 0x1406: case 0x1404: case 0x8B56: case 0x8B5E: case 0x8B62: comps = 1; break;  // float int bool samplers
          case 0x8B50: case 0x8B53: comps = 2; break;
          case 0x8B51: case 0x8B54: comps = 3; break;
          case 0x8B5B: comps = 9; slots = 3; break;   // mat3
          case 0x8B5C: comps = 16; slots = 4; break;  // mat4
          default: break;
        }
        floats += comps * size;
        vec4s += slots * size;
      }
      std::fprintf( stderr, "bicycle 3D: %s shader: %d active uniforms, %d floats (%d vec4 slots)\n", name, n, floats, vec4s );
    }

    void onWindowDestroyed() override {
      if ( _ctx ) SDL_GL_DestroyContext( _ctx );
      _ctx = nullptr;
    }

    void presentFramebuffer() override {
      fpsBegin();
      const std::size_t draws0 = _frameDraws;
      gl.Viewport( 0, 0, _fbW, _fbH );
      auto& w3 = phys3d::World3D::get();
      const m3::Vec3 sky = w3.active() ? w3.view().sky : m3::Vec3{ 0.45f, 0.65f, 0.90f };
      gl.ClearColor( sky.x, sky.y, sky.z, 1.0f );
      gl.Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
      if ( glDebug() && _dbgFrame < 2 ) glCheck( "clear" );
      if ( _haveScene ) {
        if ( w3.active() ) drawWorld3D();
        else drawWorld();
      }
      if ( glDebug() && _dbgFrame < 2 ) glCheck( "the scene" );
      fpsMark( P_MAIN );  // 2.5D scenes: all of it
      drawOverlay();
      if ( glDebug() && _dbgFrame < 2 ) glCheck( "the overlay" );
      fpsMark( P_OVERLAY );
      if ( screenshotDue() ) readBack();
      ++_dbgFrame;
      if ( _dbgFrame == 1 || ( glDebug() && ( _dbgFrame == 2 || _dbgFrame == 30 || _dbgFrame == 120 ) ) ) {
        glCheck( _dbgFrame == 1 ? "the first frame" : "a frame" );
        if ( glDebug() || s_tier != CORE )
          std::fprintf( stderr, "bicycle 3D: frame %d: %zu draws, %s, camera at %.2f %.2f %.2f, %zu meshes drawn, %zu culled\n",
                        _dbgFrame, _frameDraws - draws0, w3.active() ? "true 3D" : "2.5D", _dbgEye.x, _dbgEye.y, _dbgEye.z,
                        _stats3d.meshes, _stats3d.culled );
      }
      SDL_GL_SwapWindow( _window );
      fpsMark( P_SWAP );
      fpsEnd();
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
      if ( s_es2 ) { gl.DisableVertexAttribArray( 3 ); gl.DisableVertexAttribArray( 4 ); }
    }
    static void blobLayout() {
      gl.VertexAttribPointer( 0, 4, GL_FLOAT, 0, 6 * sizeof( float ), nullptr );
      gl.VertexAttribPointer( 2, 2, GL_FLOAT, 0, 6 * sizeof( float ), reinterpret_cast<void*>( 4 * sizeof( float ) ) );
      gl.EnableVertexAttribArray( 0 );
      gl.EnableVertexAttribArray( 2 );
      if ( s_es2 ) for ( GLuint a : { 1u, 3u, 4u } ) gl.DisableVertexAttribArray( a );
    }
    static void overlayLayout() {
      gl.VertexAttribPointer( 0, 2, GL_FLOAT, 0, 2 * sizeof( float ), nullptr );
      gl.EnableVertexAttribArray( 0 );
      if ( s_es2 ) for ( GLuint a = 1; a < 5; ++a ) gl.DisableVertexAttribArray( a );
    }
    // A VAO where there are VAOs; on GLES2 the buffer and its attribute layout, every time. The buffer is bound either
    // way (GL_ARRAY_BUFFER isn't VAO state): the billboard BufferData that follows must not land in the last mesh's VBO.
    static void bindVertices( GLuint vao, GLuint vbo, void ( *layout )() ) {
      if ( !s_es2 ) { gl.BindVertexArray( vao ); gl.BindBuffer( GL_ARRAY_BUFFER, vbo ); return; }
      gl.BindBuffer( GL_ARRAY_BUFFER, vbo );
      layout();
    }
    // Textures from ARGB words (BGRA bytes): GLES has no BGRA upload, so swap to RGBA first.
    // GL_EXT_texture_format_BGRA8888 (most Mesa GLES drivers, v3d and vc4 included) takes BGRA as is.
    // sub: the bound texture already has this size, so update it in place instead of reallocating.
    static void uploadSubBGRA( int w, int h, const uint32_t* px ) { uploadBGRA( w, h, px, true ); }
    static void uploadBGRA( int w, int h, const uint32_t* px, bool sub = false ) {
      if ( s_tier == CORE || s_bgraExt ) {
        if ( sub ) gl.TexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, px );
        else gl.TexImage2D( GL_TEXTURE_2D, 0, s_tier == CORE ? GL_RGBA8 : GL_BGRA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, px );
        return;
      }
      static std::vector<uint32_t> tmp;
      tmp.resize( static_cast<std::size_t>( w ) * h );
      for ( std::size_t i = 0; i < tmp.size(); ++i ) {
        const uint32_t c = px[i];
        tmp[i] = ( c & 0xFF00FF00u ) | ( ( c >> 16 ) & 0xFFu ) | ( ( c & 0xFFu ) << 16 );
      }
      if ( sub ) gl.TexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, tmp.data() );
      else gl.TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp.data() );
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

    // GLSL 330 -> GLSL ES 1.00 (GLES2): attribute/varying, texture2D, gl_FragColor, no shadow sampler.
    static auto toEs2( std::string src, bool vertex ) -> std::string {
      auto replaceAll = [&]( const std::string& a, const std::string& b ) {
        for ( std::size_t p = 0; ( p = src.find( a, p ) ) != std::string::npos; p += b.size() ) src.replace( p, a.size(), b );
      };
      replaceAll( "#version 330 core\n", vertex ? "#version 100\nprecision highp float;\n" : "#version 100\nprecision mediump float;\n" );
      for ( int l = 0; l < 5; ++l ) replaceAll( "layout(location = " + std::to_string( l ) + ") ", "" );
      replaceAll( "\nin ", vertex ? "\nattribute " : "\nvarying " );
      replaceAll( "\nout vec4 o;", "" );
      replaceAll( "\nout ", "\nvarying " );
      replaceAll( "texture(", "texture2D(" );
      replaceAll( " o = ", " gl_FragColor = " );
      replaceAll( "uniform sampler2DShadow uShadowMap;", "" );
      // shadow(): sampler2DShadow lookups -> lit (no shadow map on GLES2)
      replaceAll( "return texture2D(uShadowMap, vec3(uv, z));", "return 1.0;" );
      return src;
    }
    // GLSL 330 -> GLSL ES 3.00 (GLES3): the same language, plus default precisions (sampler2DShadow has none).
    static auto toEs3( std::string src, bool vertex ) -> std::string {
      const std::string from = "#version 330 core\n";
      if ( src.rfind( from, 0 ) == 0 )
        src.replace( 0, from.size(), vertex ? "#version 300 es\nprecision highp float;\nprecision highp int;\n"
                                            : "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2DShadow;\n" );
      return src;
    }
    static auto program( const char* vs, const char* fs, bool raw = false, bool* linked = nullptr ) -> GLuint {
      std::string vsEs, fsEs;
      if ( raw ) {}
      else if ( s_es2 ) { vsEs = toEs2( vs, true ); fsEs = toEs2( fs, false ); vs = vsEs.c_str(); fs = fsEs.c_str(); }
      if ( !raw && s_tier == GLES3 ) { vsEs = toEs3( vs, true ); fsEs = toEs3( fs, false ); vs = vsEs.c_str(); fs = fsEs.c_str(); }
      auto compile = []( GLenum type, const char* src ) {
        GLuint s = gl.CreateShader( type );
        gl.ShaderSource( s, 1, &src, nullptr );
        gl.CompileShader( s );
        GLint ok{};
        gl.GetShaderiv( s, GL_COMPILE_STATUS, &ok );
        if ( !ok ) {
          char log[1024];
          gl.GetShaderInfoLog( s, sizeof log, nullptr, log );
          std::fprintf( stderr, "bicycle 3D: %s shader didn't compile: %s\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment", log );
        }
        return s;
      };
      GLuint p = gl.CreateProgram();
      gl.AttachShader( p, compile( GL_VERTEX_SHADER, vs ) );
      gl.AttachShader( p, compile( GL_FRAGMENT_SHADER, fs ) );
      // The attribute locations the 330 shaders give with layout(location): GLES2 binds them before linking.
      const char* attrs[] = { "aPos", "aNrm", "aUv", "aJoints", "aWeights" };
      for ( GLuint a = 0; a < 5; ++a ) gl.BindAttribLocation( p, a, attrs[a] );
      gl.LinkProgram( p );
      GLint ok{};
      gl.GetProgramiv( p, GL_LINK_STATUS, &ok );
      if ( !ok ) {
        char log[1024];
        gl.GetProgramInfoLog( p, sizeof log, nullptr, log );
        std::fprintf( stderr, "bicycle 3D: shader program didn't link: %s\n", log );
      }
      if ( linked ) *linked = ok != 0;
      return p;
    }

    // Texture log lines: on the GLES tiers (small GPUs, where things went missing before) and with BICYCLE_GL_DEBUG.
    static auto texLog() -> bool { return s_tier != CORE || glDebug(); }
    auto loadTexture( const std::string& path ) -> GLuint {
      auto tb = assets::read( path, mem::TEMPORARY );
      if ( !tb ) { std::fprintf( stderr, "bicycle 3D: texture %s: can't read it\n", path.c_str() ); return 0; }
      auto ti = BImg::parse( *tb );
      if ( !ti ) { std::fprintf( stderr, "bicycle 3D: texture %s: not a valid .bimg (%zu bytes)\n", path.c_str(), tb->size() ); return 0; }
      while ( gl.GetError() != 0 ) {}
      const GLuint t = texture( *ti );
      const GLenum err = gl.GetError();
      if ( texLog() || err || !t )
        std::fprintf( stderr, "bicycle 3D: texture %s: %dx%d%s, GL id %u, upload error 0x%04x\n", path.c_str(), ti->w, ti->h,
                      ti->indexed() ? " indexed" : "", t, err );
      return t;
    }
    // Once, at the first frame: every textured material's texture must still be a texture of this context (one made
    // before a context was replaced, or a failed upload, would draw untextured). Missing ones are loaded again.
    void verifyTextures() {
      int ok = 0, fixed = 0, failed = 0;
      for ( auto& [name, g] : _meshes ) {
        for ( auto& r : g->ranges ) {
          if ( r.texPath.empty() ) continue;
          if ( r.tex && gl.IsTexture( r.tex ) ) { ++ok; continue; }
          std::fprintf( stderr, "bicycle 3D: mesh %s: texture %s (GL id %u) isn't a texture of this context; loading it again\n",
                        name.c_str(), r.texPath.c_str(), r.tex );
          r.tex = loadTexture( r.texPath );
          ( r.tex && gl.IsTexture( r.tex ) ? fixed : failed ) += 1;
        }
      }
      if ( texLog() || fixed || failed )
        std::fprintf( stderr, "bicycle 3D: textures at the first frame: %d ok, %d loaded again, %d missing\n", ok, fixed, failed );
    }
    auto texture( const BImg& img ) -> GLuint {
      gfx::Pixels rgba( static_cast<std::size_t>( img.w ) * img.h );
      for ( int y = 0; y < img.h; ++y )
        for ( int x = 0; x < img.w; ++x ) rgba[static_cast<std::size_t>( y ) * img.w + x] = img.pixel( x, y );
      GLuint t{};
      gl.GenTextures( 1, &t );
      texParams( t, GL_REPEAT );
      gl.PixelStorei( GL_UNPACK_ALIGNMENT, 1 );
      gl.TexImage2D( GL_TEXTURE_2D, 0, s_es2 ? GL_RGBA : GL_RGBA8, img.w, img.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data() );
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
        const bool found = assets::exists( gameDir + dir + base );
        if ( !found ) std::fprintf( stderr, "bicycle 3D: %s: material texture %s: no %s; drawn untextured\n", obj.c_str(), mat.texture.c_str(), ( gameDir + dir + base ).c_str() );
        mat.texture = found ? dir + base : "";
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
      const std::size_t nMat = std::max<std::size_t>( 1, m->materials.size() );
      for ( std::size_t mat = 0; mat < nMat; ++mat ) {
        GpuMesh::Range r;
        r.first = static_cast<int>( verts.size() / 16 );
        for ( std::size_t t = 0; t < m->nTris(); ++t ) {
          if ( ( m->triMat.empty() ? 0 : m->triMat[t] ) != mat ) continue;
          for ( int c = 0; c < 3; ++c ) {
            const std::size_t corner = 3 * t + c, v = m->tris[corner];
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
        if ( !skinned ) tessellate( verts, static_cast<std::size_t>( r.first ) * 16 );
        r.count = static_cast<int>( verts.size() / 16 ) - r.first;
        if ( mat < m->materials.size() ) {
          const auto& mm = m->materials[mat];
          r.rgba = mm.rgba;
          if ( !mm.texture.empty() ) {
            r.texPath = "./" + _gameName + "/" + mm.texture;
            r.tex = loadTexture( r.texPath );
          }
          if ( texLog() ) std::fprintf( stderr, "bicycle 3D: mesh %s material %zu: %s (GL texture %u)\n", path.c_str(), mat,
                                        mm.texture.empty() ? "no texture" : mm.texture.c_str(), r.tex );
        }
        else {
          r.rgba = { 0.8f, 0.8f, 0.8f, 1.0f };
        }
        if ( r.count > 0 ) g->ranges.push_back( r );
      }
      {
        m3::Vec3 lo{ 1e30f, 1e30f, 1e30f }, hi{ -1e30f, -1e30f, -1e30f };
        for ( std::size_t v = 0; v < m->nVerts(); ++v ) {
          const m3::Vec3 p{ m->pos[3 * v], m->pos[3 * v + 1], m->pos[3 * v + 2] };
          lo = { std::min( lo.x, p.x ), std::min( lo.y, p.y ), std::min( lo.z, p.z ) };
          hi = { std::max( hi.x, p.x ), std::max( hi.y, p.y ), std::max( hi.z, p.z ) };
        }
        g->center = ( lo + hi ) * 0.5f;
        g->radius = m3::length( hi - lo ) * 0.5f;
      }
      if ( !s_es2 ) gl.GenVertexArrays( 1, &g->vao );
      gl.GenBuffers( 1, &g->vbo );
      if ( !s_es2 ) gl.BindVertexArray( g->vao );
      gl.BindBuffer( GL_ARRAY_BUFFER, g->vbo );
      gl.BufferData( GL_ARRAY_BUFFER, verts.size() * sizeof( float ), verts.data(), GL_STATIC_DRAW );
      meshLayout();
      if ( s_es2 && g->skin ) g->bindVerts.assign( verts.begin(), verts.end() );  // CPU skinning's source
      _meshes[path] = g;
      return g;
    }

    // Large triangles (a level's ground: two 68 m triangles) are split along their longest edge until no edge is
    // longer than MAX_EDGE, and each triangle's uvs are shifted by whole tiles toward 0 (the same texels under
    // GL_REPEAT). Huge triangles crossing the near plane and large texture coordinates are where small GPUs (vc4)
    // interpolate badly; small ones also give per-vertex lighting and fog more samples. verts from `from` on are
    // triangles of 16 floats per vertex (pos3 nrm3 uv2 joints4 weights4).
    template <class V>
    static void tessellate( V& verts, std::size_t from ) {
      constexpr float MAX_EDGE = 6.f;
      constexpr int VF = 16, TF = 3 * VF;
      std::vector<float> out, stack;
      out.reserve( verts.size() - from );
      for ( std::size_t t = from; t + TF <= verts.size(); t += TF ) {
        stack.assign( verts.begin() + t, verts.begin() + t + TF );
        while ( !stack.empty() ) {
          float tri[TF];
          std::copy( stack.end() - TF, stack.end(), tri );
          stack.resize( stack.size() - TF );
          int longest = -1;
          float best = MAX_EDGE * MAX_EDGE;
          for ( int e = 0; e < 3; ++e ) {
            const float* a = tri + VF * e;
            const float* b = tri + VF * ( ( e + 1 ) % 3 );
            const float d = ( a[0] - b[0] ) * ( a[0] - b[0] ) + ( a[1] - b[1] ) * ( a[1] - b[1] ) + ( a[2] - b[2] ) * ( a[2] - b[2] );
            if ( d > best ) { best = d; longest = e; }
          }
          if ( longest < 0 ) {
            const float du = std::floor( ( tri[6] + tri[VF + 6] + tri[2 * VF + 6] ) / 3 );
            const float dv = std::floor( ( tri[7] + tri[VF + 7] + tri[2 * VF + 7] ) / 3 );
            for ( int c = 0; c < 3; ++c ) { tri[VF * c + 6] -= du; tri[VF * c + 7] -= dv; }
            out.insert( out.end(), tri, tri + TF );
            continue;
          }
          const int i0 = longest, i1 = ( longest + 1 ) % 3, i2 = ( longest + 2 ) % 3;
          float mid[VF];
          for ( int k = 0; k < VF; ++k ) mid[k] = 0.5f * ( tri[VF * i0 + k] + tri[VF * i1 + k] );
          const float nl = std::sqrt( mid[3] * mid[3] + mid[4] * mid[4] + mid[5] * mid[5] );
          if ( nl > 0 ) for ( int k = 3; k < 6; ++k ) mid[k] /= nl;
          // (i0, mid, i2) and (mid, i1, i2): the same winding as (i0, i1, i2).
          const float* a = tri + VF * i0;
          const float* b = tri + VF * i1;
          const float* c = tri + VF * i2;
          for ( const float* v : { a, static_cast<const float*>( mid ), c, static_cast<const float*>( mid ), b, c } )
            stack.insert( stack.end(), v, v + VF );
        }
      }
      verts.resize( from );
      verts.insert( verts.end(), out.begin(), out.end() );
    }

    // drawMesh's uniform locations, looked up once per program (not per draw).
    struct MeshLocs { GLint skinned, bones, mvp, model, lit, color, tex, lightMvp; };
    auto meshLocs( GLuint prog ) -> const MeshLocs& {
      auto it = _meshLocs.find( prog );
      if ( it != _meshLocs.end() ) return it->second;
      auto u = [&]( const char* n ) { return gl.GetUniformLocation( prog, n ); };
      if ( u( "uTex" ) < 0 ) std::fprintf( stderr, "bicycle 3D: program %u has no active uTex uniform: meshes draw untextured\n", prog );
      return _meshLocs[prog] = { u( "uSkinned" ), u( "uBones" ), u( "uMvp" ), u( "uModel" ), u( "uLit" ), u( "uColor" ), u( "uTex" ), u( "uLightMvp" ) };
    }
    // GLES2 CPU skinning: the bind-pose vertices blended by their 4 joints' matrices into the mesh's dynamic buffer.
    static void skinOnCpu( const GpuMesh& m, const float* bones ) {
      const auto& src = m.bindVerts;
      m.skinVerts.resize( src.size() );
      const int nj = static_cast<int>( m.skin->joints.size() );
      for ( std::size_t v = 0; v + 16 <= src.size(); v += 16 ) {
        const float* in = &src[v];
        float* out = &m.skinVerts[v];
        float s[16]{};
        for ( int k = 0; k < 4; ++k ) {
          const float w = in[12 + k];
          if ( w == 0 ) continue;
          const int j = std::clamp( static_cast<int>( in[8 + k] ), 0, nj - 1 );
          const float* b = bones + 16 * j;
          for ( int i = 0; i < 16; ++i ) s[i] += w * b[i];
        }
        for ( int r = 0; r < 3; ++r ) {
          out[r] = s[r] * in[0] + s[4 + r] * in[1] + s[8 + r] * in[2] + s[12 + r];
          out[3 + r] = s[r] * in[3] + s[4 + r] * in[4] + s[8 + r] * in[5];
        }
        for ( int i = 6; i < 16; ++i ) out[i] = in[i];
      }
      if ( !m.skinVbo ) gl.GenBuffers( 1, &m.skinVbo );
      gl.BindBuffer( GL_ARRAY_BUFFER, m.skinVbo );
      gl.BufferData( GL_ARRAY_BUFFER, m.skinVerts.size() * sizeof( float ), m.skinVerts.data(), GL_DYNAMIC_DRAW );
      meshLayout();
    }
    void drawMesh( const GpuMesh& m, const Mat4& viewProj, const Mat4& model, const float* bones = nullptr, GLuint prog = 0 ) {
      if ( !prog ) prog = _prog;
      if ( s_es2 && bones && m.skin ) {  // GLES2: skinned on the CPU, or with the small skinned shader
        if ( prog == _prog && _skinProg && m.skin->joints.size() <= static_cast<std::size_t>( ES2_BONES ) ) {
          float rows[4 * 3 * ES2_BONES];
          const std::size_t nj = m.skin->joints.size();
          for ( std::size_t j = 0; j < nj; ++j )
            for ( int r = 0; r < 3; ++r )
              for ( int c = 0; c < 4; ++c ) rows[12 * j + 4 * r + c] = bones[16 * j + 4 * c + r];
          gl.UseProgram( _skinProg );
          gl.Uniform4fv( gl.GetUniformLocation( _skinProg, "uBones" ), static_cast<GLsizei>( 3 * nj ), rows );
          drawRanges( m, meshLocs( _skinProg ), viewProj, model, false, nullptr );
          gl.UseProgram( _prog );
          return;
        }
        drawRanges( m, meshLocs( prog ), viewProj, model, false, bones );
        return;
      }
      drawRanges( m, meshLocs( prog ), viewProj, model, bones && m.skin, nullptr, bones );
    }
    // cpuBones: skin on the CPU first (GLES2); gpuBones: the GL 3.3 / ES 3 shader's uBones.
    void drawRanges( const GpuMesh& m, const MeshLocs& l, const Mat4& viewProj, const Mat4& model, bool gpuSkin,
                     const float* cpuBones, const float* gpuBones = nullptr ) {
      const float* bones = gpuBones;
      const Mat4 mvp = mul( viewProj, model );
      gl.Uniform1i( l.skinned, gpuSkin ? 1 : 0 );
      if ( gpuSkin ) gl.UniformMatrix4fv( l.bones, static_cast<GLsizei>( m.skin->joints.size() ), 0, bones );
      gl.UniformMatrix4fv( l.mvp, 1, 0, mvp.data() );
      gl.UniformMatrix4fv( l.model, 1, 0, model.data() );
      if ( l.lightMvp >= 0 ) gl.UniformMatrix4fv( l.lightMvp, 1, 0, mul( _lightVP, model ).data() );
      gl.Uniform1i( l.lit, 1 );
      if ( cpuBones && !m.bindVerts.empty() ) skinOnCpu( m, cpuBones );
      else bindVertices( m.vao, m.vbo, meshLayout );
      for ( const auto& r : m.ranges ) {
        gl.Uniform4fv( l.color, 1, r.rgba.data() );
        gl.Uniform1i( l.tex, r.tex ? 1 : 0 );
        gl.BindTexture( GL_TEXTURE_2D, r.tex );
        gl.DrawArrays( GL_TRIANGLES, r.first, r.count );
        ++_frameDraws;
      }
    }

    // One soft circle per caster on the ground below it, found with a ray down (the level, and moving platforms). It
    // is largest and darkest at contact and shrinks and fades as the caster rises (gone 6 m up). One draw for all:
    // depth test on, depth writes off, alpha blend, a polygon offset and a 2 cm lift against z-fighting.
    void drawBlobShadows( const Mat4& viewProj, const m3::Vec3& eye ) {
      if ( _blobCasters.empty() || !_blobProg ) return;
      auto& w3 = phys3d::World3D::get();
      constexpr float FADE_HEIGHT = 6.f;
      std::vector<float>& v = _blobVerts;
      v.clear();
      for ( Entity e : _blobCasters ) {
        const auto& t = w3.transform( e );
        const auto& b = w3.body( e );
        float radius = t.shadow;
        float bottom = t.pos.y;  // characters stand on pos; kinematic capsules and boxes are centered on it
        if ( b.kind == phys3d::BOX ) bottom -= b.half.y;
        else if ( b.kind == phys3d::KINEMATIC ) bottom -= 0.5f * b.height;
        if ( radius < 0 ) {  // auto: from the body (a body-less model gets none)
          if ( b.kind == phys3d::NONE ) continue;
          radius = b.kind == phys3d::BOX ? std::max( b.half.x, b.half.z ) : b.radius * 1.6f;
          radius *= std::max( t.scale.x, t.scale.z );
        }
        if ( radius <= 0 || t.shadowAlpha <= 0 ) continue;
        const m3::Vec3 o{ t.pos.x, bottom + 0.05f, t.pos.z };
        phys3d::Hit h = w3.raycast( o, { 0, -1, 0 }, FADE_HEIGHT + 0.05f, e, false );
        const phys3d::Hit he = w3.raycast( o, { 0, -1, 0 }, FADE_HEIGHT + 0.05f, e, true );  // moving platforms too
        if ( he.hit && he.entity && w3.body( he.entity ).kind == phys3d::BOX && ( !h.hit || he.t < h.t ) ) h = he;
        if ( !h.hit ) continue;
        const float height = std::max( 0.0f, bottom - h.point.y );
        const float k = std::clamp( height / FADE_HEIGHT, 0.0f, 1.0f );
        const float r = radius * ( 1.0f - 0.55f * k ), a = t.shadowAlpha * ( 1.0f - k ) * ( 1.0f - k );
        if ( a < 0.01f ) continue;
        const m3::Vec3 n = h.normal;
        const m3::Vec3 ax = m3::normalize( m3::cross( n, std::fabs( n.z ) < 0.9f ? m3::Vec3{ 0, 0, 1 } : m3::Vec3{ 1, 0, 0 } ) );
        const m3::Vec3 az = m3::cross( n, ax );  // counter-clockwise seen from above the ground (back faces are culled)
        // Lifted off the ground by more the farther it is (depth precision falls with distance; 16-bit depth buffers).
        const float lift = 0.02f + 0.006f * m3::length( h.point - eye );
        const m3::Vec3 c = h.point + n * lift;
        auto vtx = [&]( float sx, float sz ) {
          const m3::Vec3 p = c + ax * ( sx * r ) + az * ( sz * r );
          v.insert( v.end(), { p.x, p.y, p.z, a, sx * 0.5f + 0.5f, sz * 0.5f + 0.5f } );
        };
        vtx( -1, -1 ); vtx( 1, -1 ); vtx( 1, 1 );
        vtx( -1, -1 ); vtx( 1, 1 ); vtx( -1, 1 );
      }
      if ( glDebug() && _dbgFrame < 2 ) {
        std::fprintf( stderr, "bicycle 3D: blob shadows: %zu casters, %zu drawn\n", _blobCasters.size(), v.size() / 36 );
      }
      if ( v.empty() ) return;
      gl.UseProgram( _blobProg );
      gl.UniformMatrix4fv( gl.GetUniformLocation( _blobProg, "uMvp" ), 1, 0, viewProj.data() );
      gl.Uniform1i( gl.GetUniformLocation( _blobProg, "tex" ), 0 );
      gl.ActiveTexture( GL_TEXTURE0 );
      gl.BindTexture( GL_TEXTURE_2D, _blobTex );
      bindVertices( _blobVao, _blobVbo, blobLayout );
      gl.BufferData( GL_ARRAY_BUFFER, v.size() * sizeof( float ), v.data(), GL_DYNAMIC_DRAW );
      if ( s_es2 ) blobLayout();
      gl.Enable( GL_BLEND );
      gl.BlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
      gl.DepthMask( 0 );
      gl.Enable( GL_POLYGON_OFFSET_FILL );
      gl.PolygonOffset( -1.0f, -2.0f );

      gl.DrawArrays( GL_TRIANGLES, 0, static_cast<GLsizei>( v.size() / 6 ) );
      ++_frameDraws;
      if ( glDebug() && _dbgFrame < 2 ) glCheck( "the blob shadows" );
      gl.Disable( GL_POLYGON_OFFSET_FILL );
      gl.DepthMask( 1 );
      gl.Disable( GL_BLEND );
      gl.UseProgram( _prog );
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
      if ( _skinProg ) {
        gl.UseProgram( _skinProg );
        gl.Uniform1i( gl.GetUniformLocation( _skinProg, "uWorld3D" ), 0 );
      }
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
          uploadBGRA( _atlas.width(), _atlas.height(), reinterpret_cast<const uint32_t*>( _atlas.pixels().data() ) );
          _atlasTexVersion = _atlas.version();
        }
        bindVertices( _quadVao, _quadVbo, vertexLayout );
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
      if ( _skinProg ) {
        gl.UseProgram( _skinProg );
        setWorldUniforms( _skinProg, v, eye, lightVP, shadows );
        gl.UseProgram( _prog );
      }
      setWorldUniforms( _prog, v, eye, lightVP, shadows );
      _dbgEye = eye;
      _lightVP = lightVP;
    }
    void setWorldUniforms( GLuint prog, const phys3d::View& v, const m3::Vec3& eye, const m3::Mat4& lightVP, bool shadows ) {
      auto u = [&]( const char* n ) { return gl.GetUniformLocation( prog, n ); };
      gl.Uniform1i( u( "uWorld3D" ), 1 );
      const m3::Vec3 ld = m3::normalize( v.lightDir );
      gl.Uniform3fv( u( "uLightDir" ), 1, &ld.x );
      gl.Uniform3fv( u( "uLightColor" ), 1, &v.lightColor.x );
      gl.Uniform1f( u( "uAmbient" ), v.ambient );
      gl.Uniform1i( u( "uShadows" ), shadows ? 1 : 0 );
      gl.Uniform1i( u( "uShadowMap" ), 1 );
      gl.Uniform1f( u( "uShadowTexel" ), 1.0f / std::max( 1, _shadowSize ) );
      gl.Uniform1i( u( "uShadowTaps" ), QUALITY[_quality].taps );
      gl.Uniform3fv( u( "uCamPos" ), 1, &eye.x );
      gl.Uniform3fv( u( "uFogColor" ), 1, &v.fogColor.x );
      const float fog[2] = { v.fogNear, v.fogFar };
      gl.Uniform2fv( u( "uFog" ), 1, fog );
      gl.UniformMatrix4fv( u( "uLightVP" ), 1, 0, lightVP.data() );
      gl.Uniform1i( u( "uView" ), s_glView );
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
      if ( !_texVerified ) { _texVerified = true; verifyTextures(); }

      // What to draw: the level's static meshes, then every visible 3D entity's model (posed if rigged).
      _items.clear();
      for ( const auto& st : _statics ) _items.push_back( { st.get(), m3::identity(), false, nullptr } );
      std::size_t nPosed = 0;
      _blobCasters.clear();
      _models.forEach( [&]( Entity entity, std::shared_ptr<GpuMesh>& mesh ) {
        const auto& t = w3.transform( entity );
        if ( !t.on || !t.visible ) return;
        if ( t.shadow != 0 ) _blobCasters.push_back( entity );
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

      // Shadow pass: an orthographic light view around what the camera looks at. Quality (BICYCLE_SHADOWS, else
      // W.View.shadows, else auto by GPU) picks the map size, PCF taps and how far it reaches. The map is re-rendered
      // only when the light view or a caster changed: level statics go into their own map (redrawn only when the light
      // view moves) that is copied in before the moving casters are drawn.
      _quality = _envQuality >= 0 ? _envQuality : v.shadowQuality >= 0 ? v.shadowQuality : _autoQuality;
      if ( !v.shadows ) _quality = Q_OFF;
      const Quality& Q = QUALITY[_quality];
      const bool shadows = _quality != Q_OFF && _shadowOk && ensureShadowMap( Q.size );
      if ( s_es2 && _plainProg ) _prog = shadows && _shadowProg ? _shadowProg : _plainProg;
      m3::Mat4 lightVP = m3::identity();
      if ( shadows ) {
        const m3::Vec3 ld = m3::normalize( v.lightDir );
        const m3::Vec3 up = std::fabs( ld.y ) > 0.99f ? m3::Vec3{ 0, 0, 1 } : m3::Vec3{ 0, 1, 0 };
        m3::Vec3 center;
        float ext = SHADOW_EXTENT;
        if ( Q.reach <= 0 ) {  // high: a fixed square around the focus (the player, else ahead of the camera)
          const phys3d::Transform* focus = w3.rig().target ? &w3.transform( w3.rig().target ) : nullptr;
          center = focus && focus->on ? focus->pos + fwd * 6.f : eye + fwd * 12.f;
        }
        else {  // low/med: the bounding sphere of the camera frustum from the eye out to the reach (and the fog)
          const float reach = std::min( { Q.reach, v.farZ, std::max( v.fogFar, 1.f ) } );
          const float ty = std::tan( v.fovDeg * m3::PI / 360.f ), tx = ty * static_cast<float>( vw ) / std::max( 1, vh );
          // Sphere through the far corners, centered on the view axis (radius independent of the camera's turn).
          const float far2 = reach * reach * ( 1 + tx * tx + ty * ty );
          const float d = std::min( reach, far2 / ( 2 * reach ) );
          center = eye + fwd * d;
          ext = std::sqrt( std::max( far2 - 2 * d * reach + d * d, d * d ) );
          ext = std::ceil( ext * 2 ) / 2;  // steady radius: half-meter steps
        }
        // Snap the center to shadow-map texels so the shadows don't shimmer as the camera moves.
        const m3::Mat4 lv0 = m3::lookAt( { 0, 0, 0 }, ld, up );
        m3::Vec3 lc = m3::transformPoint( lv0, center );
        const float texel = 2 * ext / Q.size;
        lc.x = std::floor( lc.x / texel ) * texel;
        lc.y = std::floor( lc.y / texel ) * texel;
        const m3::Mat4 lightProj = m3::ortho( lc.x - ext, lc.x + ext, lc.y - ext, lc.y + ext, -lc.z - 80, -lc.z + 80 );
        lightVP = m3::mul( lightProj, lv0 );
        const m3::Planes lp = m3::frustumPlanes( lightVP );
        // What changed since the map was drawn: the light view (statics), the moving casters (pose included).
        auto fnv = []( uint64_t h, const void* p, std::size_t n ) {
          for ( std::size_t i = 0; i < n; ++i ) h = ( h ^ static_cast<const unsigned char*>( p )[i] ) * 1099511628211ull;
          return h;
        };
        const uint64_t viewHash = fnv( 1469598103934665603ull ^ static_cast<uint64_t>( Q.size ), lightVP.data(), 16 * sizeof( float ) );
        uint64_t dynHash = viewHash;
        _casters.clear();
        const std::size_t nStatic = _statics.size();
        for ( std::size_t i = 0; i < _items.size(); ++i ) {
          const auto& it = _items[i];
          m3::Vec3 c;
          float r;
          worldSphere( it, c, r );
          if ( !m3::sphereVisible( lp, c, r ) ) continue;
          if ( i >= nStatic ) {
            dynHash = fnv( dynHash, &it.mesh, sizeof it.mesh );
            dynHash = fnv( dynHash, it.model.data(), 16 * sizeof( float ) );
            if ( it.skinned && it.mesh->skin ) dynHash = fnv( dynHash, it.bones->data(), it.mesh->skin->joints.size() * 16 * sizeof( float ) );
          }
          _casters.push_back( i );
        }
        const bool split = _staticFbo != 0;  // a separate statics map (GL 3.3 / ES 3.0 blit)
        const bool staticsDirty = !_shadowValid || viewHash != _shadowViewHash;
        const bool dirty = staticsDirty || dynHash != _shadowDynHash;
        if ( dirty ) {
          ++_fpsShadowRenders;
          gl.Enable( GL_DEPTH_TEST );
          gl.DepthFunc( GL_LEQUAL );
          gl.Enable( GL_POLYGON_OFFSET_FILL );
          gl.PolygonOffset( 1.5f, 3.0f );
          gl.UseProgram( _depthProg );
          gl.ActiveTexture( GL_TEXTURE0 );
          gl.Uniform1i( gl.GetUniformLocation( _depthProg, "tex" ), 0 );
          gl.Viewport( 0, 0, Q.size, Q.size );
          auto drawCasters = [&]( bool statics, bool dynamics ) {
            for ( std::size_t i : _casters ) {
              const bool st = i < nStatic;
              if ( ( st && !statics ) || ( !st && !dynamics ) ) continue;
              const auto& it = _items[i];
              drawMesh( *it.mesh, lightVP, it.model, it.skinned ? it.bones->data() : nullptr, _depthProg );
            }
          };
          if ( split ) {
            if ( staticsDirty ) {
              ++_fpsStaticRenders;
              gl.BindFramebuffer( GL_FRAMEBUFFER, _staticFbo );
              gl.Clear( GL_DEPTH_BUFFER_BIT );
              drawCasters( true, false );
            }
            gl.BindFramebuffer( GL_READ_FRAMEBUFFER, _staticFbo );
            gl.BindFramebuffer( GL_DRAW_FRAMEBUFFER, _shadowFbo );
            gl.BlitFramebuffer( 0, 0, Q.size, Q.size, 0, 0, Q.size, Q.size, GL_DEPTH_BUFFER_BIT, GL_NEAREST );
            gl.BindFramebuffer( GL_FRAMEBUFFER, _shadowFbo );
            drawCasters( false, true );
          }
          else {
            gl.BindFramebuffer( GL_FRAMEBUFFER, _shadowFbo );
            gl.Clear( GL_DEPTH_BUFFER_BIT );
            drawCasters( true, true );
          }
          gl.Disable( GL_POLYGON_OFFSET_FILL );
          gl.BindFramebuffer( GL_FRAMEBUFFER, 0 );
          _shadowValid = true;
          _shadowViewHash = viewHash;
          _shadowDynHash = dynHash;
        }
      }
      if ( glDebug() && _dbgFrame < 2 ) glCheck( "the shadow pass" );
      fpsMark( P_SHADOW );

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

      // Blob shadows where there's no shadow map (BICYCLE_BLOB_SHADOWS=1 adds them to shadow maps, =0 turns them off).
      if ( s_blobs == 1 || ( s_blobs < 0 && !shadows ) ) drawBlobShadows( viewProj, eye );
      if ( glDebug() && _dbgFrame < 2 ) glCheck( "the meshes" );
      fpsMark( P_MAIN );
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
          uploadBGRA( _atlas.width(), _atlas.height(), reinterpret_cast<const uint32_t*>( _atlas.pixels().data() ) );
          _atlasTexVersion = _atlas.version();
        }
        bindVertices( _quadVao, _quadVbo, vertexLayout );
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
        ++_frameDraws;
        _bbVerts.clear();
      }
      fpsMark( P_BILLBOARDS );
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
      const bool same = _overlayW == _fbW && _overlayH == _fbH;
      float scale[2] = { 1, 1 };
      if ( s_npot ) uploadBGRA( _fbW, _fbH, reinterpret_cast<const uint32_t*>( _fb.data() ), same );  // ARGB words
      else {
        auto pot = []( int v ) { int p = 1; while ( p < v ) p <<= 1; return p; };
        const int tw = pot( _fbW ), th = pot( _fbH );
        if ( !same ) {  // allocate the power-of-two texture (contents come from the sub-upload)
          std::vector<uint32_t> zero( static_cast<std::size_t>( tw ) * th, 0 );
          uploadBGRA( tw, th, zero.data() );
        }
        uploadSubBGRA( _fbW, _fbH, reinterpret_cast<const uint32_t*>( _fb.data() ) );
        scale[0] = static_cast<float>( _fbW ) / tw;
        scale[1] = static_cast<float>( _fbH ) / th;
      }
      _overlayW = _fbW;
      _overlayH = _fbH;
      gl.Uniform2fv( gl.GetUniformLocation( _overlayProg, "uUvScale" ), 1, scale );
      gl.Uniform1i( gl.GetUniformLocation( _overlayProg, "tex" ), 0 );
      bindVertices( _overlayVao, _overlayVbo, overlayLayout );
      gl.DrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    }

    void readBack() {
      gfx::Pixels px( static_cast<std::size_t>( _fbW ) * _fbH );
      gl.PixelStorei( GL_PACK_ALIGNMENT, 4 );
      gl.ReadPixels( 0, 0, _fbW, _fbH, s_tier != CORE ? GL_RGBA : GL_BGRA, GL_UNSIGNED_BYTE, px.data() );
      if ( s_tier != CORE ) for ( auto& c : px ) c = ( c & 0xFF00FF00u ) | ( ( c >> 16 ) & 0xFFu ) | ( ( c & 0xFFu ) << 16 );
      _capture.resize( px.size() );
      for ( int y = 0; y < _fbH; ++y )  // GL rows are bottom first
        for ( int x = 0; x < _fbW; ++x ) _capture[static_cast<std::size_t>( y ) * _fbW + x] = px[static_cast<std::size_t>( _fbH - 1 - y ) * _fbW + x] | 0xFF000000u;
    }

    static GL gl;
    static inline int s_tier = CORE;   // context tier (see contextAttributes)
    static inline bool s_es2 = false;  // OpenGL ES 2.0 context (see contextAttributes)
    static inline bool s_npot = true;  // non-power-of-two textures are complete (all but strict GLES2)
    SDL_GLContext _ctx{};
    // Shadow quality: map size, depth compares per pixel, reach (meters; 0 = high's fixed square around the focus).
    enum { Q_OFF, Q_LOW, Q_MED, Q_HIGH };
    struct Quality { int size, taps; float reach; const char* name; };
    static constexpr Quality QUALITY[4] = { { 0, 0, 0, "off" }, { 512, 1, 16, "low" }, { 1024, 4, 24, "med" }, { 2048, 9, 0, "high" } };
    static auto qualityName( int q ) -> const char* { return QUALITY[q].name; }
    static auto parseQuality( const char* e ) -> int {
      if ( !e || !*e ) return -1;
      for ( int q = 0; q < 4; ++q ) if ( std::strcmp( e, QUALITY[q].name ) == 0 ) return q;
      if ( std::strcmp( e, "0" ) == 0 ) return Q_OFF;
      if ( std::strcmp( e, "auto" ) != 0 ) std::fprintf( stderr, "bicycle 3D: BICYCLE_SHADOWS=%s unknown (off|low|med|high|auto)\n", e );
      return -1;
    }
    // The depth texture(s) at this size; on ES 16-bit depth. A second statics-only map where depth blits work.
    auto ensureShadowMap( int size ) -> bool {
      if ( size == _shadowSize ) return _shadowOk;
      const bool es = s_tier != CORE;
      auto make = [&]( GLuint& tex, GLuint& fbo, bool compare ) {
        if ( s_es2 ) compare = false;  // OES_depth_texture: sampled as a plain texture, compared in the shader
        if ( !tex ) gl.GenTextures( 1, &tex );
        if ( !fbo ) gl.GenFramebuffers( 1, &fbo );
        gl.BindTexture( GL_TEXTURE_2D, tex );
        gl.TexImage2D( GL_TEXTURE_2D, 0, s_es2 ? GL_DEPTH_COMPONENT : es ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT,
                       es ? GL_UNSIGNED_SHORT : GL_FLOAT, nullptr );
        gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, compare ? GL_LINEAR : GL_NEAREST );
        gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, compare ? GL_LINEAR : GL_NEAREST );
        // ES 3.0 has no border clamp; the shader treats lookups outside the map as lit anyway.
        const GLenum wrap = es ? GL_CLAMP_TO_EDGE : GL_CLAMP_TO_BORDER;
        gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap );
        gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap );
        const float border[4] = { 1, 1, 1, 1 };
        if ( !es ) gl.TexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border );
        if ( compare ) {
          gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
          gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
        }
        gl.BindFramebuffer( GL_FRAMEBUFFER, fbo );
        gl.FramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0 );
        if ( !es ) {  // ES 3.0: a depth-only framebuffer is complete as is
          gl.DrawBuffer( GL_NONE );
          gl.ReadBuffer( GL_NONE );
        }
        bool ok = gl.CheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE;
        if ( !ok && s_es2 ) {  // some GLES2 drivers want a color attachment too: a small 565 one, never read
          constexpr GLenum RENDERBUFFER = 0x8D41, COLOR_ATTACHMENT0 = 0x8CE0, RGB565 = 0x8D62;
          if ( !_shadowColorRb ) gl.GenRenderbuffers( 1, &_shadowColorRb );
          gl.BindRenderbuffer( RENDERBUFFER, _shadowColorRb );
          gl.RenderbufferStorage( RENDERBUFFER, RGB565, size, size );
          gl.FramebufferRenderbuffer( GL_FRAMEBUFFER, COLOR_ATTACHMENT0, RENDERBUFFER, _shadowColorRb );
          ok = gl.CheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE;
          std::fprintf( stderr, "bicycle 3D: GLES2 shadow map: depth-only framebuffer incomplete; with a color buffer: %s\n", ok ? "ok" : "still incomplete" );
        }
        gl.BindFramebuffer( GL_FRAMEBUFFER, 0 );
        gl.BindTexture( GL_TEXTURE_2D, 0 );
        return ok;
      };
      _shadowOk = make( _shadowTex, _shadowFbo, true );
      if ( !_shadowOk ) std::fprintf( stderr, "bicycle 3D: no shadow map framebuffer; shadows off\n" );
      if ( _shadowOk && !s_es2 && gl.BlitFramebuffer && !make( _staticTex, _staticFbo, false ) ) _staticFbo = 0;
      _shadowSize = size;
      _shadowValid = false;
      return _shadowOk;
    }
    // BICYCLE_FPS=1: once a second, the average fps and ms per pass (glFinish at each pass end, so the numbers are
    // GPU time too; that costs a little, so it's only on when asked).
    enum { P_SHADOW, P_MAIN, P_BILLBOARDS, P_OVERLAY, P_SWAP, P_COUNT };
    void fpsBegin() {
      if ( !s_fps ) return;
      _fpsLast = SDL_GetPerformanceCounter();
      if ( !_fpsStart ) _fpsStart = _fpsLast;
    }
    void fpsMark( int pass ) {
      if ( !s_fps || !_fpsLast ) return;
      gl.Finish();
      const uint64_t t = SDL_GetPerformanceCounter();
      _fpsMs[pass] += 1000.0 * static_cast<double>( t - _fpsLast ) / static_cast<double>( SDL_GetPerformanceFrequency() );
      _fpsLast = t;
    }
    void fpsEnd() {
      if ( !s_fps ) return;
      ++_fpsFrames;
      _fpsDraws += _frameDraws;
      _frameDraws = 0;
      const double el = static_cast<double>( SDL_GetPerformanceCounter() - _fpsStart ) / static_cast<double>( SDL_GetPerformanceFrequency() );
      if ( el < 1.0 ) return;
      const double n = static_cast<double>( _fpsFrames );
      std::fprintf( stderr, "fps %.1f | ms/frame: shadow %.2f main %.2f billboards %.2f overlay %.2f swap %.2f | shadow maps drawn %d/%d (statics %d) | draws %.0f | shadows %s | %s\n",
                    n / el, _fpsMs[P_SHADOW] / n, _fpsMs[P_MAIN] / n, _fpsMs[P_BILLBOARDS] / n, _fpsMs[P_OVERLAY] / n, _fpsMs[P_SWAP] / n,
                    _fpsShadowRenders, _fpsFrames, _fpsStaticRenders, static_cast<double>( _fpsDraws ) / n, _shadowOk ? qualityName( _quality ) : "none",
                    tierName( s_tier ) );
      _fpsFrames = 0;
      _fpsDraws = 0;
      _fpsShadowRenders = 0;
      _fpsStaticRenders = 0;
      for ( auto& m : _fpsMs ) m = 0;
      _fpsStart = SDL_GetPerformanceCounter();
    }
    static inline bool s_fps = false;
    static inline bool s_bgraExt = false;  // GLES: GL_EXT_texture_format_BGRA8888
    uint64_t _fpsStart{}, _fpsLast{};
    double _fpsMs[P_COUNT]{};
    int _fpsFrames{}, _fpsShadowRenders{}, _fpsStaticRenders{};
    std::size_t _fpsDraws{}, _frameDraws{};
    int _quality{ Q_HIGH }, _autoQuality{ Q_HIGH }, _envQuality{ -1 }, _shadowSize{};
    bool _shadowValid{};
    uint64_t _shadowViewHash{}, _shadowDynHash{};
    GLuint _staticTex{}, _staticFbo{};
    std::vector<std::size_t> _casters;
    std::unordered_map<GLuint, MeshLocs> _meshLocs;
    int _overlayW{}, _overlayH{};
    static constexpr float SHADOW_EXTENT = 22.f;  // meters each side of the shadow focus
    std::vector<Item> _items;
    std::vector<std::array<float, 16 * skin::MAX_JOINTS>> _poses;
    GLuint _depthProg{}, _shadowTex{}, _shadowFbo{};
    GLuint _plainProg{}, _shadowProg{}, _shadowColorRb{};
    GLuint _blobProg{}, _blobVao{}, _blobVbo{}, _blobTex{};
    std::vector<Entity> _blobCasters;
    bool _texVerified{};
    std::vector<float> _blobVerts;
    static inline int s_blobs = -1;  // BICYCLE_BLOB_SHADOWS: -1 when there's no shadow map, 1 always, 0 never
    m3::Mat4 _lightVP = m3::identity();  // this frame's light view-projection (GLES2: uLightMvp per draw)
    static inline int s_glView = [] {    // BICYCLE_GL_VIEW (GLES2 shader): a stage on its own, for diagnosis
      const char* e = std::getenv( "BICYCLE_GL_VIEW" );
      if ( !e ) return 0;
      static const char* const names[5] = { "", "uv", "tex", "lit", "fog" };
      for ( int i = 1; i < 5; ++i ) if ( std::strcmp( e, names[i] ) == 0 ) return i;
      return 0;
    }();
    bool _shadowOk{};
    struct Stats3D { std::size_t meshes{}, culled{}, billboards{}; } _stats3d;
    GLuint _skinProg{};
    int _dbgFrame{};
    m3::Vec3 _dbgEye{};  // GLES2: the skinned shader (BICYCLE_GLES2_GPU_SKIN=1), else 0 (CPU skinning)
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
    struct { std::size_t billboardDraws{}, billboards{}, skinnedDraws{}; } _stats;
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
