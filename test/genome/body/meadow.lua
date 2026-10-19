--auto-generated table by generate_table.lua

local meadow = {
  layers = {
    {
      tileset = "meadow_sky_tileset",
      bg = {
        y = 0,
        parallaxy = 100,
        mode = "glued",
        vy = 0,
        vx = 0,
        x = 0,
        parallax = 100
      },
      tilemap = "meadow_sky_tilemap",
      colorpal = "meadow_sky_palette"
    },
    {
      tileset = "meadow_clouds_tileset",
      bg = {
        y = 0,
        parallaxy = 100,
        mode = "autoloop",
        vy = 0,
        vx = 6,
        x = 0,
        parallax = 100
      },
      tilemap = "meadow_clouds_tilemap",
      colorpal = "meadow_clouds_palette"
    },
    {
      tileset = "meadow_hills_tileset",
      bg = {
        y = 0,
        parallaxy = 50,
        mode = "parallax",
        vy = 0,
        vx = 0,
        x = 0,
        parallax = 50
      },
      tilemap = "meadow_hills_tilemap",
      colorpal = "meadow_hills_palette"
    },
    {
      tileset = "meadow_ground_tileset",
      bg = {
        y = 0,
        parallaxy = 100,
        mode = "fixed",
        vy = 0,
        vx = 0,
        x = 0,
        parallax = 100
      },
      tilemap = "meadow_ground_tilemap",
      colorpal = "meadow_ground_palette"
    },
    {
      tileset = "meadow_fence_tileset",
      bg = {
        y = 0,
        parallaxy = 100,
        mode = "loop",
        vy = 0,
        vx = 0,
        x = 0,
        parallax = 150
      },
      tilemap = "meadow_fence_tilemap",
      colorpal = "meadow_fence_palette"
    }
  }
}

return meadow