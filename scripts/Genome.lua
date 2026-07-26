require("bicycle")
require("io")
local Scene = require("Scene")

--------------------
---- GENES
--------------------

local Gene = {}
Gene.__index = Gene
Gene.new = function( type, val )
  local instance = {type=type, val=val}
  setmetatable( instance, Gene )
  return instance
end

function Gene.getAssetDir(geneType)
  checkType(geneType, "string")
  return "./"..gameName.."/genome/"..geneType.."/"
end

function Gene.getLuaAsset( assetType, assetName )
  checkType( assetName, "string" )
  local dir = Gene.getAssetDir( assetType )
  return require( dir..assetName )  -- returns rendered lua script
end

function Gene.getTxtAsset( assetType, assetName )
  checkType( assetType, "string" )
  checkType( assetName, "string" )
  local dir = Gene.getAssetDir( assetType )
  local fp = dir..assetName..".txt"  -- TODO make this more generalizable
  local file = io.open( fp, "r" )
  if not file then error("Couldn't open "..key.." file '"..fp.."'.") end
  local contents = file:read("*a")
  file:close()
  return contents
end

Gene["GeneLoader"] = {
  scene = function(entity, gene)
    checkType( entity, "number" )
    checkType( gene, "table" )
    asset = Gene.getLuaAsset( "scene", gene.val )
    Scene.new( asset )
  end,
	img = function(entity, gene)
    local imgStr = Gene.getTxtAsset("img", gene.val)  -- should use gene.type somehow too
    ffi.C.newImage( entity, imgStr )
  end,
  anim = function(entity, gene)
    --TODO insert into world anim array
  end,
  vel = function(entity, gene)
    --TODO insert into world vel array
  end,
  name = 0,  -- do nothing, just allow it to exist
}

-- gameSupportedGenes is expected to be a table mapping to true.
-- Invalid keys both there and in the standard set are invalid altogether.
function Gene:load( entity, genomeName )
  checkType( entity, "number" )
  checkType( genomeName, "string" )
  if gameSupportedGenes[self.type] then
    return  -- gene is game-specific; TODO consider making this a lua loader
  end
  loader = Gene.GeneLoader[self.type]

  -- Error-handling
  if not loader then
    recognizedGenes = ""
    for key, _ in pairs(Gene.GeneLoader) do
      recognizedGenes = recognizedGenes.."  "..key
    end
    for key, _ in pairs(gameSupportedGenes) do
      recognizedGenes = recognizedGenes.."  "..key
    end
    error( "Genome '"..genomeName.."' has unrecognized gene key '"..self.type.."'.\nThese are the available genes:\n"..recognizedGenes )
  end

  -- Time to load the gene already. Geez!!
  -- TODO load processed gene straight into the entity's table. I think that table in turn should reside in global entities.
  loader( entity, self )
end

--------------------
---- GENOMES
--------------------

-- Any genes not supported by the game engine, such as stats, items, or equipment, are presumed to be Lua scripts we can require.
local Genome = {}
Genome.__index = Genome
-- allow __newindex for blackboard functionality
Genome.new = function( name )
  local tbl = Genome.get( name ) -- tbl is the table of type-value pairs of genes (e.g. { img = "shopkeeper", ... })
  local instance = { id=ffi.C.newEntity(), tbl=tbl, genes={} }
  for type, val in pairs(instance.tbl) do
    local gene = Gene.new( type, val )
    gene:load(instance.id, name)  -- genome's self.id = entity ID
  end
  setmetatable( instance, Genome )
  return instance
end

function Genome.get(genomeName)
  checkType(genomeName, "string")
  return require("./"..gameName.."/genome/"..genomeName)
end

-- TODO call this from the loader to put a different position, dialogue, etc. on every instance of the same genome (e.g. shopkeepers w/ different items)
function Genome.inject(entity, tbl)
  -- TODO
end

return Genome
