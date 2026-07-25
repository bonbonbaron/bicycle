require("bicycle")
require("io")

-- Any genes not supported by the game engine, such as stats, items, or equipment, are presumed to be Lua scripts we can require.

Genome = {}
Genome.__index = Genome
-- allow __newindex for blackboard functionality
Genome.new = function( name )
  local instance = { id=ffi.C.newEntity(), genes={} }
  setmetatable( instance, Genome )
  return instance
end

-- TODO call this from the loader to put a different position, dialogue, etc. on every instance of the same genome (e.g. shopkeepers w/ different items)
function Genome.inject(entity, tbl)
  -- TODO
end

local Gene = {}
Gene.__index = Gene
Gene.new = function( type, val )
  local instance = {type=type, val=val}
  setmetatable( instance, Gene )
  return instance
end

function Gene.getAssetDir(assetType)
  checkType(assetType, "string")
  return "./"..gameName.."/asset/"..assetType.."/"
end

function Gene.getLuaAsset( key, val )
  checkType( val, "string" )
  local dir = Gene.getAssetDir( key )
  return require( dir..val )  -- returns rendered lua script
end

function Gene.getTxtAsset( key, val )
  checkType( key, "string" )
  checkType( val, "string" )
  local dir = Gene.getAssetDir( key )
  local fp = dir..val..".txt"  -- TODO make this more generalizable
  local file = io.open( fp, "r" )
  if not file then error("Couldn't open "..key.." file '"..fp.."'.") end
  local contents = file:read("*a")
  file:close()
  return contents
end

Gene["GeneLoader"] = {
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
}

-- gameSupportedGenes is expected to be a table mapping to true.
-- Invalid keys both there and in the standard set are invalid altogether.
function Gene:load( entity )
  if gameSupportedGenes[self.type] then
    return  -- gene is game-specific; TODO consider making this a lua loader
  end
  loader = Gene.GeneLoader[self.type]

  -- Error-handling
  if not loader then
    recognizedGenes = ""
    for key, _ in pairs(GeneLoader) do
      recognizedGenes = recognizedGenes.."  "..key
    end
    for key, _ in pairs(gameSupportedGenes) do
      recognizedGenes = recognizedGenes.."  "..key
    end
    error( "Unrecognized gene type, "..gene.."\ntThese are the available genes:\n"..recognizedGenes )
  end

  -- Time to load the gene already. Geez!!
  -- TODO load processed gene straight into the entity's table. I think that table in turn should reside in global entities.
  loader( entity, self )
end


function Genome:load( genes )
  checkType( genes, "table" )
  for type, val in pairs(genes) do
    gene = Gene.new( type, val )
    gene:load(self.id)  -- genome's self.id = entity ID
  end
end
    
