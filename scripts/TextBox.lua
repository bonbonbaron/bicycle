require("bicycle")

-- A dialogue box glued to the bottom of the screen: word-wrapped, paged, advanced with a key.
--   TextBox.open( "Elder", { "First thing he says.", "Second thing." }, function() ... end )
--   TextBox.isOpen()   -- while true, route the talk key to TextBox.advance() and freeze the player
--   TextBox.advance()  -- next page; closes (and calls onClose) after the last one
-- Each entry in the list starts a new page; long entries wrap and spill over onto more pages.
local TextBox = { ROWS = 3, KEY = "E" }
local pages, pageIdx, onClose = nil, 0, nil
-- Characters the player can talk to: each has an id (its entity), a name and lines. NPC genomes add themselves.
TextBox.speakers = {}
function TextBox.addSpeaker( npc )
  table.insert( TextBox.speakers, npc )
end

-- Splits text into rows of at most width characters, breaking at spaces (and inside words longer than a row).
function TextBox.wrap( text, width )
  local rows, row = {}, ""
  for word in text:gmatch( "%S+" ) do
    while #word > width do  -- a word longer than a row: hard-break it
      if #row > 0 then table.insert( rows, row ); row = "" end
      table.insert( rows, word:sub( 1, width ) )
      word = word:sub( width + 1 )
    end
    if #row == 0 then
      row = word
    elseif #row + 1 + #word <= width then
      row = row .. " " .. word
    else
      table.insert( rows, row )
      row = word
    end
  end
  if #row > 0 or #rows == 0 then table.insert( rows, row ) end
  return rows
end

-- Pages of up to `rows` rows; each text in texts starts a new page.
function TextBox.paginate( texts, width, rows )
  local out = {}
  for _, text in ipairs( texts ) do
    local wrapped = TextBox.wrap( text, width )
    for i = 1, #wrapped, rows do
      local page = {}
      for j = i, math.min( i + rows - 1, #wrapped ) do table.insert( page, wrapped[j] ) end
      table.insert( out, page )
    end
  end
  return out
end

local function show()
  local page = pages[pageIdx]
  local rows = { pages.speaker and ( pages.speaker .. ":" ) or nil }
  for _, r in ipairs( page ) do table.insert( rows, r ) end
  for _ = #page + 1, TextBox.ROWS do table.insert( rows, "" ) end  -- constant height: no jumping box
  local hint = pageIdx < #pages and ( "[" .. TextBox.KEY .. "] more  " .. pageIdx .. "/" .. #pages ) or ( "[" .. TextBox.KEY .. "] close" )
  table.insert( rows, string.rep( " ", math.max( 0, pages.width - #hint ) ) .. hint )
  ffi.C.setTextBox( table.concat( rows, "\n" ) )
end

function TextBox.open( speaker, texts, closeCb )
  local width = ffi.C.getTextBoxWidth()
  pages = TextBox.paginate( type( texts ) == "table" and texts or { texts }, width, TextBox.ROWS )
  pages.speaker, pages.width = speaker, width
  pageIdx, onClose = 1, closeCb
  show()
end

function TextBox.isOpen()
  return pages ~= nil
end

function TextBox.close()
  if not pages then return end
  pages, pageIdx = nil, 0
  ffi.C.setTextBox( "" )
  local cb = onClose
  onClose = nil
  if cb then cb() end
end

-- Returns true while the box is still open.
function TextBox.advance()
  if not pages then return false end
  if pageIdx < #pages then
    pageIdx = pageIdx + 1
    show()
    return true
  end
  TextBox.close()
  return false
end

return TextBox
