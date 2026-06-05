#pragma once

#include <Inventory/Inventory.h>
#include <unordered_map>

namespace UI
{

class Hotbar
{
public:
	void initialize();
	void shutdown();
	void render(const Inventory::PlayerInventory& inventory, int screenW, int screenH);

private:
	std::unordered_map<int, unsigned int> m_layerIcons;
	bool m_initialized = false;

	unsigned int getIconForBlock(Voxel::BlockID blockId);
	unsigned int getOrCreateLayerIcon(int layer);
	static bool generateLayerPixels(int layer, uint8_t* pixels, int pixelCount);
};

} // namespace UI
