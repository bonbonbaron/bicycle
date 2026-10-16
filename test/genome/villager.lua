require('bicycle')
require('Dialogue')
-- An old villager (Aseprite sprite "elder", idling) who blocks the way and talks when the hero presses E next to him.
local villager = {
  sprite="elder", initAnim="idle", coll={ type="npc" },
  name="Old Tobias",
  lines={
    "Ah, a pilgrim! It has been many seasons since anyone walked this meadow road.",
    "Mind that horse. She wanders where she pleases, and she kicks anyone who gets underfoot. You will feel it, too: watch the bar up top.",
    "The coins? Folk drop them on the way to market. Keep what you find. Safe travels, friend.",
  },
}

function villager:onSpawn()
  Dialogue.addSpeaker( self )
end

function villager:onInput(input) end

return villager
