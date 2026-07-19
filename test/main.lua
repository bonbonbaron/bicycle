package.path = package.path .. ";./scripts/?.lua"
require("Scene")
require("Menu")
require("Dialogue")

local s = Scene.new("intro")

function a(name) Dialogue.new( "hi, i'm an "..name, 10, 10, 20, 3 ) end

local m = Menu.new()
m:add( "Apples", function() a( "apple" ) end )
m:add( "Oranges", function() a( "orange" ) end )


