#include <UI/FabricatorUI.h>
#include <imgui.h>

namespace UI
{

FabricatorUI::FabricatorUI()
	: m_isOpen(false)
	, m_selectedRarity(Crafting::BlueprintRarity::Common)
	, m_hoveredBlueprintIndex(-1)
	, m_selectedBlueprint(nullptr)
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

	// Fullscreen window
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::SetNextWindowPos(ImVec2(0, 0));

	// Sci-fi dark styling
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.08f, 0.98f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.12f, 0.95f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.5f, 0.7f, 0.6f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	if (ImGui::Begin("##BlueprintFabricator", &m_isOpen, 
					 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
					 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
					 ImGuiWindowFlags_NoBringToFrontOnFocus))
	{
		// Title bar
		ImGui::SetCursorPos(ImVec2(20, 20));
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);  // Use default font (larger)
		ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "BLUEPRINT FABRICATOR");
		ImGui::PopFont();

		ImGui::SetCursorPos(ImVec2(20, 50));
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
						   "Capacity: %d/%d slots | Press I or ESC to close", 
						   inventory.getCurrentCapacity(), inventory.getMaxCapacity());

		// Position for main content
		ImGui::SetCursorPos(ImVec2(20, 90));

		// Add a dummy to inform ImGui about the space used by SetCursorPos
		ImGui::Dummy(ImVec2(0, 0));

		// Main layout: Rarity list (left) | Blueprint grid (center) | Crafting panel (right)
		ImGui::BeginChild("RarityPanel", ImVec2(200, -20), true);
		renderRarityList();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("BlueprintPanel", ImVec2(ImGui::GetContentRegionAvail().x * 0.6f, -20), true);
		renderBlueprintGrid(inventory);
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("CraftingPanel", ImVec2(0, -20), true);
		renderCraftingPanel(inventory);
		ImGui::EndChild();
	}
	ImGui::End();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(3);
}

void FabricatorUI::renderRarityList()
{
	ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "RARITY TIERS");
	ImGui::Separator();
	ImGui::Spacing();

	const Crafting::BlueprintRarity rarities[] = {
		Crafting::BlueprintRarity::Common,
		Crafting::BlueprintRarity::Uncommon,
		Crafting::BlueprintRarity::Rare,
		Crafting::BlueprintRarity::Epic,
		Crafting::BlueprintRarity::Legendary
	};

	for (auto rarity : rarities)
	{
		bool isSelected = (rarity == m_selectedRarity);
		std::string rarityName = Crafting::Blueprint::getRarityName(rarity);
		ImVec4 rarityColor = Crafting::Blueprint::getRarityColor(rarity);

		if (isSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(rarityColor.x * 0.5f, rarityColor.y * 0.5f, rarityColor.z * 0.5f, 1.0f));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.2f, 1.0f));
		}

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(rarityColor.x * 0.7f, rarityColor.y * 0.7f, rarityColor.z * 0.7f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, rarityColor);

		if (ImGui::Button(rarityName.c_str(), ImVec2(-1, 50)))
		{
			m_selectedRarity = rarity;
			m_selectedBlueprint = nullptr;
		}

		// Show tooltip on hover
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextColored(rarityColor, "%s Tier", rarityName.c_str());
			ImGui::Text("Click to view blueprints");
			ImGui::EndTooltip();
		}

		ImGui::PopStyleColor(3);
		ImGui::Spacing();
	}
}

void FabricatorUI::renderBlueprintGrid(Inventory::PlayerInventory& inventory)
{
	std::string rarityName = Crafting::Blueprint::getRarityName(m_selectedRarity);
	ImGui::TextColored(Crafting::Blueprint::getRarityColor(m_selectedRarity), 
					   "BLUEPRINTS - %s", rarityName.c_str());
	ImGui::Separator();
	ImGui::Spacing();

	auto blueprints = inventory.getBlueprintDatabase().getBlueprintsByRarity(m_selectedRarity);

	if (blueprints.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No blueprints in this tier");
		return;
	}

	auto resources = inventory.getResourceMap();
	const int columns = 3;
	const float buttonWidth = 200.0f;
	const float buttonHeight = 100.0f;

	for (int i = 0; i < static_cast<int>(blueprints.size()); ++i)
	{
		const Crafting::Blueprint* bp = blueprints[i];

		ImGui::PushID(i);

		bool isUnlocked = bp->isUnlocked;
		bool canCraft = bp->canCraft(resources);
		ImVec4 buttonColor;

		if (!isUnlocked)
		{
			buttonColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);  // Locked - dark
		}
		else if (canCraft)
		{
			buttonColor = ImVec4(0.1f, 0.3f, 0.2f, 1.0f);  // Craftable - green tint
		}
		else
		{
			buttonColor = ImVec4(0.15f, 0.15f, 0.2f, 1.0f);  // Not enough resources - dim
		}

		ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonColor.x * 1.5f, buttonColor.y * 1.5f, buttonColor.z * 1.5f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Crafting::Blueprint::getRarityColor(bp->rarity));

		if (ImGui::Button(bp->name.c_str(), ImVec2(buttonWidth, buttonHeight)))
		{
			m_selectedBlueprint = bp;
		}

		// Show tooltip on hover with blueprint details
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextColored(Crafting::Blueprint::getRarityColor(bp->rarity), "%s", bp->name.c_str());
			ImGui::Separator();
			ImGui::TextWrapped("%s", bp->description.c_str());
			ImGui::Spacing();
			if (!isUnlocked)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "LOCKED - Not yet discovered");
			}
			else if (canCraft)
			{
				ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Ready to craft!");
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Insufficient resources");
			}
			ImGui::Text("Click to view details");
			ImGui::EndTooltip();
		}

		ImGui::PopStyleColor(3);

		// Show lock icon or checkmark on the same line as button
		ImGui::SameLine();
		// Use spacing instead of SetCursorPosX to avoid ImGui warning
		float spacingOffset = -30.0f;
		if (spacingOffset < 0)
		{
			// Move cursor back by using negative dummy width
			ImGui::Dummy(ImVec2(spacingOffset, 0));
			ImGui::SameLine();
		}

		if (!isUnlocked)
		{
			ImGui::TextColored(ImVec4(0.5f, 0.1f, 0.1f, 1.0f), "[LOCKED]");
		}
		else if (canCraft)
		{
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[READY]");
		}

		ImGui::PopID();

		// Grid layout
		if ((i + 1) % columns != 0)
		{
			ImGui::SameLine();
		}
	}
}

void FabricatorUI::renderCraftingPanel(Inventory::PlayerInventory& inventory)
{
	ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "CRAFTING");
	ImGui::Separator();
	ImGui::Spacing();

	if (!m_selectedBlueprint)
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a blueprint to craft");
		return;
	}

	const Crafting::Blueprint& bp = *m_selectedBlueprint;

	// Blueprint info
	ImGui::TextColored(Crafting::Blueprint::getRarityColor(bp.rarity), "%s", bp.name.c_str());
	ImGui::TextWrapped("%s", bp.description.c_str());
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Check if unlocked
	if (!bp.isUnlocked)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "LOCKED");
		ImGui::TextWrapped("This blueprint has not been discovered yet.");
		return;
	}

	// Show resource costs
	ImGui::Text("Required Resources:");
	ImGui::Separator();

	auto resources = inventory.getResourceMap();
	bool canCraftAll = true;

	for (const auto& cost : bp.costs)
	{
		int playerHas = 0;
		auto it = resources.find(cost.resourceType);
		if (it != resources.end())
		{
			playerHas = it->second;
		}

		bool hasEnough = playerHas >= cost.quantity;
		if (!hasEnough)
			canCraftAll = false;

		// Find material name
		const auto& allMaterials = Inventory::PlayerInventory::getAllMaterials();
		std::string resourceName = "Unknown";
		for (const auto& mat : allMaterials)
		{
			if (mat.blockID == cost.resourceType)
			{
				resourceName = mat.name;
				break;
			}
		}

		ImVec4 textColor = hasEnough ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
		ImGui::TextColored(textColor, "%s: %d / %d", resourceName.c_str(), playerHas, cost.quantity);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Craft yield info
	ImGui::Text("Crafts: %d x %s", bp.craftYield, bp.name.c_str());
	ImGui::Spacing();

	// Craft button
	bool canCraft = bp.canCraft(resources);

	if (!canCraft)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.1f, 0.1f, 1.0f));
		ImGui::Button("INSUFFICIENT RESOURCES", ImVec2(-1, 50));
		ImGui::PopStyleColor();
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.7f, 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));

		if (ImGui::Button("CRAFT", ImVec2(-1, 50)))
		{
			int crafted = inventory.craftBlueprint(bp);
			if (crafted > 0)
			{
				// Success feedback (could add sound/particles here)
			}
		}

		ImGui::PopStyleColor(3);
	}
}

} // namespace UI

