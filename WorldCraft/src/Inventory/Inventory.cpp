#include <Inventory/Inventory.h>
#include <Persistence/WorldMetadata.h>
#include <algorithm>

namespace Inventory
{

PlayerInventory::PlayerInventory()
{
	for (int i = 0; i < 3; ++i)
	{
		m_hotbar[i].blockId = static_cast<Voxel::BlockID>(static_cast<int>(Voxel::BlockID::Stone) + i);
		m_hotbar[i].count = kMaxStack;
	}
}

const InventorySlot& PlayerInventory::getHotbarSlot(int index) const
{
	index = clampSlotIndex(index, kHotbarSlots);
	return m_hotbar[static_cast<size_t>(index)];
}

const InventorySlot& PlayerInventory::getPersonalSlot(int index) const
{
	index = clampSlotIndex(index, kPersonalSlots);
	return m_personal[static_cast<size_t>(index)];
}

InventorySlot& PlayerInventory::getHotbarSlotMutable(int index)
{
	index = clampSlotIndex(index, kHotbarSlots);
	return m_hotbar[static_cast<size_t>(index)];
}

InventorySlot& PlayerInventory::getPersonalSlotMutable(int index)
{
	index = clampSlotIndex(index, kPersonalSlots);
	return m_personal[static_cast<size_t>(index)];
}

int PlayerInventory::getSelectedSlot() const
{
	return m_selectedSlot;
}

void PlayerInventory::setSelectedSlot(int slot)
{
	m_selectedSlot = clampSlotIndex(slot, kHotbarSlots);
}

Voxel::BlockID PlayerInventory::getSelectedBlockID() const
{
	const InventorySlot& selected = m_hotbar[static_cast<size_t>(m_selectedSlot)];
	return selected.isEmpty() ? Voxel::BlockID::Air : selected.blockId;
}

bool PlayerInventory::consumeSelectedItem(int amount)
{
	if (amount <= 0)
		return true;

	InventorySlot& selected = m_hotbar[static_cast<size_t>(m_selectedSlot)];
	if (selected.isEmpty() || selected.count < amount)
		return false;

	selected.count -= amount;
	normalizeSlot(selected);
	return true;
}

int PlayerInventory::addToRange(std::array<InventorySlot, kHotbarSlots>& slots, Voxel::BlockID blockId, int amount)
{
	for (InventorySlot& slot : slots)
	{
		if (amount <= 0)
			return 0;
		if (!slot.isEmpty() && slot.blockId == blockId && slot.count < kMaxStack)
		{
			const int canTake = std::min(kMaxStack - slot.count, amount);
			slot.count += canTake;
			amount -= canTake;
		}
	}

	for (InventorySlot& slot : slots)
	{
		if (amount <= 0)
			return 0;
		if (slot.isEmpty())
		{
			const int canTake = std::min(kMaxStack, amount);
			slot.blockId = blockId;
			slot.count = canTake;
			amount -= canTake;
		}
	}

	return amount;
}

int PlayerInventory::addToRange(std::array<InventorySlot, kPersonalSlots>& slots, Voxel::BlockID blockId, int amount)
{
	for (InventorySlot& slot : slots)
	{
		if (amount <= 0)
			return 0;
		if (!slot.isEmpty() && slot.blockId == blockId && slot.count < kMaxStack)
		{
			const int canTake = std::min(kMaxStack - slot.count, amount);
			slot.count += canTake;
			amount -= canTake;
		}
	}

	for (InventorySlot& slot : slots)
	{
		if (amount <= 0)
			return 0;
		if (slot.isEmpty())
		{
			const int canTake = std::min(kMaxStack, amount);
			slot.blockId = blockId;
			slot.count = canTake;
			amount -= canTake;
		}
	}

	return amount;
}

int PlayerInventory::addPickedUpItem(Voxel::BlockID blockId, int amount)
{
	if (blockId == Voxel::BlockID::Air || amount <= 0)
		return amount;

	int remaining = amount;
	remaining = addToRange(m_hotbar, blockId, remaining);
	remaining = addToRange(m_personal, blockId, remaining);
	return remaining;
}

void PlayerInventory::swapSlots(bool sourceHotbar, int sourceIndex, bool targetHotbar, int targetIndex)
{
	InventorySlot& source = sourceHotbar ? getHotbarSlotMutable(sourceIndex) : getPersonalSlotMutable(sourceIndex);
	InventorySlot& target = targetHotbar ? getHotbarSlotMutable(targetIndex) : getPersonalSlotMutable(targetIndex);
	std::swap(source, target);
	normalizeSlot(source);
	normalizeSlot(target);
}

void PlayerInventory::saveToMetadata(Persistence::WorldMetadata& metadata) const
{
	for (int i = 0; i < kHotbarSlots; ++i)
	{
		const InventorySlot& slot = m_hotbar[static_cast<size_t>(i)];
		metadata.hotbarBlockIds[static_cast<size_t>(i)] = static_cast<uint16_t>(slot.isEmpty() ? Voxel::BlockID::Air : slot.blockId);
		metadata.hotbarCounts[static_cast<size_t>(i)] = static_cast<uint16_t>(slot.isEmpty() ? 0 : slot.count);
	}

	for (int i = 0; i < kPersonalSlots; ++i)
	{
		const InventorySlot& slot = m_personal[static_cast<size_t>(i)];
		metadata.personalBlockIds[static_cast<size_t>(i)] = static_cast<uint16_t>(slot.isEmpty() ? Voxel::BlockID::Air : slot.blockId);
		metadata.personalCounts[static_cast<size_t>(i)] = static_cast<uint16_t>(slot.isEmpty() ? 0 : slot.count);
	}

	metadata.selectedHotbarSlot = m_selectedSlot;
}

void PlayerInventory::loadFromMetadata(const Persistence::WorldMetadata& metadata)
{
	for (int i = 0; i < kHotbarSlots; ++i)
	{
		InventorySlot slot;
		slot.blockId = static_cast<Voxel::BlockID>(metadata.hotbarBlockIds[static_cast<size_t>(i)]);
		slot.count = static_cast<int>(metadata.hotbarCounts[static_cast<size_t>(i)]);
		normalizeSlot(slot);
		m_hotbar[static_cast<size_t>(i)] = slot;
	}

	for (int i = 0; i < kPersonalSlots; ++i)
	{
		InventorySlot slot;
		slot.blockId = static_cast<Voxel::BlockID>(metadata.personalBlockIds[static_cast<size_t>(i)]);
		slot.count = static_cast<int>(metadata.personalCounts[static_cast<size_t>(i)]);
		normalizeSlot(slot);
		m_personal[static_cast<size_t>(i)] = slot;
	}

	m_selectedSlot = clampSlotIndex(metadata.selectedHotbarSlot, kHotbarSlots);
}

int PlayerInventory::clampSlotIndex(int value, int maxExclusive)
{
	if (maxExclusive <= 0)
		return 0;
	return std::clamp(value, 0, maxExclusive - 1);
}

void PlayerInventory::normalizeSlot(InventorySlot& slot)
{
	if (slot.count <= 0)
	{
		slot.blockId = Voxel::BlockID::Air;
		slot.count = 0;
		return;
	}

	if (slot.blockId == Voxel::BlockID::Air)
	{
		slot.count = 0;
		return;
	}

	slot.count = std::clamp(slot.count, 1, kMaxStack);
}

} // namespace Inventory
