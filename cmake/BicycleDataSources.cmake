# Asset code, relative to the repository root; included by CMakeLists.txt and tools/pack/CMakeLists.txt (which
# also builds standalone for the host when cross-compiling), so the two lists can't drift apart.
# Runtime: loading, inflating (tinfl) and decoding the asset formats. The engine links only these.
set( BICYCLE_DATA_SOURCES
  src/data/tinfl.c src/data/Mem.cpp src/data/Pack.cpp src/data/Obj.cpp src/data/Atlas.cpp src/data/Skin.cpp
  src/data/Strip.cpp src/data/BImg.cpp src/data/Mesh.cpp src/data/EdgebreakerDecode.cpp )
# Offline: deflate (miniz) and every encoder/writer. Only build-time tools and the codec tests link these;
# the engine never does, so none of it adds to the executable.
set( BICYCLE_CODEC_ENCODE_SOURCES
  tools/codec/Deflate.cpp tools/codec/StripEncode.cpp tools/codec/MeshEncode.cpp tools/codec/EdgebreakerEncode.cpp
  tools/codec/SkinEncode.cpp tools/codec/BImgEncode.cpp )
