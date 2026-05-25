#include <UI/Hotbar.h>
#include <imgui.h>

namespace UI
{

Hotbar::Hotbar()
	: m_visible(true)
{
}

void Hotbar::render(const Inventory::PlayerInventory& inventory, int screenWidth, int screenHeight)
{
	if (!m_visible)
		return;

	// Calculate total width of hotbar
	constexpr int numSlots = Inventory::PlayerInventory::HOTBAR_SIZE;
	float totalWidth = (SLOT_SIZE * numSlots) + (SLOT_PADDING * (numSlots - 1));

	// Center horizontally, position at bottom with padding
	float startX = (screenWidth - totalWidth) * 0.5f;
	float startY = screenHeight - HOTBAR_PADDING_BOTTOM - SLOT_SIZE;

	// Create invisible window to hold hotbar slots
	ImGui::SetNextWindowPos(ImVec2(startX - 10.0f, startY - 10.0f));
	ImGui::SetNextWindowSize(ImVec2(totalWidth + 20.0f, SLOT_SIZE + 40.0f));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("##Hotbar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoInputs);

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// Render each slot
	int selectedSlot = inventory.getSelectedSlot();
	for (int i = 0; i < numSlots; ++i)
	{
		float slotX = startX + i * (SLOT_SIZE + SLOT_PADDING);
		const Inventory::Material& material = inventory.getMaterial(i);
		renderSlot(i, material, i == selectedSlot, slotX, startY);
	}

	ImGui::End();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();
}

void Hotbar::renderSlot(int slotIndex, const Inventory::Material& material, 
						bool isSelected, float posX, float posY)
{
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();

	// Slot bounds
	ImVec2 slotMin(posX, posY);
	ImVec2 slotMax(posX + SLOT_SIZE, posY + SLOT_SIZE);

	// Slot background color (sci-fi dark with glow)
	ImU32 bgColor = isSelected 
		? IM_COL32(0, 100, 150, 200)      // Cyan glow for selected
		: IM_COL32(20, 20, 30, 180);      // Dark background for unselected

	// Border color
	ImU32 borderColor = isSelected
		? IM_COL32(0, 200, 255, 255)      // Bright cyan for selected
		: IM_COL32(80, 80, 100, 200);     // Gray for unselected

	float borderThickness = isSelected ? 3.0f : 2.0f;

	// Draw slot background
	drawList->AddRectFilled(slotMin, slotMax, bgColor, 4.0f);

	// Draw border
	drawList->AddRect(slotMin, slotMax, borderColor, 4.0f, 0, borderThickness);

	// Draw slot number in top-left corner
	char slotNumText[8];
	snprintf(slotNumText, sizeof(slotNumText), "%d", (slotIndex + 1));
	ImVec2 textPos(posX + 4.0f, posY + 2.0f);
	drawList->AddText(textPos, IM_COL32(200, 200, 200, 255), slotNumText);

	// Draw material name if not empty
	if (!material.isEmpty())
	{
		// Material icon placeholder (colored square representing block type)
		ImVec2 iconMin(posX + SLOT_SIZE * 0.25f, posY + SLOT_SIZE * 0.3f);
		ImVec2 iconMax(posX + SLOT_SIZE * 0.75f, posY + SLOT_SIZE * 0.7f);

		// Color based on category
		ImU32 iconColor = IM_COL32(150, 150, 150, 255); // Default gray

		switch (material.category)
		{
		case Inventory::MaterialCategory::Structure:  iconColor = IM_COL32(120, 120, 150, 255); break; // Blue-gray
		case Inventory::MaterialCategory::Energy:     iconColor = IM_COL32(200, 180, 50, 255);  break; // Gold
		case Inventory::MaterialCategory::Tech:       iconColor = IM_COL32(50, 150, 200, 255);  break; // Cyan
		case Inventory::MaterialCategory::Defensive:  iconColor = IM_COL32(150, 50, 50, 255);   break; // Red
		case Inventory::MaterialCategory::Lighting:   iconColor = IM_COL32(200, 200, 100, 255); break; // Yellow
		case Inventory::MaterialCategory::Natural:    iconColor = IM_COL32(100, 180, 100, 255); break; // Green
		case Inventory::MaterialCategory::Special:    iconColor = IM_COL32(180, 100, 200, 255); break; // Purple
		default: break;
		}

		drawList->AddRectFilled(iconMin, iconMax, iconColor, 2.0f);

		// Material name below slot (only for selected)
		if (isSelected)
		{
			ImVec2 namePos(posX + SLOT_SIZE * 0.5f, posY + SLOT_SIZE + 5.0f);
			ImVec2 textSize = ImGui::CalcTextSize(material.name.c_str());
			namePos.x -= textSize.x * 0.5f; // Center text

			// Draw text shadow for readability
			drawList->AddText(ImVec2(namePos.x + 1, namePos.y + 1), IM_COL32(0, 0, 0, 200), material.name.c_str());
			drawList->AddText(namePos, IM_COL32(0, 255, 255, 255), material.name.c_str());
		}
	}
	else
	{
		// Empty slot indicator
		ImVec2 crossCenter(posX + SLOT_SIZE * 0.5f, posY + SLOT_SIZE * 0.5f);
		float crossSize = SLOT_SIZE * 0.2f;
		ImU32 crossColor = IM_COL32(80, 80, 80, 150);

		drawList->AddLine(
			ImVec2(crossCenter.x - crossSize, crossCenter.y - crossSize),
			ImVec2(crossCenter.x + crossSize, crossCenter.y + crossSize),
			crossColor, 1.5f);
		drawList->AddLine(
			ImVec2(crossCenter.x + crossSize, crossCenter.y - crossSize),
			ImVec2(crossCenter.x - crossSize, crossCenter.y + crossSize),
			crossColor, 1.5f);
	}
}

} // namespace UI
