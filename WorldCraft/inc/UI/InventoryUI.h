#pragma once

#include <Inventory/Inventory.h>
#include <unordered_map>

namespace UI
{

class InventoryUI
{
public:
	void initialize();
	void shutdown();

	void toggle();
	void open();
	void close();
	bool isOpen() const;

	void render(Inventory::PlayerInventory& inventory, int screenW, int screenH);

private:
	struct DragPayload
	{
		bool sourceHotbar = true;
		int sourceIndex = 0;
	};

	bool m_open = false;
	bool m_initialized = false;
	Inventory::InventorySlot m_cursorHeld;
	std::unordered_map<int, unsigned int> m_layerIcons;

	void renderSlot(Inventory::PlayerInventory& inventory, bool isHotbar, int index, const char* idSuffix);
	void handleLeftClick(Inventory::InventorySlot& slot);
	void handleRightClick(Inventory::InventorySlot& slot);
	void handleDrop(Inventory::PlayerInventory& inventory, bool targetHotbar, int targetIndex);

	unsigned int getIconForBlock(Voxel::BlockID blockId);
	unsigned int getOrCreateLayerIcon(int layer);
	static bool generateLayerPixels(int layer, uint8_t* pixels, int pixelCount);
};

} // namespace UI
