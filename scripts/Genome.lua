require("bicycle")

Genome = {}
Genome.__index = Genome
Genome.new = function( name )
  local instance = { id=ffi.C.newEntity() }
  setmetatable( instance, Genome )
  return instance
end

function Genome.inject(entity, tbl)
  -- TODO
end

local Gene = {}
Gene.__index = Gene
Gene.new = function( name )
  local instance = { id=ffi.C.newEntity() }
  setmetatable( instance, Gene )
  return instance
end

function Gene.getAssetDir(assetType)
  checkType(name, "string")
  return "./"..gameName.."/asset/"..assetType.."/"
end

function Gene.getAsset( key, val )
  checkType( val, "string" )
  local dir = getAssetDir( key )
  return require( dir..val )
end

Gene["GeneLoader"] = {
	img = function(entity, gene)
  end,
  anim = function(key, val)
  end,
  vel = function(key, val)
  end,
}

function Gene.load( key, val )
  local asset = Gene.getAsset(key, val)

end


--function Gene:add( geneType

--start with image
--Genome:load
