require("Window")

Dialogue = {}
Dialogue.__index = Dialogue
Dialogue.__newindex = function() error( "You can't create new attributes for Dialogue." ) end
Dialogue.HEIGHT = 7  -- border + 4 rows of text + the "[E] more" row + border
Dialogue.speakers = {}  -- characters the player can talk to (NPC genomes add themselves); each has id, name, lines

-- Pushes a dialogue window and sends it input until the reader pages past the end (E or Space), then pops it,
-- gives input back to whoever had it and calls onClose. Without x, y, w, h it spans the bottom of the screen.
-- Newlines in text start new rows.
Dialogue.new = function( text, x, y, w, h, onClose )
  local h = h or Dialogue.HEIGHT
  local w = w or ffi.C.getScreenWidth() - 2
  local x = x or 1
  local y = y or ffi.C.getScreenHeight() - h - 1
  local id = ffi.C.pushDialogue( text, x, y, w, h )
  local instance = { id=id, x=x, y=y, w=w, h=h, cbs = {}, prevFocus=getFocus(), onClose=onClose }
  setmetatable( instance, Dialogue )
  setmetatable( Dialogue, Window )
  Window.register(id, instance)
  Window.focusOnEntity(id)
  return instance
end

function Dialogue:onInput( input )
  -- The C++ Dialogue has already seen this key (it pages forward on E and Space).
  if ffi.C.isDialogueDone() or input == ffi.C.Escape then
    ffi.C.popWindow()
    unregister(self.id)
    Window.focusOnEntity(self.prevFocus)
    if self.onClose then self.onClose() end
  end
end

function Dialogue.addSpeaker( npc )
  table.insert( Dialogue.speakers, npc )
end
