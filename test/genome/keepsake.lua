require('bicycle')
local Sound = require('Sound')
local Scene = require('Scene')
-- A keepsake on the cottage floor: the saved-state demo. Picking it up removes it for good (Scene.removeForever),
-- so it is still gone after you walk out to the meadow and back in.
local keepsake = { sprite = "goldcoin", initAnim = "spin", coll = { type = "coin" } }
local HERO = 1  -- Genome.COLL.hero

function keepsake:onCollision(other, collType)
  if collType == HERO and not self.taken then
    self.taken = true
    Sound.play( "coin" )
    Scene.state.keepsakes = ( Scene.state.keepsakes or 0 ) + 1  -- per-scene saved state: any Lua values
    Scene.removeForever( self )
    dbglog( "keepsake taken" )
  end
end

return keepsake
