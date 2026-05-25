#pragma once

#include <World/ItemEntity.h>
#include <vector>
#include <memory>
#include <functional>

namespace World
{

// Manages all item entities in the world
class ItemEntityManager
{
public:
	ItemEntityManager() = default;

	// Spawn a new item entity at the given position
	void spawnItem(const glm::vec3& position, Voxel::BlockID blockType, int stackCount = 1);

	// Update all items (physics, collection, despawn)
	// collisionCheck: callback to check if a world position is solid
	// Calls onCollect for each collected item (blockType, stackCount)
	void update(float deltaTime, const glm::vec3& playerPos, float pickupRadius = 2.5f,
				CollisionCheckFunc collisionCheck = nullptr,
				std::function<void(Voxel::BlockID, int)> onCollect = nullptr);

	// Get all active items for rendering
	const std::vector<std::unique_ptr<ItemEntity>>& getItems() const { return m_items; }

	// Clear all items (used when switching worlds)
	void clear();

	// Get number of active items
	size_t getCount() const { return m_items.size(); }

private:
	std::vector<std::unique_ptr<ItemEntity>> m_items;
};

} // namespace World
