#pragma once
#include <string>

const std::string CONFIG_DIR{ "./config/" };
const std::string NODE_DIR{ CONFIG_DIR + "node/" };
const std::string ENTITY_DIR{ NODE_DIR + "entity/" };
const std::string BODY_DIR{ ENTITY_DIR + "body/" };
const std::string PERS_DIR{ ENTITY_DIR + "personality/" };
const std::string TREE_DIR{ PERS_DIR + "tree/" };  
const std::string_view SUFFIX{ ".yml" };
