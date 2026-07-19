require("io")
require("table")

-- Open a pipe to read ("r") from the command
local handle = io.popen("ls *.l")

if not handle then
  error("failed to execute ls")
end

fileList = {}
for line in handle:lines() do
  -- Insert each line into the end of our table
  table.insert(fileList, line)
end

-- Read the entire output of the command
local files = handle:read("*all")

-- Always close the handle
handle:close()

print(files)

for k, v in pairs(fileList) do
  print("i love "..tostring(v))
end

