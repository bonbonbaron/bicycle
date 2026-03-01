#pragma once
#include <string>
#include <string_view>

constexpr unsigned FRAMES_PER_SECOND{ 30 };
constexpr double MILLISECONDS_PER_FRAME{ 1e3 / FRAMES_PER_SECOND };
constexpr double FRAMES_PER_MILLISECOND{ 1.0 / MILLISECONDS_PER_FRAME };

constexpr std::string_view SUFFIX{ ".yml" };
const std::string_view CFG_DIR{ "./config/" };

  // mkdir -p config/node/{dialogue,menu,grid/bg,entity/{body,personality/tree}}
const std::string MENU_DIR{ CFG_DIR.data() + std::string("menu/") };
const std::string DIALOGUE_DIR{ CFG_DIR.data() + std::string("dialogue/") };
const std::string GRID_DIR{ CFG_DIR.data() + std::string("grid/") };

const std::string NODE_DIR{ CFG_DIR.data() + std::string("node/") };

const std::string BG_DIR{ GRID_DIR + std::string("bg/") };

const std::string ENTITY_DIR{ NODE_DIR + std::string("entity/") };
const std::string BODY_DIR { ENTITY_DIR + std::string("body/") };
const std::string PERSONALITY_DIR { ENTITY_DIR + std::string("personality/") };
const std::string TREE_DIR { PERSONALITY_DIR + std::string("tree/") };
