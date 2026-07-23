package.path = package.path .. ";./scripts/?.lua"
require("Scene")
require("Genome")

gameSupportedGenes = {}
genome = Genome.new()
a = { img = "abc" }
genome:load(a)

gameName = "test"
Scene.new("intro")

