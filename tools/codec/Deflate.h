#pragma once
// zlib-format deflate for the packer (miniz, fetched at configure time). The engine only inflates (tinfl).
#include <string>
auto deflateZlib( const std::string& raw ) -> std::string;
