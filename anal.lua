os = require("os")

function show(w)
  for k, v in ipairs(w) do
    print(v)
  end
end

show(os)
