local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])")
package.path = current_dir .. "?.lua;" .. package.path

--[[
	NOTES
		1) minimizing the palette in AS excludes occluded colors, preventing parallax layers
		2) every layer should be saved separately except in sprites, where we'll flatten for safety
		3) only one color palette is allowed per animation frame across
		4) layers shrink-wrap their contents when we write out. That's good!
		
	So for worlds, i want to
		\color sets    (png)
		\color maps    (png)
		\tile sets     (png)
		\tile maps     (lua)   (TODO: fix bug drawing tile indices in wrong order)
		\animation      (lua)  (bookmark... need to insert frames flexibly)
			\two types of animation: tile-based and frame-based.
			\TILE-BASED
			\FRAME-BASED
		collision sets (lua)  -- needs to be animation frame- and strip-based
		collision maps (lua)  -- 

	BOOKMARK: okay.... i'm seeing what needs to happen. Tile-based animation changes per tile, not entire image.
			  That means I need to output time remaining until the next tile change. 
			  I need to figure out how I'll do this from Aseprite.
]]
DEBUG = true
local function dbgp( s )
	if DEBUG then
		print(s)
	end
end

local function getBasename(fp)
	return fp:match("([^\\/]*)%.[^%.]+$")
end


local function ctoi(c)
	return app.pixelColor.rgba(c.red, c.green, c.blue, c.alpha)
end

local s = app.sprite
if not s then
	app.alert('no active sprite')
	return
end
local colorMode = s.colorMode
local spritePal = app.sprite.palettes[1]

-- package.path = package.path .. ";.\\?.lua"
require("debug")
require("table")
--local g = require("gentable")  -- TODO get require() to work later
local g = dofile("gentable.lua")

local Set = {}
Set.__index = Set

Set.new = function(name)
	local instance = {name=name, members={}, idx=0}
	setmetatable(instance, Set)
	return instance
end

function Set:add(key)
	if self.members[key] == nil then
		dbgp("mapping "..key.. " to " .. self.idx.. " in "..self.name)
		self.members[key] = self.idx
		self.idx = self.idx + 1
	end
end

function Set:keys()
	local result = {}
	for key, _ in pairs(self.members) do
		table.insert(result, key)
	end
	return result
end

function Set:has(idx)
	return self.members[idx] ~= nil
end


local function getGlobalAnimData(sprite)
	local globalAnimData = { strips = {} }
	-- ... what if there are untagged frames amidst tag ones?
	-- we'll only support one set of untagged frames. That'll be keyed "DEFAULT".
	if #sprite.tags > 0 then
		globalAnimData.name = getBasename(sprite.filename)
		for i = 1,#sprite.tags do
			local tag = sprite.tags[i]
			globalAnimData.strips[tag.name] = {}
			globalAnimData.strips[tag.name].direction = tag.aniDir
			globalAnimData.strips[tag.name].numFrames = tag.frames
			globalAnimData.strips[tag.name].numReps = tag.repeats
			globalAnimData.strips[tag.name].frames = {}
			local frames = globalAnimData.strips[tag.name].frames
			local frame = tag.fromFrame
			local framesRemaining = tag.frames
			while framesRemaining > 0 do
				table.insert( frames, { duration = frame.duration } )
				frame = frame.next
				framesRemaining = framesRemaining - 1
			end
		end
		-- TODO handle the case of leftover frames that're untagged
	elseif #sprite.frames > 1 then
		local DEFAULT_KEY = "DEFAULT"
		globalAnimData.strips[DEFAULT_KEY] = {}
		globalAnimData.strips[DEFAULT_KEY].direction = AniDir.FORWARD
		globalAnimData.strips[DEFAULT_KEY].numFrames = #sprite.frames
		globalAnimData.strips[DEFAULT_KEY].numReps = 0
		globalAnimData.strips[DEFAULT_KEY].frames = {}
		local frames = globalAnimData.strips[DEFAULT_KEY].frames
		for i = 1,#sprite.frames do
			table.insert( frames, { duration = frames[i].duration })
		end
	else
		globalAnimData = nil
	end
	return globalAnimData
end

local function getPixelValues(pixel, colorMode, spritePal)
	local pixelValue = pixel()  -- color in RGB mode, index otherwise
	local srcX = pixel.x
	local srcY = pixel.y
	if colorMode == ColorMode.INDEXED then
		return srcX, srcY, ctoi(spritePal:getColor(pixelValue))
	elseif colorMode == ColorMode.RGB then  -- Only support RGB and indexed color modes for now.
		return srcX, srcY, pixelValue
	end
end

local function getAnimTag(sprite, frameIdx)
	for i = 1,#sprite.tags do
		local tag = sprite.tags[i]
		if frameIdx >= tag.fromFrame.frameNumber and frameIdx <= tag.toFrame.frameNumber then
			return tag.name
		end
	end
	return nil
end

local OUTPUT_DIR = "C:\\Users\\michael\\AppData\\Roaming\\Aseprite\\scripts\\output\\"

local function getColors(ctr, globAnim)
	if ctr.layers == nil then
		error(debug.traceback())
	end
	for i, layer in ipairs(ctr.layers) do
		if layer.isReference then
			dbgp("skipping reference layer "..layer.name)
			goto continue
		end
		-- layer is a group
		local colorpal = Set.new(layer.name.." palette")
		local baseName = layer.name:gsub("%s+", "_")
		local cmIdx
		if layer.isGroup then
			getColors(layer, globAnim)  -- recurse for each layer in layer group
			-- TODO groups of layers may warrant combined anim info
		else  -- layer is a standalone
			if layer.isTilemap then
				dbgp("PROCESSING LAYER "..layer.name.." AS A TILEMAP")
				local outputColormapFp = OUTPUT_DIR..baseName.."_tileset_colormap.png"
				local tileset = layer.tileset
				local TILE_W = tileset.grid.tileSize.width
				local TILE_H = tileset.grid.tileSize.height
				local outputTilesetImg = Image(TILE_W * #layer.tileset, TILE_H, ColorMode.INDEXED)
				local outputTileIdx = 0
				local outputTilemap = {}
				local outputTileset = { anim = {}, colormap = baseName.."_tileset_colormap" }
				-- Generate the tileset.
				for tileIdx = 0, #layer.tileset - 1 do  -- don't go over image, do tileset
					local tile = tileset:tile(tileIdx)
					if tile.properties.duration then
						print("tile "..tileIdx.." has duration "..tile.properties.duration)
						table.insert(outputTileset.anim, { tileIdx = tileIdx, duration = tile.properties.duration })
					end
					local tileImg = tile.image -- the image of a cel within tileset
					for pixel in tileImg:pixels() do
						local srcX, srcY, pixelValue = getPixelValues(pixel, colorMode, spritePal)
						colorpal:add(pixelValue)
						cmIdx = colorpal.members[pixelValue]
						outputTilesetImg:drawPixel(srcX + TILE_W * outputTileIdx, srcY, cmIdx)
					end  -- for each pixel in the current cel
					outputTileIdx = outputTileIdx + 1
				end  -- for each tile's cel
				-- Generate the tilemap and animation.
				outputTilemap["tileWidth"] = TILE_W
				outputTilemap["tileHeight"] = TILE_H
				outputTilemap["numTilesWide"] = 0
				outputTilemap["numTilesHigh"] = 0
				local cumulativeTilesWide = 0
				-- Write tile indices to output tilemap.
				for _, cel in ipairs(layer.cels) do
					for tileIdx in cel.image:pixels() do
						table.insert( outputTilemap, tileIdx() ) -- TODO fix this
					end
					outputTilemap["numTilesWide"] = outputTilemap["numTilesWide"] + cel.image.width
					if outputTilemap["numTilesHigh"] < cel.image.height then
						outputTilemap["numTilesHigh"] = cel.image.height
					end
					-- While we're at it, let's add the source rectangle to the animation frame.
					cumulativeTilesWide = cumulativeTilesWide + cel.image.width
				end
				g.serialize_table( baseName.."_tilemap", outputTilemap, DEBUG )
				g.serialize_table( baseName.."_tileset", outputTileset, DEBUG )
				outputTilesetImg:saveAs(outputColormapFp)
				outputTilesetImg = nil
			elseif layer.isImage then
				local outputAnimSrcRects = { strips = {} }
				dbgp("PROCESSING LAYER "..layer.name.." AS AN IMAGE")
				local outputColormapFp = OUTPUT_DIR..baseName.."_colormap.png"
				-- First, obtain the total size of the output image.
				local outputW = 0
				local outputH = 0
				for _, cel in ipairs(layer.cels) do
					outputW = outputW + cel.image.width
					if cel.image.height > outputH then
						outputH = cel.image.height
					end
				end
				local outputFullImg = Image(outputW, outputH, ColorMode.INDEXED)
				local cumulativeWidth = 0
				for frameIdx, cel in ipairs(layer.cels) do
					for pixel in cel.image:pixels() do  -- TODO support animation (multiple cels)
						local srcX, srcY, pixelValue = getPixelValues(pixel, colorMode, spritePal)
						colorpal:add(pixelValue)
						cmIdx = colorpal.members[pixelValue]
						outputFullImg:drawPixel(cumulativeWidth + srcX, srcY, cmIdx )
					end   -- for each pixel in cel image
					local srcRect = { x = cumulativeWidth, y = 0, w = cel.image.width, h = cel.image.height }
					local animTag = getAnimTag(s, frameIdx)
					print(animTag .. " IS MY TAG BABY")
					if animTag and outputAnimSrcRects.strips[animTag] then
						table.insert(outputAnimSrcRects.strips[animTag], srcRect)
					elseif animTag then
						outputAnimSrcRects.strips[animTag] = { srcRect }
					else
						table.insert(outputAnimSrcRects.strips["DEFAULT"], srcRect)
					end
					cumulativeWidth = cumulativeWidth + cel.image.width
					-- While we're at it, let's add the source rectangle to the animation frame.
					end  -- for each cel in layer
				outputFullImg:saveAs(outputColormapFp)
				-- If there are any animation frames, write out the source rectangles for each frame.
				if next(outputAnimSrcRects) then
					outputAnimSrcRects.refGlobalAnim = globAnim.name
					g.serialize_table(baseName.."_anim_src_rects", outputAnimSrcRects, DEBUG)
				end
			end  -- whether layer is a tileset or an image
			g.serialize_table(baseName.."_color_palette", colorpal:keys(), DEBUG)
		end  -- if layer is group, recurse
		::continue::
	end  -- for each layer
end  -- function getColors()


local globAnim = getGlobalAnimData(s)
if colorMode == ColorMode.RGB or colorMode == ColorMode.INDEXED then
	getColors(s, globAnim)
	if globAnim ~= nil then
		g.serialize_table(globAnim.name.."_globalAnim", globAnim, DEBUG)
	end
	--elseif colorMode == ColorMode.GRAY then  -- PSST! This'll be super useful for ASCII games.
	-- TODO
end

app.events:off("fgcolorchange")