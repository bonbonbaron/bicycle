# bicycle-pack, the asset packer (see pack.cpp), and the offline codec library it shares with the codec tests.
# Built as part of the main build, or on its own for the host when cross-compiling (the main CMakeLists.txt does
# that): set MINIZ_DIR.
cmake_minimum_required( VERSION 3.21 )
if ( NOT DEFINED BICYCLE_ROOT )
  project( bicycle-pack C CXX )
  set( CMAKE_CXX_STANDARD 20 )
  set( CMAKE_CXX_STANDARD_REQUIRED ON )
  get_filename_component( BICYCLE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE )
endif()
include( "${BICYCLE_ROOT}/cmake/BicycleDataSources.cmake" )
list( TRANSFORM BICYCLE_DATA_SOURCES PREPEND "${BICYCLE_ROOT}/" OUTPUT_VARIABLE toolsDataSources )
list( TRANSFORM BICYCLE_CODEC_ENCODE_SOURCES PREPEND "${BICYCLE_ROOT}/" OUTPUT_VARIABLE encodeSources )
# The runtime decoders, compiled again for the tools (with this build's compilers: the host's when cross-compiling).
add_library( bicycle_tools_data STATIC ${toolsDataSources} )
target_include_directories( bicycle_tools_data PUBLIC ${BICYCLE_ROOT}/include )
# Offline only: deflate and the encoders (tools/codec), on top of the decoders they round-trip against.
add_library( bicycle_codecs_encode STATIC ${MINIZ_DIR}/miniz.c ${encodeSources} )
target_include_directories( bicycle_codecs_encode PUBLIC ${BICYCLE_ROOT}/tools PRIVATE ${MINIZ_DIR} )
target_link_libraries( bicycle_codecs_encode PUBLIC bicycle_tools_data )
set_source_files_properties( ${MINIZ_DIR}/miniz.c PROPERTIES COMPILE_OPTIONS "$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-w>" )
add_executable( bicycle-pack ${BICYCLE_ROOT}/tools/pack/pack.cpp )
target_link_libraries( bicycle-pack PRIVATE bicycle_codecs_encode )
if ( MINGW )
  target_link_options( bicycle-pack PRIVATE -static )
endif()
