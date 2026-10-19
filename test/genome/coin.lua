require('bicycle')
-- A spinning coin (Aseprite animation via C++ Animator). Touching it chimes and moves it somewhere random.
-- Everything about it is here: its sprite, animation, collision and its own sound, so it can be dropped into any
-- scene (scripts/aseprite/addentity.lua) as is.
local coin = {
  sprite = "goldcoin", initAnim = "spin", coll = { type = "coin" },
  sfx = { soundfont = "meadow", program = 120, sounds = { collect = 72 } },
}
local HERO = 1  -- Genome.COLL.hero

function coin:onCollision(other, collType)
  if collType == HERO then
    self:sound( "collect" )
    local x, y = ffi.C.rollDice( 4, 300 ), ffi.C.rollDice( 34, 42 )
    ffi.C.teleport( self.id, x * 256, y * 256 )
    dbglog( "coin collected; respawned at %d,%d", x, y )
  end
end

return coin
