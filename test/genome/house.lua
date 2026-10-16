--auto-generated table by generate_table.lua

local house = {
  layers = {
    {
      tilemap = "house_wall_tilemap",
      collset = "house_wall_collset",
      tileset = "house_wall_tileset",
      bg = {
        y = 0,
        vx = 0,
        parallax = 100,
        mode = "fixed",
        parallaxy = 100,
        x = 0,
        vy = 0
      },
      colorpal = "house_wall_palette",
      collmap = "house_wall_collmap"
    },
    {
      tilemap = "house_floor_tilemap",
      collset = "house_floor_collset",
      tileset = "house_floor_tileset",
      bg = {
        y = 0,
        vx = 0,
        parallax = 100,
        mode = "fixed",
        parallaxy = 100,
        x = 0,
        vy = 0
      },
      colorpal = "house_floor_palette",
      collmap = "house_floor_collmap"
    }
  }
}

return house