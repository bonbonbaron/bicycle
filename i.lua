
-- metatable = __somekey = metavalue or metamethod
a = { 
  b = 2,
  __add = function(c) return c end
}

c = 25
print(getmetatable(c))
