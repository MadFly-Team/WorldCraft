#include <UI/FabricatorUI.h>
#include <imgui.h>

namespace UI
{

FabricatorUI::FabricatorUI()
	: m_isOpen(false)
	, m_selectedCategory(Inventory::MaterialCategory::Structure)
	, m_hoveredMaterialIndex(-1)
	, m_hoveredHotbarSlot(-1)
{
}

void FabricatorUI::show()
{
	m_isOpen = true;
}

void FabricatorUI::hide()
{
	m_isOpen = false;
}

void FabricatorUI::render(Inventory::PlayerInventory& inventory)
{
	if (!m_isOpen)
		return;

	// Center the window
	ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
							ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

	// Sci-fi window styling
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.12f, 0.95f));
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.0f, 0.4f, 0.6f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.0f, 0.5f, 0.7f, 1.0f));

	if (ImGui::Begin("Material Fabricator Database", &m_isOpen, ImGuiWindowFlags_NoCollapse))
	{
		// Left panel - Category list (20% width)
		ImGui::BeginChild("CategoryPanel", ImVec2(180, -50), true);
		renderCategoryList();
		ImGui::EndChild();

		ImGui::SameLine();

		// Right panel - Material grid (80% width)
		ImGui::BeginChild("MaterialPanel", ImVec2(0, -50), true);
		renderMaterialGrid(inventory);
		ImGui::EndChild();

		// Bottom panel - Hotbar slots
		ImGui::Separator();
		ImGui::Text("Hotbar:");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Click material above, then click hotbar slot to assign)");
		renderHotbarSlots(inventory);
	}
	ImGui::End();

	ImGui::PopStyleColor(3);
}

void FabricatorUI::renderCategoryList()
{
	ImGui::Text("Categories");
	ImGui::Separator();

	for (int i = 0; i < static_cast<int>(Inventory::MaterialCategory::COUNT); ++i)
	{
		Inventory::MaterialCategory category = static_cast<Inventory::MaterialCategory>(i);
		std::string categoryName = Inventory::PlayerInventory::getCategoryName(category);

		bool isSelected = (category == m_selectedCategory);

		// Category button styling
		if (isSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.7f, 1.0f));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
		}

		if (ImGui::Button(categoryName.c_str(), ImVec2(-1, 40)))
		{
			m_selectedCategory = category;
			m_hoveredMaterialIndex = -1;
		}

		ImGui::PopStyleColor();
	}
}

void FabricatorUI::renderMaterialGrid(Inventory::PlayerInventory& inventory)
{
	std::string categoryName = Inventory::PlayerInventory::getCategoryName(m_selectedCategory);
	ImGui::Text("Materials - %s", categoryName.c_str());
	ImGui::Separator();

	auto materials = Inventory::PlayerInventory::getMaterialsByCategory(m_selectedCategory);

	if (materials.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No materials in this category");
		return;
	}

	// Grid layout
	const int columns = 4;
	const float buttonSize = 120.0f;

	for (int i = 0; i < static_cast<int>(materials.size()); ++i)
	{
		const Inventory::Material& mat = materials[i];

		// Material button
		ImGui::PushID(i);

		// Color based on category
		ImVec4 buttonColor(0.2f, 0.2f, 0.3f, 1.0f);
		switch (mat.category)
		{
		case Inventory::MaterialCategory::Structure:  buttonColor = ImVec4(0.12f, 0.12f, 0.18f, 1.0f); break;
		case Inventory::MaterialCategory::Energy:     buttonColor = ImVec4(0.25f, 0.22f, 0.06f, 1.0f);  break;
		case Inventory::MaterialCategory::Tech:       buttonColor = ImVec4(0.06f, 0.18f, 0.25f, 1.0f);  break;
		case Inventory::MaterialCategory::Defensive:  buttonColor = ImVec4(0.18f, 0.06f, 0.06f, 1.0f);  break;
		case Inventory::MaterialCategory::Lighting:   buttonColor = ImVec4(0.25f, 0.25f, 0.12f, 1.0f);  break;
		case Inventory::MaterialCategory::Natural:    buttonColor = ImVec4(0.12f, 0.22f, 0.12f, 1.0f);  break;
		case Inventory::MaterialCategory::Special:    buttonColor = ImVec4(0.22f, 0.12f, 0.25f, 1.0f);  break;
		default: break;
		}

		ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonColor.x * 1.5f, buttonColor.y * 1.5f, buttonColor.z * 1.5f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.6f, 0.8f, 1.0f));

		if (ImGui::Button(mat.name.c_str(), ImVec2(buttonSize, buttonSize)))
		{
			m_hoveredMaterialIndex = i;
		}

		ImGui::PopStyleColor(3);

		// Tooltip with description
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("%s", mat.name.c_str());
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", mat.description.c_str());
			ImGui::Text("Category: %s", Inventory::PlayerInventory::getCategoryName(mat.category).c_str());
			ImGui::EndTooltip();
		}

		ImGui::PopID();

		// Layout columns
		if ((i + 1) % columns != 0 && i < static_cast<int>(materials.size()) - 1)
		{
			ImGui::SameLine();
		}
	}

	// Instructions
	ImGui::Spacing();
	ImGui::Separator();
	if (m_hoveredMaterialIndex >= 0)
	{
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Selected: %s", materials[m_hoveredMaterialIndex].name.c_str());
		ImGui::Text("Click a hotbar slot below to assign this material");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Click a material to select it");
	}
}

void FabricatorUI::renderHotbarSlots(Inventory::PlayerInventory& inventory)
{
	const float slotSize = 60.0f;
	const float spacing = 10.0f;

	for (int i = 0; i < Inventory::PlayerInventory::HOTBAR_SIZE; ++i)
	{
		ImGui::PushID(100 + i);

		const Inventory::Material& mat = inventory.getMaterial(i);
		bool isCurrentSlot = (i == inventory.getSelectedSlot());

		// Slot button styling
		ImVec4 slotColor = isCurrentSlot 
			? ImVec4(0.0f, 0.5f, 0.7f, 1.0f)
			: ImVec4(0.15f, 0.15f, 0.2f, 1.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, slotColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.6f, 0.8f, 1.0f));

		std::string label = mat.isEmpty() ? "Empty" : mat.name;
		if (ImGui::Button(label.c_str(), ImVec2(slotSize, slotSize)))
		{
			// Assign selected material to this slot
			if (m_hoveredMaterialIndex >= 0)
			{
				auto materials = Inventory::PlayerInventory::getMaterialsByCategory(m_selectedCategory);
				if (m_hoveredMaterialIndex < static_cast<int>(materials.size()))
				{
					inventory.setMaterial(i, materials[m_hoveredMaterialIndex]);
				}
			}
		}

		ImGui::PopStyleColor(2);

		// Slot number overlay
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 slotPos = ImGui::GetItemRectMin();
		char slotNum[4];
		snprintf(slotNum, sizeof(slotNum), "%d", i + 1);
		drawList->AddText(ImVec2(slotPos.x + 4, slotPos.y + 2), IM_COL32(200, 200, 200, 255), slotNum);

		ImGui::PopID();

		if (i < Inventory::PlayerInventory::HOTBAR_SIZE - 1)
		{
			ImGui::SameLine();
		}
	}
}

} // namespace UI
