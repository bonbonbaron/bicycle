require('bicycle')
-- A palette swap of the beetle (genome/beetle.lua): the same graybody (body/graybody/beetle_sprite_* pixels and
-- animation, loaded once and shared) with a red color palette (body/palette/color/redbeetle_palette.lua) and a
-- fire collision palette (body/palette/collision/redbeetle_collset.lua), like a red koopa next to a green one.
local redbeetle = {
  sprite = "beetle_sprite", initAnim = "crawl",
  palette = "redbeetle_palette",
  collpal = "redbeetle_collset",
  coll = {},  -- type from collpal: region 1 -> "fire"
  motion = { type = "linear", speed = 8, path = { { 0, 0 }, { 10, 0 } } },
  sfx = { soundfont = "meadow", program = 120, sounds = { burn = 55 } },
}
local HERO = 1  -- Genome.COLL.hero

function redbeetle:onCollision( other, collType )
  if collType == HERO then
    self:sound( "burn" )
    dbglog( "red beetle: fire! (collision palette redbeetle_collset)" )
  end
end

return redbeetle
