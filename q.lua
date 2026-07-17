ffi = require("ffi")
ffi.cdef[[
  typedef struct {
    int i;
    double k;
    char awer;
  } ABC;

  extern ABC a;

  typedef struct {
    ABC* arr;
    unsigned nElems;
  } Arr;

  Arr abcs;
]]

-- local a = ffi.new("ABC")
-- a.i = 45
print(ffi.C.a.i)
print(ffi.C.a.k)
print(ffi.C.a.awer)

for i = 0, ffi.C.abcs.nElems - 1 do
  print( "abcs["..i.."] = "..ffi.C.abcs.arr[i].i )
end
