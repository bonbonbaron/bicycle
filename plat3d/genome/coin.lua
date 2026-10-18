-- A coin: spins and bobs where it's placed (declarative motion, the engine moves it), chimes and is gone for good
-- when the hero touches it. Everything about it is here, so it can be dropped into any level (Blender: Bicycle
-- panel > Add entity > coin).
local coin = {
  model = "coin3d",                                         -- genome/body/model/coin3d (exported from Blender)
  body = { kind = "kinematic", radius = 0.4, height = 0.8, pitch = 90 },
  coll = { type = "coin" },
  motion = { type = "spin", speed = 180, bob = 0.12, bobHz = 0.8 },
  sfx = { soundfont = "sfx", program = 120, sounds = { collect = 72 } },  -- music/sfx.sf2
}
local HERO = 1  -- Genome.COLL.hero

function coin:onCollision( other, collType )
  if collType ~= HERO then return end
  self:sound( "collect", 115 )
  local hero = entityOf( other )
  if hero and hero.collect then hero:collect( self ) end
  require( "Scene" ).removeForever( self )  -- stays collected when the hero comes back
end

return coin
