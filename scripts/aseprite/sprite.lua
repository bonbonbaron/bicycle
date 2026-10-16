local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])")
package.path = current_dir .. "?.lua;" .. package.path
local cfg = require("config")
local ascii = require("pixelToAscii")

local function pkv(m)
  for k, v in pairs(m) do
    print("key "..k.." --> "..tostring(v))
  end
end

--[[
	Remaining things to do:
		0. support ColorMode.GRAY
		1. wrap all outputs into a single body lua file
		2. wrap body file into a genome lua file
		3. load genome from bicycle and see what it does
		4. convert pixels to ASCII (how to handle terminals with limited color?)
		5. 

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

local function count(t)
    local n = 0
	if type(t) == "table" then
		for _ in pairs(t) do
			n = n + 1
		end
	end
    return n
end

local colorMode = s.colorMode
local spritePal = app.sprite.palettes[1]

-- package.path = package.path .. ";.\\?.lua"
require("debug")
require("table")
--local g = require("gentable")  -- TODO get require() to work later
local g = dofile(current_dir .. "gentable.lua")

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
	local globalAnimData = { strips = {}, name = getBasename(sprite.filename) }
	-- ... what if there are untagged frames amidst tag ones?
	-- we'll only support one set of untagged frames. That'll be keyed "DEFAULT".
	if #sprite.tags > 0 then
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
		dbgp("WARNING: Your animation frames are untagged, so we're making only one and naming it 'DEFAULT'.")
		local DEFAULT_KEY = "DEFAULT"
		globalAnimData.strips[DEFAULT_KEY] = {}
		globalAnimData.strips[DEFAULT_KEY].direction = AniDir.FORWARD
		globalAnimData.strips[DEFAULT_KEY].numFrames = #sprite.frames
		globalAnimData.strips[DEFAULT_KEY].numReps = 0
		globalAnimData.strips[DEFAULT_KEY].frames = {}
		local frames = globalAnimData.strips[DEFAULT_KEY].frames
		for i = 1,#sprite.frames do
			table.insert( frames, { duration = sprite.frames[i].duration })
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

-- Pixel export (cfg.pixels): .bimg files, bicycle's raw indexed image (include/data/BImg.h):
--   "BIMG" u16 w, u16 h, u16 nColors, u16 0, nColors RGBA u32 (R in the low byte), then w*h u8 palette indices.
-- Index 0 is always transparent. The layer's palette set (<layer>_palette.lua) maps each RGBA value to its index.
local TRANSPARENT = 0
local function paletteIndex(colorpal, pixelValue)
	if (pixelValue >> 24) == 0 then pixelValue = TRANSPARENT end  -- every fully transparent color is index 0
	colorpal:add(pixelValue)
	return colorpal.members[pixelValue]
end

local function writeBimg(path, w, h, colorpal, indices)
	if colorpal.idx > 256 then
		error(path..": "..colorpal.idx.." colors; a .bimg layer can have at most 256")
	end
	local pal = {}
	for value, idx in pairs(colorpal.members) do pal[idx + 1] = value end
	local out = { "BIMG", string.pack("<I2I2I2I2", w, h, colorpal.idx, 0) }
	for i = 1, colorpal.idx do out[#out + 1] = string.pack("<I4", pal[i] & 0xFFFFFFFF) end
	local row = {}
	for y = 0, h - 1 do
		for x = 0, w - 1 do row[x + 1] = indices[y * w + x + 1] or 0 end
		out[#out + 1] = string.char(table.unpack(row, 1, w))
	end
	local f = assert(io.open(path, "wb"))
	f:write(table.concat(out))
	f:close()
	dbgp("wrote "..path.." ("..w.."x"..h..", "..colorpal.idx.." colors)")
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

-- Parses "key=value key=value" from a layer's user data into background settings for the engine.
--   mode      fixed (default) | glued | parallax | loop | autoloop
--   parallax  percent of the camera's horizontal motion this layer follows (default 100; 50 for mode=parallax)
--   parallaxy percent of the camera's vertical motion this layer follows (default: same as parallax)
--   vx, vy    autoloop drift in cells per second (default vx=4 for autoloop)
--   x, y      layer offset in cells (default 0)
local BG_MODES = { fixed = true, glued = true, parallax = true, loop = true, autoloop = true }
function parseBgSettings(data)
	local bg = {}
	for key, val in (data or ""):gmatch("(%w+)%s*=%s*([%w%.%-]+)") do
		bg[key:lower()] = tonumber(val) or val:lower()
	end
	bg.mode = bg.mode or "fixed"
	if not BG_MODES[bg.mode] then
		error("Layer user data has unknown mode '"..tostring(bg.mode).."'. Use fixed, glued, parallax, loop or autoloop.")
	end
	bg.parallax = bg.parallax or (bg.mode == "parallax" and 50 or 100)
	bg.parallaxy = bg.parallaxy or bg.parallax
	if bg.mode == "autoloop" and not bg.vx and not bg.vy then
		bg.vx = 4
	end
	bg.vx = bg.vx or 0
	bg.vy = bg.vy or 0
	bg.x = bg.x or 0
	bg.y = bg.y or 0
	return bg
end

-- This is going to bundle together everything you output. 
local luaSprite = { layers = {}, anim = nil}
local spriteName = getBasename(s.filename)

local function makeSprite(ctr, globAnim)
	-- If your sprite has no layers, what're you even doing bro?
	if ctr.layers == nil then
		error(debug.traceback())
	end
	-- For each layer in sprite...
	for i, layer in ipairs(ctr.layers) do
		-- Output files are named after the layer, prefixed with the sprite's name (unless the layer name already
		-- starts with it) so layers with common names (e.g. "Layer 1", "sky") in different sprites don't collide.
		local layerName = layer.name:gsub("%s+", "_")
		if layerName:sub(1, #spriteName) ~= spriteName then
			layerName = spriteName.."_"..layerName
		end
		local colorpalName = layerName.."_palette"
		local colorpal = Set.new(colorpalName)
		if cfg.pixels then colorpal:add(TRANSPARENT) end  -- index 0
		local cmIdx
		local outputLayer = {}
		if layer.isReference then
			dbgp("skipping reference layer "..layer.name)
			goto continue
		end
		-- Layers that contain sub-layers are groups; recurse into them while not creating new sprites every time.
		if layer.isGroup then
			dbgp('layer is group')
			makeSprite(layer, globAnim)  -- recurse for each layer in layer group
			-- TODO groups of layers may warrant combined anim info
		else  -- layer is a standalone
			if layer.isTilemap then

				dbgp("PROCESSING LAYER "..layer.name.." AS A TILEMAP")
				local outputColormapFp = cfg.pathFor(layerName.."_tileset_colormap", "png")
				local tileset = layer.tileset
				local TILE_W = tileset.grid.tileSize.width
				local TILE_H = tileset.grid.tileSize.height
				local outputTilesetImg = Image(TILE_W * #layer.tileset, TILE_H, ColorMode.INDEXED)
				local outputTileIdx = 0
				local outputTilemap = {}
				local outputTileset = { anim = {}, colormap = layerName.."_tileset_colormap", ascii = {} }
				local tilesetPixels = {}  -- pixel export: palette indices of the tileset strip (tile i at x = i * TILE_W)
				local tilesetW = TILE_W * #layer.tileset
				local outputCollmap = {}
				local outputCollset = {}
				-- Generate the tileset.
				for tileIdx = 0, #layer.tileset - 1 do  -- don't go over image, do tileset
					local tile = tileset:tile(tileIdx)
					-- Tile animation
					if tile.properties.duration then
						table.insert(outputTileset.anim, { tileIdx = tileIdx, duration = tile.properties.duration })
					end
					-- Tile collision
					if s.properties.tileIdxToCollisionType then
						local collType = s.properties.tileIdxToCollisionType[tileIdx]
						if collType then
							outputCollmap[tileIdx] = collType
							outputCollset[collType] = true
						end
					end
					-- Tile image
					local tileImg = tile.image -- the image of a cel within tileset
					-- ASCII version of the tile, same 2-chars-per-pixel scheme as colormaps. ascii[tileIdx + 1] is tile tileIdx.
					if cfg.ascii then
						outputTileset.ascii[tileIdx + 1] = ascii.image_to_ascii(s, tileImg, nil, "detailed", false, TILE_W, TILE_H)
					end
					for pixel in tileImg:pixels() do
						local srcX, srcY, pixelValue = getPixelValues(pixel, colorMode, spritePal)
						if cfg.pixels then
							cmIdx = paletteIndex(colorpal, pixelValue)
							tilesetPixels[srcY * tilesetW + srcX + TILE_W * outputTileIdx + 1] = cmIdx
						else
							colorpal:add(pixelValue)
							cmIdx = colorpal.members[pixelValue]
							outputTilesetImg:drawPixel(srcX + TILE_W * outputTileIdx, srcY, cmIdx)
						end
					end  -- for each pixel in the current cel
					outputTileIdx = outputTileIdx + 1
				end  -- for each tile's cel
				-- Generate the tilemap from frame 1: tile indices, row-major and 1-based: tilemap[(row - 1) * numTilesWide + col].
				-- The map covers the whole canvas; the cel may be smaller and offset, so place it by its position.
				outputTilemap["tileWidth"] = TILE_W
				outputTilemap["tileHeight"] = TILE_H
				local W = math.ceil(s.width / TILE_W)
				local H = math.ceil(s.height / TILE_H)
				outputTilemap["numTilesWide"] = W
				outputTilemap["numTilesHigh"] = H
				for i = 1, W * H do
					outputTilemap[i] = 0  -- tile 0 is Aseprite's empty tile
				end
				local cel = layer:cel(1)
				if cel then
					local col0 = cel.position.x // TILE_W
					local row0 = cel.position.y // TILE_H
					for row = 0, cel.image.height - 1 do
						for col = 0, cel.image.width - 1 do
							local mapRow, mapCol = row0 + row, col0 + col
							if mapRow >= 0 and mapRow < H and mapCol >= 0 and mapCol < W then
								outputTilemap[mapRow * W + mapCol + 1] = app.pixelColor.tileI(cel.image:getPixel(col, row))
							end
						end
					end
				end
				-- Background settings come from the layer's user data (Layer Properties > user data), e.g.
				-- "mode=parallax parallax=50" or "mode=autoloop vx=4". See README.txt.
				outputLayer.bg = parseBgSettings(layer.data)
				local layerTilemapName = layerName.."_tilemap"
				local layerTilesetName = layerName.."_tileset"
				local layerCollmapName = layerName.."_collmap"
				local layerCollsetName = layerName.."_collset"
				outputLayer.tilemap = layerTilemapName
				outputLayer.tileset = layerTilesetName
				outputLayer.collmap = layerCollmapName
				outputLayer.collset = layerCollsetName
				g.serialize_table( layerTilemapName, outputTilemap, DEBUG )
				g.serialize_table( layerCollmapName, outputCollmap, DEBUG )
				g.serialize_table( layerCollsetName, outputCollset, DEBUG )
				if cfg.pixels then
					outputTileset.pixels = layerName.."_tileset_pixels"
					outputTileset.colormap = outputTileset.pixels
					writeBimg(cfg.pathFor(outputTileset.pixels, "bimg"), tilesetW, TILE_H, colorpal, tilesetPixels)
				else
					outputTilesetImg:saveAs(outputColormapFp)
				end
				g.serialize_table( layerTilesetName, outputTileset, DEBUG )
				outputTilesetImg = nil
			elseif layer.isImage then
				local outputAnimSrcRects = { strips = {} }
				dbgp("PROCESSING LAYER "..layer.name.." AS AN IMAGE")
				-- Decide between outputting ASCII art or pixel art.
				local colormapName = layerName.."_colormap"
				local ext = "png"
				if cfg.ascii then
					ext = "lua"
				end
				local outputColormapFp = cfg.pathFor(colormapName, ext)
				-- First, obtain the total size of the output image.
				local outputW = 0
				local outputH = 0
				-- Get maximum cel height to determine output image size.
				-- Width can just be added together. Anim strips are horizontal in our engine.
				-- Every layer gets one full-canvas frame per sprite frame (blank where it has no cel),
				-- so layers stay frame-aligned and can be composited.
				outputW = s.width * #s.frames
				outputH = s.height
				local outputFullImg = ""
				local framePixels = {}  -- pixel export: palette indices of the frame strip
				local cumulativeWidth = 0
				local outputCollRects = {}
				local outputCollSet = {}
				local asciiFrames = {}
				for frameIdx = 1, #s.frames do
					local cel = layer:cel(frameIdx)
					local collType = nil
					-- Frame image
					if cfg.ascii then
							local currCelAsAscii = ascii.image_to_ascii(s, cel and cel.image, cel, "detailed", false)
							table.insert(asciiFrames, currCelAsAscii)
					end
					if cfg.pixels and cel then
						for pixel in cel.image:pixels() do
							local srcX, srcY, pixelValue = getPixelValues(pixel, colorMode, spritePal)
							-- The cel can be smaller than the canvas: place it by its position within this frame's slot.
							local x, y = cel.position.x + srcX, cel.position.y + srcY
							if x >= 0 and x < s.width and y >= 0 and y < s.height then
								framePixels[y * outputW + cumulativeWidth + x + 1] = paletteIndex(colorpal, pixelValue)
							end
							-- For now, support only one collision type per frame.
							if layer.name == "collision" and s.properties.colorToCollisionType then
								collType = collType or s.properties.colorToCollisionType[tostring(pixelValue)]
							end
						end   -- for each pixel in cel image
					end
					if cfg.ascii then
						outputFullImg = ascii.hconcat(asciiFrames, " ")
					end
					-- Frame animation
					-- The reason we do this separately from global animation is each layer has different source rects.
					local srcRect = { x = cumulativeWidth, y = 0, w = s.width, h = s.height }
					local animTag = getAnimTag(s, frameIdx)
					if animTag and outputAnimSrcRects.strips[animTag] then
						table.insert(outputAnimSrcRects.strips[animTag], srcRect)
					elseif animTag then
						outputAnimSrcRects.strips[animTag] = { srcRect }
					else  -- if there is no tag for this animation, name it "DEFAULT".
						if outputAnimSrcRects.strips["DEFAULT"] == nil then
							outputAnimSrcRects.strips["DEFAULT"] = {}
						end
						table.insert(outputAnimSrcRects.strips["DEFAULT"], srcRect)
					end
					cumulativeWidth = cumulativeWidth + s.width
					-- Frame collision (rectangle-based, at least for now in our engine)
					if layer.name == "collision" and cel then
						local label = animTag or "default"
						-- Start a new animation strip if this is a new one (or anim'd at all).	
						local collRect = {
							x = cel.bounds.x,
							y = cel.bounds.y,
							w = cel.bounds.w,
							h = cel.bounds.h
						}
						outputCollRects[label] = outputCollRects[label] or {}
						table.insert(outputCollRects[label], collRect)
						if collType then 
							outputCollSet[collType] = true
						end
					end
				end  -- for each cel in layer
				-- Image (whether ascii or PNG)
				dbgp("outputting colormap name "..colormapName)
				outputLayer.colormap = colormapName
				if cfg.ascii then
					g.serialize_table(colormapName, {img = outputFullImg}, DEBUG)
				end
				if cfg.pixels then
					-- Frames side by side, each the full canvas: frame f's pixels start at x = (f - 1) * canvas width,
					-- matching the anim src rects (which are in pixels).
					outputLayer.pixels = layerName.."_pixels"
					outputLayer.pixelW = outputW
					outputLayer.pixelH = outputH
					writeBimg(cfg.pathFor(outputLayer.pixels, "bimg"), outputW, outputH, colorpal, framePixels)
				end
				-- If there are any animation frames, write out the source rectangles for each frame.
				-- Collision map
				if next(outputAnimSrcRects) then
					local animSrcRectsName = layerName.."_anim_src_rects"
					outputLayer.animSrcRects = animSrcRectsName
					outputAnimSrcRects.refGlobalAnim = globAnim and globAnim.name  -- TODO  put this at sprite's top level
					g.serialize_table(animSrcRectsName, outputAnimSrcRects, DEBUG)  -- reusable per sprite, so name after layer
				end
				-- Collision rects
				if next(outputCollRects) then
					local collRectsName = spriteName.."_collRects"
					outputLayer.collRects = collRectsName
					g.serialize_table(collRectsName, outputCollRects, DEBUG)  -- only one per sprite, so name after sprite
					if not next(outputCollSet) then
						app.alert("Your sprite has coll rects, but no collision types.")
					end
				end
				-- Collision set
				if next(outputCollSet) then
					local collSetName = spriteName.."_collSet"
					outputLayer.collSet = collSetName
					g.serialize_table(collSetName, outputCollSet, DEBUG)  -- only one per sprite, so name after sprite
					if not next(outputCollSet) then
						app.alert("Your sprite has coll types, but no collision rects.")
					end
				end
			-- else
			-- 	if layer.isBackground then
			-- 		dbpg("its'a  bg")
			-- 	end
			-- 	dbgp("its' none of these thnings")
			end  -- whether layer is a tileset or an image
		end  -- if layer is group, recurse
		::continue::
		g.serialize_table(colorpalName, colorpal, DEBUG)
		outputLayer.colorpal = colorpalName
		table.insert( luaSprite.layers, outputLayer )
	end  -- for each layer
end  -- function makeSprite()



local globAnim = getGlobalAnimData(s)
luaSprite.anim = globAnim and globAnim.name
if colorMode == ColorMode.RGB or colorMode == ColorMode.INDEXED then
	makeSprite(s, globAnim)
	if globAnim ~= nil then
		g.serialize_table(globAnim.name.."_globalAnim", globAnim, DEBUG)
	end
	--elseif colorMode == ColorMode.GRAY then  -- PSST! This'll be super useful for ASCII games.
	-- TODO
	g.serialize_table(spriteName, luaSprite, DEBUG)
end

-- New tags/sprites: refresh gen/ids.lua now (the build does it too; README, "IDs").
require("refreshids")()
