-- Paths are relative to this script's directory so the exporter works on any machine.
local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])") or "./"

-- BICYCLE_EXPORT picks the outputs: "ascii" (ASCII-art colormaps, for the ASCII render mode), "pixels" (.bimg
-- indexed images + palettes, for the 2D mode) or "both" (default; one export serves every render mode).
local export = os.getenv("BICYCLE_EXPORT") or "both"
assert(export == "ascii" or export == "pixels" or export == "both", "BICYCLE_EXPORT must be ascii, pixels or both")

local options = {
    ascii = export ~= "pixels",
    pixels = export ~= "ascii",
    -- Where generated genome files go. Override with the BICYCLE_GENOME_PATH environment variable (keep the trailing slash).
    genomePath = os.getenv("BICYCLE_GENOME_PATH") or (current_dir .. "../../test/genome/")
}

-- The genome tree layout (scripts/GenomeLayout.lua, shared with the engine): every output goes to the subfolder
-- for its kind, e.g. pilgrim_body_palette.lua -> <genome>/body/palette/color/ (README, "Genome layout").
options.layout = dofile(current_dir .. "../GenomeLayout.lua")

local function makeDirs(dir)
    if app and app.fs and app.fs.makeAllDirectories then
        app.fs.makeAllDirectories(dir)
    else
        os.execute('mkdir -p "' .. dir .. '"')
    end
end

-- Full path for a generated file by its bare name and extension (directories are created as needed).
function options.pathFor(name, ext)
    local dir = options.genomePath .. options.layout.dirFor(name)
    makeDirs(dir)
    return dir .. name .. "." .. ext
end

return options
