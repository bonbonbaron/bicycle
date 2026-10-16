require('bicycle')
local Sound = require('Sound')
-- A spinning coin (Aseprite animation via C++ Animator). Touching it chimes and moves it somewhere random.
local coin = { sprite="goldcoin", initAnim="spin", coll={ type="coin" } }
local HERO = 1  -- Genome.COLL.hero

function coin:onCollision(other, collType)
  if collType == HERO then
    Sound.play( "coin" )
    local pos = ffi.C.World_getBoxes().arr[self.id].pos
    pos.x = ffi.C.rollDice( 4, 300 ) * 256
    pos.y = ffi.C.rollDice( 34, 42 ) * 256
    dbglog( "coin collected; respawned at %d,%d", pos.x / 256, pos.y / 256 )
  end
end

return coin
