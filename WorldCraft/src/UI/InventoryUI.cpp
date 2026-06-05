#include <UI/InventoryUI.h>

#include <Texture/BlockTextures.h>
#include <Voxel/BlockRegistry.h>
#include <imgui.h>
#include <cstdio>
#include <cstring>

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

	Inventory::InventorySlot& resolveSlot(Inventory::PlayerInventory& inventory, bool isHotbar, int index)
	{
		return isHotbar ? inventory.getHotbarSlotMutable(index) : inventory.getPersonalSlotMutable(index);
	}
}

void InventoryUI::initialize()
{
	if (m_initialized)
		return;
	m_initialized = true;
}

void InventoryUI::shutdown()
{
	for (auto& pair : m_layerIcons)
	{
		if (pair.second != 0)
			glDeleteTextures(1, &pair.second);
	}
	m_layerIcons.clear();
	m_initialized = false;
	m_cursorHeld = {};
}

void InventoryUI::toggle() { m_open = !m_open; }
void InventoryUI::open() { m_open = true; }
void InventoryUI::close() { m_open = false; }
bool InventoryUI::isOpen() const { return m_open; }

void InventoryUI::render(Inventory::PlayerInventory& inventory, int screenW, int screenH)
{
	if (!m_open)
		return;
	if (!m_initialized)
		initialize();

	ImGui::SetNextWindowSize(ImVec2(760.0f, 520.0f), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2((screenW - 760.0f) * 0.5f, (screenH - 520.0f) * 0.5f), ImGuiCond_Always);

	if (!ImGui::Begin("Inventory", &m_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Hotbar");
	for (int i = 0; i < Inventory::PlayerInventory::kHotbarSlots; ++i)
	{
		renderSlot(inventory, true, i, "hotbar");
		if (i < Inventory::PlayerInventory::kHotbarSlots - 1)
			ImGui::SameLine();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Text("Personal Inventory");

	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 10; ++col)
		{
			const int idx = row * 10 + col;
			renderSlot(inventory, false, idx, "personal");
			if (col < 9)
				ImGui::SameLine();
		}
	}

	if (!m_cursorHeld.isEmpty())
	{
		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* fg = ImGui::GetForegroundDrawList();
		unsigned int icon = getIconForBlock(m_cursorHeld.blockId);
		if (icon != 0)
		{
			fg->AddImage(ImTextureRef(static_cast<ImTextureID>(icon)),
				ImVec2(io.MousePos.x - 16.0f, io.MousePos.y - 16.0f),
				ImVec2(io.MousePos.x + 16.0f, io.MousePos.y + 16.0f));
		}
		if (m_cursorHeld.count > 1)
		{
			char countBuf[16];
			std::snprintf(countBuf, sizeof(countBuf), "%d", m_cursorHeld.count);
			fg->AddText(ImVec2(io.MousePos.x + 12.0f, io.MousePos.y + 8.0f), IM_COL32(255, 255, 255, 255), countBuf);
		}
	}

	ImGui::End();
}

void InventoryUI::renderSlot(Inventory::PlayerInventory& inventory, bool isHotbar, int index, const char* idSuffix)
{
	Inventory::InventorySlot& slot = resolveSlot(inventory, isHotbar, index);

	char id[64];
	std::snprintf(id, sizeof(id), "##slot_%s_%d", idSuffix, index);
	ImGui::PushID(id);
	ImGui::InvisibleButton("slotbtn", ImVec2(kSlotSize, kSlotSize));

	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, max, IM_COL32(45, 45, 45, 220), 4.0f);
	drawList->AddRect(min, max, IM_COL32(170, 170, 170, 220), 4.0f, 0, 1.5f);

	if (!slot.isEmpty())
	{
		unsigned int icon = getIconForBlock(slot.blockId);
		if (icon != 0)
		{
			drawList->AddImage(
				ImTextureRef(static_cast<ImTextureID>(icon)),
				ImVec2(min.x + kIconPadding, min.y + kIconPadding),
				ImVec2(max.x - kIconPadding, max.y - kIconPadding));
		}

		if (slot.count > 1)
		{
			char countBuf[16];
			std::snprintf(countBuf, sizeof(countBuf), "%d", slot.count);
			ImVec2 textSize = ImGui::CalcTextSize(countBuf);
			drawList->AddText(ImVec2(max.x - textSize.x - 4.0f, max.y - textSize.y - 2.0f), IM_COL32(255, 255, 255, 255), countBuf);
		}
	}

	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		handleLeftClick(slot);
	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		handleRightClick(slot);

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
	{
		DragPayload payload;
		payload.sourceHotbar = isHotbar;
		payload.sourceIndex = index;
		ImGui::SetDragDropPayload("INV_SLOT", &payload, sizeof(payload));
		ImGui::Text("Move");
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("INV_SLOT"))
		{
			if (payload->DataSize == sizeof(DragPayload))
			{
				const auto* data = static_cast<const DragPayload*>(payload->Data);
				if (data->sourceHotbar != isHotbar || data->sourceIndex != index)
				{
					Inventory::InventorySlot& src = resolveSlot(inventory, data->sourceHotbar, data->sourceIndex);
					Inventory::InventorySlot& dst = resolveSlot(inventory, isHotbar, index);
					if (!src.isEmpty())
					{
						if (!dst.isEmpty() && dst.blockId == src.blockId && dst.count < Inventory::PlayerInventory::kMaxStack)
						{
							const int move = std::min(Inventory::PlayerInventory::kMaxStack - dst.count, src.count);
							dst.count += move;
							src.count -= move;
							if (src.count <= 0)
								src = {};
						}
						else
						{
							std::swap(src, dst);
						}
					}
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::PopID();
}

void InventoryUI::handleLeftClick(Inventory::InventorySlot& slot)
{
	if (m_cursorHeld.isEmpty())
	{
		if (!slot.isEmpty())
		{
			m_cursorHeld = slot;
			slot = {};
		}
		return;
	}

	if (slot.isEmpty())
	{
		slot = m_cursorHeld;
		m_cursorHeld = {};
		return;
	}

	if (slot.blockId == m_cursorHeld.blockId && slot.count < Inventory::PlayerInventory::kMaxStack)
	{
		const int canMove = std::min(Inventory::PlayerInventory::kMaxStack - slot.count, m_cursorHeld.count);
		slot.count += canMove;
		m_cursorHeld.count -= canMove;
		if (m_cursorHeld.count <= 0)
			m_cursorHeld = {};
		return;
	}

	std::swap(slot, m_cursorHeld);
}

void InventoryUI::handleRightClick(Inventory::InventorySlot& slot)
{
	if (m_cursorHeld.isEmpty())
	{
		if (!slot.isEmpty() && slot.count > 1)
		{
			const int take = (slot.count + 1) / 2;
			slot.count -= take;
			m_cursorHeld.blockId = slot.blockId;
			m_cursorHeld.count = take;
			if (slot.count <= 0)
				slot = {};
		}
		return;
	}

	if (slot.isEmpty())
	{
		slot.blockId = m_cursorHeld.blockId;
		slot.count = 1;
		m_cursorHeld.count -= 1;
		if (m_cursorHeld.count <= 0)
			m_cursorHeld = {};
		return;
	}

	if (slot.blockId == m_cursorHeld.blockId && slot.count < Inventory::PlayerInventory::kMaxStack)
	{
		slot.count += 1;
		m_cursorHeld.count -= 1;
		if (m_cursorHeld.count <= 0)
			m_cursorHeld = {};
	}
}

unsigned int InventoryUI::getIconForBlock(Voxel::BlockID blockId)
{
	if (blockId == Voxel::BlockID::Air)
		return 0;
	const Voxel::BlockRegistry& registry = Voxel::BlockRegistry::get();
	const int layer = registry.texLayer(blockId, Voxel::FaceDir::PosY);
	return getOrCreateLayerIcon(layer);
}

unsigned int InventoryUI::getOrCreateLayerIcon(int layer)
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

bool InventoryUI::generateLayerPixels(int layer, uint8_t* pixels, int pixelCount)
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
