package.path = package.path .. ";./scripts/?.lua"
require("Scene")
require("Genome")

gameName = "test"
gameSupportedGenes = {}
genome = Genome.new()
a = { img = "abc" }
genome:load(a)

Scene.new("intro")

