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
      collset = "meadow_sky_collset",
      tilemap = "meadow_sky_tilemap",
      colorpal = "meadow_sky_palette",
      collmap = "meadow_sky_collmap"
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
      collset = "meadow_clouds_collset",
      tilemap = "meadow_clouds_tilemap",
      colorpal = "meadow_clouds_palette",
      collmap = "meadow_clouds_collmap"
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
      collset = "meadow_hills_collset",
      tilemap = "meadow_hills_tilemap",
      colorpal = "meadow_hills_palette",
      collmap = "meadow_hills_collmap"
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
      collset = "meadow_ground_collset",
      tilemap = "meadow_ground_tilemap",
      colorpal = "meadow_ground_palette",
      collmap = "meadow_ground_collmap"
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
      collset = "meadow_fence_collset",
      tilemap = "meadow_fence_tilemap",
      colorpal = "meadow_fence_palette",
      collmap = "meadow_fence_collmap"
    }
  }
}

return meadow