ffi = require("ffi")
ffi.cdef[[
typedef struct {
  int i;
  double k;
  char awer;
} ABC;

ABC a;
]]

-- local a = ffi.new("ABC")
-- a.i = 45
print(a.i)
print(a.k)
print(a.awer)
