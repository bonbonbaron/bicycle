-- Paths are relative to this script's directory so the exporter works on any machine.
local current_file = debug.getinfo(1, "S").source:match("^@?(.*)")
local current_dir = current_file:match("^(.*[/\\])") or "./"

local options = {
    ascii = true,
    -- Where generated genome files go. Override with the BICYCLE_GENOME_PATH environment variable (keep the trailing slash).
    genomePath = os.getenv("BICYCLE_GENOME_PATH") or (current_dir .. "../../test/genome/")
}

return options
