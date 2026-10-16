# bicycle-pack, the asset packer (see pack.cpp). Built as part of the main build, or on its own for the host when
# cross-compiling (the main CMakeLists.txt does that): set BICYCLE_ROOT and MINIZ_DIR.
cmake_minimum_required( VERSION 3.21 )
if ( NOT DEFINED BICYCLE_ROOT )
  project( bicycle-pack C CXX )
  set( CMAKE_CXX_STANDARD 20 )
  set( CMAKE_CXX_STANDARD_REQUIRED ON )
  get_filename_component( BICYCLE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE )
endif()
add_library( bicycle_pack_codecs STATIC
  ${MINIZ_DIR}/miniz.c
  ${BICYCLE_ROOT}/tools/pack/Deflate.cpp ${BICYCLE_ROOT}/tools/pack/StripEncode.cpp ${BICYCLE_ROOT}/tools/pack/MeshEncode.cpp
  ${BICYCLE_ROOT}/src/data/tinfl.c ${BICYCLE_ROOT}/src/data/Pack.cpp ${BICYCLE_ROOT}/src/data/Strip.cpp
  ${BICYCLE_ROOT}/src/data/BImg.cpp ${BICYCLE_ROOT}/src/data/Mesh.cpp ${BICYCLE_ROOT}/src/data/Edgebreaker.cpp )
target_include_directories( bicycle_pack_codecs PUBLIC ${BICYCLE_ROOT}/include ${MINIZ_DIR} )
set_source_files_properties( ${MINIZ_DIR}/miniz.c PROPERTIES COMPILE_OPTIONS "$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-w>" )
add_executable( bicycle-pack ${BICYCLE_ROOT}/tools/pack/pack.cpp )
target_link_libraries( bicycle-pack PRIVATE bicycle_pack_codecs )
if ( MINGW )
  target_link_options( bicycle-pack PRIVATE -static )
endif()
