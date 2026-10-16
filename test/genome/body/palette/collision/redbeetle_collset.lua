-- The red beetle's collision palette: its graybody's collision regions -> collision types (Genome.COLL).
-- The green beetle has none (harmless); the red one burns: same graybody, same animation, different palettes.
return {
  name = "redbeetle_collset",
  types = { [1] = "fire" },
}
