--[[
  gentable.lua
  Writes a Lua file that defines a table supporting:
  - Key-value pairs (with automatic type handling)
  - Array-style (value-only) tables
  - Nested key-value tables
]]
require("debug")
local gen = {}

local function serialize_value(filenamePrefix, val, dbg, indent)
  indent = indent or ""
  local t = type(val)

  if t == "nil" then
    return "nil"
  elseif t == "boolean" then
    return tostring(val)
  elseif t == "number" then
    -- Preserve integer appearance when possible
    if math.type and math.type(val) == "integer" then
      return string.format("%d", val)
    else
      return string.format("%.14g", val)
    end
  elseif t == "string" then
    -- Escape special characters and wrap in double quotes
    return string.format("%q", val)
  elseif t == "table" then
    return gen.serialize_table(filenamePrefix, val, dbg, indent)
  else
    error(debug.traceback().."\nUnsupported type: " .. t)
  end
end

function gen.serialize_table (filenamePrefix, tbl, dbg, indent)
  local arg1type = type(filenamePrefix)
  local arg2type = type(tbl)
  if arg1type ~= "string" then
    error("gen.serialize_table(): arg 1 is supposed to be a string, got a "..arg1type.."\n"..debug.traceback())
  elseif arg2type ~= "table" then
    error("gen.serialize_table(): arg 2 is supposed to be a table, got a "..arg2type.."\n"..debug.traceback())
  end
  indent = indent or ""
  local next_indent = indent .. "  "
  local parts = {}
  filenamePrefix = filenamePrefix:gsub(" ", "_")

  -- Determine if the table is a pure array (consecutive integer keys starting at 1)
  local is_array = true
  local max_index = 0
  local count = 0

  for k, _ in pairs(tbl) do
    count = count + 1
    if type(k) ~= "number" or k < 1 or math.floor(k) ~= k then
      is_array = false
    else
      if k > max_index then max_index = k end
    end
  end

  if is_array and count == max_index then
    -- Pure array: write values only
    for i = 1, max_index do
      local val = serialize_value(filenamePrefix, tbl[i], dbg, next_indent)
      if val == nil then
        error(debug.traceback("got a nil value"))
      end
      table.insert(parts, next_indent .. serialize_value(filenamePrefix, tbl[i], dbg, next_indent))
    end
  else
    -- Dictionary / mixed table: write key = value
    for k, v in pairs(tbl) do
      local key_str
      if type(k) == "string" and k:match("^[%a_][%w_]*$") then
        -- Valid identifier → write without brackets
        key_str = k
      else
        -- Otherwise use [key] syntax
        key_str = "[" .. serialize_value(filenamePrefix, k, dbg, next_indent) .. "]"
      end
      table.insert(parts, next_indent .. key_str .. " = " .. serialize_value(filenamePrefix, v, dbg, next_indent))
    end
  end

  if #parts == 0 then
    if dbg then
      print("returning empty")
    end
    return "{}"
  end

  local table_literal = "{\n" .. table.concat(parts, ",\n") .. "\n" .. indent .. "}"
  if indent ~= "" then
    return table_literal
  end

  local contents = "--auto-generated table by generate_table.lua\n\nlocal "..filenamePrefix.." = " .. table_literal .. "\n\nreturn "..filenamePrefix
  -- Write the file
  local outfile = filenamePrefix..".lua"
  -- TODO figure out how to configure where this goes in game engine context
  local f, err = io.open("C:\\Users\\michael\\AppData\\Roaming\\Aseprite\\scripts\\output\\"..outfile, "w")
  if not f then
    error("Could not open " .. outfile .. " for writing: " .. tostring(err))
  end
  f:write(contents)
  f:close()
  
  if dbg then
    print("Successfully wrote " .. outfile)
  end
  
end  -- function gen.serialize_table

return gen