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

return options
