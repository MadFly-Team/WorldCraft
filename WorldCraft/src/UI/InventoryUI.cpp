#include <UI/InventoryUI.h>
#include <Voxel/BlockTypes.h>
#include <imgui.h>
#include <string>

namespace UI
{

InventoryUI::InventoryUI()
	: m_isOpen(false)
{
}

void InventoryUI::render(Inventory::PlayerInventory& inventory)
{
	if (!m_isOpen)
		return;

	// Fullscreen semi-transparent overlay
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::SetNextWindowBgAlpha(0.90f);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40, 40));

	if (!ImGui::Begin("##InventoryFullscreen", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus))
	{
		ImGui::PopStyleVar(2);
		ImGui::End();
		return;
	}

	// Title
	ImGui::PushFont(ImGui::GetFont());
	ImGui::SetWindowFontScale(2.0f);
	ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "INVENTORY");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopFont();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Show capacity
	int currentCap = inventory.getCurrentCapacity();
	int maxCap = inventory.getMaxCapacity();
	ImGui::Text("Capacity: %d / %d slots", currentCap, maxCap);
	ImGui::Spacing();

	// Display inventory grid
	const float slotSize = 64.0f;
	const float spacing = 8.0f;
	const int slotsPerRow = 10;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

	for (int i = 0; i < currentCap; ++i)
	{
		const Inventory::InventorySlot& slot = inventory.getSlot(i);

		// Start new row
		if (i > 0 && i % slotsPerRow == 0)
			ImGui::NewLine();
		else if (i > 0)
			ImGui::SameLine();

		// Slot background
		ImVec4 slotColor = (i == inventory.getSelectedSlot()) 
			? ImVec4(0.2f, 0.5f, 0.8f, 1.0f)  // Selected: blue
			: ImVec4(0.2f, 0.2f, 0.2f, 1.0f);  // Normal: dark gray

		ImGui::PushStyleColor(ImGuiCol_Button, slotColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

		std::string slotLabel = "##slot" + std::to_string(i);
		if (ImGui::Button(slotLabel.c_str(), ImVec2(slotSize, slotSize)))
		{
			// Click to select this slot
			inventory.setSelectedSlot(i);
		}

		// Show tooltip on hover if slot has an item
		if (slot.blockID != Voxel::BlockID::Air && slot.stackCount > 0 && ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("Block ID: %d", static_cast<int>(slot.blockID));
			ImGui::Text("Stack: %d", slot.stackCount);
			ImGui::Text("Click to select");
			ImGui::EndTooltip();
		}

		ImGui::PopStyleColor(3);

		// Draw item info on top of the slot
		if (slot.blockID != Voxel::BlockID::Air && slot.stackCount > 0)
		{
			ImVec2 slotMin = ImGui::GetItemRectMin();
			ImVec2 slotMax = ImGui::GetItemRectMax();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			// Item type text (centered)
			std::string itemName = std::to_string(static_cast<int>(slot.blockID));
			ImVec2 textSize = ImGui::CalcTextSize(itemName.c_str());
			ImVec2 textPos = ImVec2(
				slotMin.x + (slotSize - textSize.x) * 0.5f,
				slotMin.y + 10
			);
			drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), itemName.c_str());

			// Stack count (bottom-right)
			if (slot.stackCount > 1)
			{
				std::string countText = std::to_string(slot.stackCount);
				ImVec2 countSize = ImGui::CalcTextSize(countText.c_str());
				ImVec2 countPos = ImVec2(
					slotMax.x - countSize.x - 4,
					slotMax.y - countSize.y - 4
				);
				drawList->AddText(countPos, IM_COL32(255, 255, 0, 255), countText.c_str());
			}
		}
	}

	ImGui::PopStyleVar();

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Instructions
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press I or ESC to close");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Use number keys (1-9) to select hotbar slots");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press B to open Blueprint Crafting");

	ImGui::End();
	ImGui::PopStyleVar(2);
}

} // namespace UI
