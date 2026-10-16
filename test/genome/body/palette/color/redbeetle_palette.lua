-- A palette swap of beetle_sprite (test/genome/redbeetle.lua): the beetle graybody's gray indices -> new colors.
-- Only the entries listed change; RGBA is 0xAABBGGRR (R in the low byte, as in .bimg).
-- beetle_sprite_body_palette: 1 = shell (40,140,70), 2 = shine (120,210,120), 3 = legs (25,25,25).
return {
  name = "redbeetle_palette",
  colors = {
    [1] = 0xFF2828BE,  -- shell (190,40,40)
    [2] = 0xFF5A8CFA,  -- shine (250,140,90)
  },
}
