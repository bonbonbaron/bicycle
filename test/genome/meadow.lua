--auto-generated table by generate_table.lua

local meadow = {
  layers = {
    {
      collmap = "meadow_sky_collmap",
      tilemap = "meadow_sky_tilemap",
      colorpal = "meadow_sky_palette",
      collset = "meadow_sky_collset",
      tileset = "meadow_sky_tileset",
      bg = {
        y = 0,
        parallax = 100,
        parallaxy = 100,
        x = 0,
        vy = 0,
        vx = 0,
        mode = "glued"
      }
    },
    {
      collmap = "meadow_clouds_collmap",
      tilemap = "meadow_clouds_tilemap",
      colorpal = "meadow_clouds_palette",
      collset = "meadow_clouds_collset",
      tileset = "meadow_clouds_tileset",
      bg = {
        y = 0,
        mode = "autoloop",
        vx = 6,
        x = 0,
        vy = 0,
        parallaxy = 100,
        parallax = 100
      }
    },
    {
      collmap = "meadow_hills_collmap",
      tilemap = "meadow_hills_tilemap",
      colorpal = "meadow_hills_palette",
      collset = "meadow_hills_collset",
      tileset = "meadow_hills_tileset",
      bg = {
        y = 0,
        parallax = 50,
        parallaxy = 50,
        x = 0,
        vy = 0,
        vx = 0,
        mode = "parallax"
      }
    },
    {
      collmap = "meadow_ground_collmap",
      tilemap = "meadow_ground_tilemap",
      colorpal = "meadow_ground_palette",
      collset = "meadow_ground_collset",
      tileset = "meadow_ground_tileset",
      bg = {
        y = 0,
        parallax = 100,
        parallaxy = 100,
        x = 0,
        vy = 0,
        vx = 0,
        mode = "fixed"
      }
    },
    {
      collmap = "meadow_fence_collmap",
      tilemap = "meadow_fence_tilemap",
      colorpal = "meadow_fence_palette",
      collset = "meadow_fence_collset",
      tileset = "meadow_fence_tileset",
      bg = {
        y = 0,
        parallax = 150,
        parallaxy = 100,
        x = 0,
        vy = 0,
        vx = 0,
        mode = "loop"
      }
    }
  }
}

return meadow