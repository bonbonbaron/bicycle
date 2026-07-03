#pragma once

#include "c/Collision.h"
#include "m/Entity.h"
#include <vector>
#include <array>
#include <unordered_set>
#include <utility>

constexpr unsigned NUM_SUPPORTED_COLL_LAYERS{8};

class CollisionDetector {
  public:
    using CollisionLayers = std::array<std::vector<Entity>, NUM_SUPPORTED_COLL_LAYERS>;
    static void check();
    static void registerEntity( Entity entity, unsigned layer );
    static auto getInstance() -> CollisionDetector&;
    void addEntity( Entity entity, unsigned layer );
    auto getCollisionLayers() -> CollisionLayers&;
		auto recordedCollision( Entity e1, Entity e2 ) -> bool;
		void recordCollision( Entity e1, Entity e2 );
		void deleteCollision( Entity e1, Entity e2 );
  private:
    CollisionDetector() = default;
    CollisionDetector(const CollisionDetector&) = delete;
    CollisionDetector operator=(const CollisionDetector&) = delete;
    CollisionDetector(const CollisionDetector&&) = delete;
    CollisionDetector operator=(const CollisionDetector&&) = delete;
    CollisionLayers _collLayers;

		// Simple custom hash function for pair<Entity, Entity>
		struct PairHash {
				std::size_t operator()(const std::pair<Entity, Entity>& p) const {
						// Shift and XOR to combine hashes (works well for grid coordinates)
						return std::hash<Entity>{}(p.first) ^ (std::hash<Entity>{}(p.second) << 1);
				}
		};
    // The smaller entity ID is always on the LHS, so this always works.
		std::unordered_set<std::pair<Entity, Entity>, PairHash> _recordedCollisions;
};

