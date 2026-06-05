#pragma once

#include <Voxel/BlockTypes.h>
#include <array>
#include <cstdint>

namespace Persistence
{
	struct WorldMetadata;
}

namespace Inventory
{

struct InventorySlot
{
	Voxel::BlockID blockId = Voxel::BlockID::Air;
	int count = 0;

	bool isEmpty() const
	{
		return blockId == Voxel::BlockID::Air || count <= 0;
	}
};

class PlayerInventory
{
public:
	static constexpr int kHotbarSlots = 10;
	static constexpr int kPersonalSlots = 40;
	static constexpr int kMaxStack = 64;

	PlayerInventory();

	const InventorySlot& getHotbarSlot(int index) const;
	const InventorySlot& getPersonalSlot(int index) const;
	InventorySlot& getHotbarSlotMutable(int index);
	InventorySlot& getPersonalSlotMutable(int index);

	int getSelectedSlot() const;
	void setSelectedSlot(int slot);

	Voxel::BlockID getSelectedBlockID() const;
	bool consumeSelectedItem(int amount);

	int addPickedUpItem(Voxel::BlockID blockId, int amount);

	void swapSlots(bool sourceHotbar, int sourceIndex, bool targetHotbar, int targetIndex);

	void saveToMetadata(Persistence::WorldMetadata& metadata) const;
	void loadFromMetadata(const Persistence::WorldMetadata& metadata);

private:
	std::array<InventorySlot, kHotbarSlots> m_hotbar;
	std::array<InventorySlot, kPersonalSlots> m_personal;
	int m_selectedSlot = 0;

	static int clampSlotIndex(int value, int maxExclusive);
	static void normalizeSlot(InventorySlot& slot);
	int addToRange(std::array<InventorySlot, kHotbarSlots>& slots, Voxel::BlockID blockId, int amount);
	int addToRange(std::array<InventorySlot, kPersonalSlots>& slots, Voxel::BlockID blockId, int amount);
};

} // namespace Inventory
