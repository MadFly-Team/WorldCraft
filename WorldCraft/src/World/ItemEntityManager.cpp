#include <World/ItemEntityManager.h>
#include <algorithm>

namespace World
{

void ItemEntityManager::spawnItem(const glm::vec3& position, Voxel::BlockID blockType, int stackCount)
{
	m_items.push_back(std::make_unique<ItemEntity>(position, blockType, stackCount));
}

void ItemEntityManager::update(float deltaTime, const glm::vec3& playerPos, float pickupRadius,
							   CollisionCheckFunc collisionCheck,
							   std::function<void(Voxel::BlockID, int)> onCollect)
{
	// Update all items
	for (auto& item : m_items)
	{
		bool stillActive = item->update(deltaTime, playerPos, pickupRadius, collisionCheck);

		// Check if item was collected
		if (!stillActive && item->isBeingCollected())
		{
			// Notify callback about collected item
			if (onCollect)
			{
				onCollect(item->getBlockType(), item->getStackCount());
			}
		}
	}

	// Remove collected/expired items
	m_items.erase(
		std::remove_if(m_items.begin(), m_items.end(),
			[](const std::unique_ptr<ItemEntity>& item) {
				return item->shouldRemove();
			}),
		m_items.end()
	);
}

void ItemEntityManager::clear()
{
	m_items.clear();
}

} // namespace World
