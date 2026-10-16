-- The cottage interior in 3D (genome/scene/house.lua: scenery3d = "house3d"). Hand-written: a plank floor and walls
-- with a doorway on the left (Blender isn't needed for boxes this simple). The packer turns the .obj files into .bmesh.
return {
  objects = {
    { name = "house_floor", mesh = "model/house_floor", role = "static" },
    { name = "house_walls", mesh = "model/house_walls", role = "static" },
  }
}
