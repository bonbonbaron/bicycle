--[[
  Pixel Art to ASCII (grayscale)
  Aseprite Lua script.

  Install:
    File > Scripts > Open Scripts Folder
    Copy this file there, then File > Scripts > pixel_art_to_ascii

  Conversion:
    Each source pixel becomes one character. Luminance is mapped onto a
    character ramp. Color is ignored. Transparent pixels are treated as
    white (light) unless invert is enabled.

  The core functions (luminance_of, pixel_to_char, image_to_ascii) can
  also be required or copied into other Lua tools if you already have
  pixel data.
]]

local ascii = {}
local RAMPS = {
  classic  = "@%#*+=-:. ",
  simple   = "@#*+=-. ",
  blocks   = "█▓▒░ ",
  detailed = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ",
}

local RAMP_LABELS = {
  "classic",
  "simple",
  "blocks",
  "detailed",
}

local function clamp(n, lo, hi)
  if n < lo then return lo end
  if n > hi then return hi end
  return n
end

-- Rec. 601 luma. Input channels are 0..255.
local function luminance_of(r, g, b)
  return 0.299 * r + 0.587 * g + 0.114 * b
end

local function pixel_to_char(luma, ramp, invert)
  local n = #ramp
  if n < 1 then
    return " "
  end
  local t = clamp(luma, 0, 255) / 255
  if invert then
    t = 1 - t
  end
  -- Dark (t near 0) maps to the first (densest) character.
  local index = math.floor(t * (n - 1) + 0.5) + 1
  index = clamp(index, 1, n)
  return ramp:sub(index, index)
end

local function rgba_from_pixel(sprite, image, pixel)
  local mode = image.colorMode
  if mode == ColorMode.RGB then
    return app.pixelColor.rgbaR(pixel),
           app.pixelColor.rgbaG(pixel),
           app.pixelColor.rgbaB(pixel),
           app.pixelColor.rgbaA(pixel)
  elseif mode == ColorMode.GRAY then
    local v = app.pixelColor.grayaV(pixel)
    local a = app.pixelColor.grayaA(pixel)
    return v, v, v, a
  elseif mode == ColorMode.INDEXED then
    local idx = pixel
    -- Aseprite uses 0 as the conventional transparent index on many sprites.
    if sprite.transparentColor ~= nil and idx == sprite.transparentColor then
      return 255, 255, 255, 0
    end
    local pal = sprite.palettes[1]
    if pal == nil or idx < 0 or idx >= #pal then
      return 0, 0, 0, 0
    end
    local c = pal:getColor(idx)
    return c.red, c.green, c.blue, c.alpha
  end
  return 0, 0, 0, 0
end

-- Flatten a pixel against white using its alpha.
local function flatten_on_white(r, g, b, a)
  if a >= 255 then
    return r, g, b
  end
  if a <= 0 then
    return 255, 255, 255
  end
  local t = a / 255
  local inv = 1 - t
  return r * t + 255 * inv,
         g * t + 255 * inv,
         b * t + 255 * inv
end

-- Render the visible canvas (flattened cel at the current frame).
function ascii.image_to_ascii(sprite, image, cel, ramp, invert)
  local lines = {}
  local ox = cel and cel.position.x or 0
  local oy = cel and cel.position.y or 0

  -- Walk the sprite canvas so empty areas become spaces / light chars.
  for y = 0, sprite.height - 1 do
    local row = {}
    for x = 0, sprite.width - 1 do
      local ix = x - ox
      local iy = y - oy
      local r, g, b, a = 255, 255, 255, 0
      if image ~= nil
         and ix >= 0 and iy >= 0
         and ix < image.width and iy < image.height then
        local pixel = image:getPixel(ix, iy)
        r, g, b, a = rgba_from_pixel(sprite, image, pixel)
      end
      r, g, b = flatten_on_white(r, g, b, a)
      local char = pixel_to_char(luminance_of(r, g, b), ramp, invert)
      -- Output pixel twice since pixels' aspect ratio (assuming you chose 1x1 like a sane person) are larger than that of ASCII's.
      row[#row + 1] = char
      row[#row + 1] = char
    end
    lines[#lines + 1] = table.concat(row)
  end
  return table.concat(lines, "\n")
end

local function default_output_path(sprite)
  local path = sprite.filename
  if path == nil or path == "" then
    return "ascii.txt"
  end
  local dir = app.fs.filePath(path)
  local title = app.fs.fileTitle(path)
  if title == nil or title == "" then
    title = "ascii"
  end
  return app.fs.joinPath(dir, title .. ".txt")
end

local function write_text(path, text)
  local f, err = io.open(path, "w")
  if not f then
    return false, err or "could not open file"
  end
  f:write(text)
  if text:sub(-1) ~= "\n" then
    f:write("\n")
  end
  f:close()
  return true
end

-- ---------------------------------------------------------------------------
-- Aseprite UI
-- ---------------------------------------------------------------------------

local sprite = app.sprite
if sprite == nil then
  app.alert("Open a sprite first.")
  return
end

-- local image = app.image
-- local cel = app.cel

-- local dlg = Dialog{ title = "Pixel Art to ASCII" }

-- dlg:combobox{
--   id = "ramp",
--   label = "Ramp",
--   option = "detailed",
--   options = RAMP_LABELS,
-- }

-- dlg:check{
--   id = "invert",
--   label = "Invert",
--   text = "Reverse dark / light",
--   selected = false,
-- }

-- dlg:file{
--   id = "outfile",
--   label = "Save as",
--   save = true,
--   filename = default_output_path(sprite),
--   filetypes = { "txt" },
-- }

-- dlg:check{
--   id = "also_alert",
--   text = "Show result in a dialog",
--   selected = false,
-- }

-- dlg:button{
--   id = "ok",
--   text = "Convert",
--   focus = true,
-- }

-- dlg:button{
--   id = "cancel",
--   text = "Cancel",
-- }

-- dlg:show()

-- local data = dlg.data
-- if not data.ok then
--   return
-- end

-- local ramp = RAMPS[data.ramp] or RAMPS.detailed
-- local art = image_to_ascii(sprite, image, cel, ramp, data.invert)
-- local art = image_to_ascii(sprite, image, cel, "detailed", false)

-- if data.outfile ~= nil and data.outfile ~= "" then
--   local ok, err = write_text(data.outfile, art)
--   if not ok then
--     app.alert("Could not write file:\n" .. tostring(err))
--     return
--   end
-- end

-- if data.also_alert then
--   -- Dialogs truncate very large strings; still useful for small sprites.
--   local preview = art
--   local max_chars = 4000
--   if #preview > max_chars then
--     preview = preview:sub(1, max_chars) .. "\n... (truncated; full text saved to file)"
--   end
--   app.alert{ title = "ASCII", text = preview }
-- end

local function split_lines(text)
  local lines = {}
  if text == nil or text == "" then
    return lines
  end
  text = text:gsub("\r\n", "\n"):gsub("\r", "\n")
  for line in (text .. "\n"):gmatch("(.-)\n") do
    lines[#lines + 1] = line
  end
  -- Drop a single trailing empty line produced by a final newline.
  if #lines > 0 and lines[#lines] == "" then
    lines[#lines] = nil
  end
  return lines
end

local function pad_right(s, width)
  local n = #s
  if n >= width then
    return s
  end
  return s .. string.rep(" ", width - n)
end

local function frame_width(lines)
  local w = 0
  for i = 1, #lines do
    if #lines[i] > w then
      w = #lines[i]
    end
  end
  return w
end

-- frames: array of ASCII strings
-- sep: text inserted between frames on every row ("" or " " or " | ")
function ascii.hconcat(frames, sep)
  sep = sep or " "
  local normalized = {}
  local heights = {}
  local max_h = 0

  for i = 1, #frames do
    local lines = split_lines(frames[i])
    local w = frame_width(lines)
    for r = 1, #lines do
      lines[r] = pad_right(lines[r], w)
    end
    normalized[i] = { lines = lines, width = w }
    heights[i] = #lines
    if #lines > max_h then
      max_h = #lines
    end
  end

  for i = 1, #normalized do
    local blank = string.rep(" ", normalized[i].width)
    while #normalized[i].lines < max_h do
      normalized[i].lines[#normalized[i].lines + 1] = blank
    end
  end

  local out = {}
  for y = 1, max_h do
    local parts = {}
    for i = 1, #normalized do
      parts[#parts + 1] = normalized[i].lines[y]
    end
    out[y] = table.concat(parts, sep)
  end
  return table.concat(out, "\n")
end

return ascii