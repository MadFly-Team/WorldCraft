#include <UI/Hotbar.h>

#include <Texture/BlockTextures.h>
#include <Voxel/BlockRegistry.h>
#include <imgui.h>
#include <cstdio>

namespace UI
{

namespace
{
	constexpr float kSlotSize = 48.0f;
	constexpr float kSlotPadding = 6.0f;
	constexpr float kIconPadding = 6.0f;

	void createTextureFromPixels(const uint8_t* pixels, unsigned int& outTexture)
	{
		glGenTextures(1, &outTexture);
		glBindTexture(GL_TEXTURE_2D, outTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			Texture::TEX_SIZE, Texture::TEX_SIZE,
			0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void Hotbar::initialize()
{
	if (m_initialized)
		return;
	m_initialized = true;
}

void Hotbar::shutdown()
{
	for (auto& pair : m_layerIcons)
	{
		if (pair.second != 0)
			glDeleteTextures(1, &pair.second);
	}
	m_layerIcons.clear();
	m_initialized = false;
}

void Hotbar::render(const Inventory::PlayerInventory& inventory, int screenW, int screenH)
{
	if (!m_initialized)
		initialize();

	const float totalWidth = Inventory::PlayerInventory::kHotbarSlots * kSlotSize +
		(Inventory::PlayerInventory::kHotbarSlots - 1) * kSlotPadding;
	const float startX = (static_cast<float>(screenW) - totalWidth) * 0.5f;
	const float startY = static_cast<float>(screenH) - kSlotSize - 20.0f;

	ImGui::SetNextWindowPos(ImVec2(startX - 10.0f, startY - 10.0f), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::Begin("##HotbarOverlay", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_AlwaysAutoResize);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 base = ImGui::GetCursorScreenPos();

	for (int i = 0; i < Inventory::PlayerInventory::kHotbarSlots; ++i)
	{
		const float x = base.x + i * (kSlotSize + kSlotPadding);
		const float y = base.y;
		ImVec2 min(x, y);
		ImVec2 max(x + kSlotSize, y + kSlotSize);

		const bool selected = (i == inventory.getSelectedSlot());
		ImU32 fillColor = selected ? IM_COL32(80, 120, 80, 220) : IM_COL32(50, 50, 50, 190);
		ImU32 borderColor = selected ? IM_COL32(255, 220, 80, 255) : IM_COL32(170, 170, 170, 220);

		drawList->AddRectFilled(min, max, fillColor, 4.0f);
		drawList->AddRect(min, max, borderColor, 4.0f, 0, selected ? 3.0f : 1.5f);

		const Inventory::InventorySlot& slot = inventory.getHotbarSlot(i);
		if (!slot.isEmpty())
		{
			unsigned int icon = getIconForBlock(slot.blockId);
			if (icon != 0)
			{
				drawList->AddImage(
					ImTextureRef(static_cast<ImTextureID>(icon)),
					ImVec2(x + kIconPadding, y + kIconPadding),
					ImVec2(x + kSlotSize - kIconPadding, y + kSlotSize - kIconPadding));
			}

			if (slot.count > 1)
			{
				char countBuf[16];
				std::snprintf(countBuf, sizeof(countBuf), "%d", slot.count);
				ImVec2 textSize = ImGui::CalcTextSize(countBuf);
				drawList->AddText(
					ImVec2(x + kSlotSize - textSize.x - 4.0f, y + kSlotSize - textSize.y - 2.0f),
					IM_COL32(255, 255, 255, 255),
					countBuf);
			}
		}
	}

	ImGui::Dummy(ImVec2(totalWidth, kSlotSize));
	ImGui::End();
}

unsigned int Hotbar::getIconForBlock(Voxel::BlockID blockId)
{
	if (blockId == Voxel::BlockID::Air)
		return 0;

	const Voxel::BlockRegistry& registry = Voxel::BlockRegistry::get();
	const int layer = registry.texLayer(blockId, Voxel::FaceDir::PosY);
	return getOrCreateLayerIcon(layer);
}

unsigned int Hotbar::getOrCreateLayerIcon(int layer)
{
	auto it = m_layerIcons.find(layer);
	if (it != m_layerIcons.end())
		return it->second;

	uint8_t pixels[Texture::TEX_SIZE * Texture::TEX_SIZE * 4] = {};
	if (!generateLayerPixels(layer, pixels, static_cast<int>(sizeof(pixels))))
		return 0;

	unsigned int texture = 0;
	createTextureFromPixels(pixels, texture);
	m_layerIcons[layer] = texture;
	return texture;
}

bool Hotbar::generateLayerPixels(int layer, uint8_t* pixels, int pixelCount)
{
	if (!pixels || pixelCount < Texture::TEX_SIZE * Texture::TEX_SIZE * 4)
		return false;

	using GenFn = void(*)(Texture::TextureArray::PixelBuf);
	static const GenFn generators[Texture::TOTAL_LAYERS] = {
		Texture::TextureArray::genStone,
		Texture::TextureArray::genDirt,
		Texture::TextureArray::genGrassTop,
		Texture::TextureArray::genGrassSide,
		Texture::TextureArray::genSand,
		Texture::TextureArray::genWoodTop,
		Texture::TextureArray::genWoodSide,
		Texture::TextureArray::genLeaf,
		Texture::TextureArray::genBedrock,
		Texture::TextureArray::genGravel,
		Texture::TextureArray::genSnow,
		Texture::TextureArray::genWater,
		Texture::TextureArray::genCoalOre,
		Texture::TextureArray::genIronOre,
		Texture::TextureArray::genGoldOre,
		Texture::TextureArray::genDiamondOre,
		Texture::TextureArray::genRedstoneOre,
		Texture::TextureArray::genLapisOre,
		Texture::TextureArray::genEmeraldOre,
		Texture::TextureArray::genObsidian,
		Texture::TextureArray::genGrassTop1,
		Texture::TextureArray::genGrassTop2,
		Texture::TextureArray::genGrassTop3,
		Texture::TextureArray::genGrassTop4,
		Texture::TextureArray::genGrassTop5,
		Texture::TextureArray::genGrassTop6,
		Texture::TextureArray::genGrassTop7,
		Texture::TextureArray::genGrassTop8,
		Texture::TextureArray::genGrassSide1,
		Texture::TextureArray::genGrassSide2,
		Texture::TextureArray::genGrassSide3,
		Texture::TextureArray::genGrassSide4,
		Texture::TextureArray::genGrassSide5,
		Texture::TextureArray::genGrassSide6,
		Texture::TextureArray::genGrassSide7,
		Texture::TextureArray::genGrassSide8,
		Texture::TextureArray::genRock1,
		Texture::TextureArray::genRock2,
		Texture::TextureArray::genRock3,
		Texture::TextureArray::genRock4,
		Texture::TextureArray::genRock5,
		Texture::TextureArray::genRock6,
		Texture::TextureArray::genRock7,
		Texture::TextureArray::genRock8,
		Texture::TextureArray::genMushroom,
		Texture::TextureArray::genTorch,
	};

	if (layer < 0 || layer >= Texture::TOTAL_LAYERS)
		return false;

	auto* buf = reinterpret_cast<Texture::TextureArray::PixelBuf*>(pixels);
	generators[layer](*buf);
	return true;
}

} // namespace UI
