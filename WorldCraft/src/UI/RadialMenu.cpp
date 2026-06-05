#include <UI/RadialMenu.h>
#include <imgui.h>
#include <cmath>
#include <algorithm>

namespace UI
{

RadialMenu::RadialMenu()
	: m_visible(false)
	, m_hoveredCategoryIndex(-1)
	, m_hoveredMaterialIndex(-1)
{
}

void RadialMenu::show()
{
	m_visible = true;
	m_hoveredCategoryIndex = -1;
	m_hoveredMaterialIndex = -1;
	m_currentCategoryMaterials.clear();
}

void RadialMenu::hide()
{
	m_visible = false;
	m_hoveredCategoryIndex = -1;
	m_hoveredMaterialIndex = -1;
	m_currentCategoryMaterials.clear();
}

bool RadialMenu::update(Inventory::PlayerInventory& inventory, float mouseX, float mouseY, 
						int screenWidth, int screenHeight)
{
	if (!m_visible)
		return false;

	float centerX = screenWidth * 0.5f;
	float centerY = screenHeight * 0.5f;

	// First check if hovering over a category
	int categoryIndex = getCategoryUnderMouse(mouseX, mouseY, centerX, centerY);

	if (categoryIndex != m_hoveredCategoryIndex)
	{
		m_hoveredCategoryIndex = categoryIndex;

		// Load materials for this category
		if (categoryIndex >= 0)
		{
			Inventory::MaterialCategory category = static_cast<Inventory::MaterialCategory>(categoryIndex);
			m_currentCategoryMaterials = Inventory::PlayerInventory::getMaterialsByCategory(category);
		}
		else
		{
			m_currentCategoryMaterials.clear();
		}

		m_hoveredMaterialIndex = -1;
	}

	// If a category is selected, check if hovering over a material
	if (m_hoveredCategoryIndex >= 0 && !m_currentCategoryMaterials.empty())
	{
		m_hoveredMaterialIndex = getMaterialUnderMouse(mouseX, mouseY, centerX, centerY, m_currentCategoryMaterials);
	}

	return false; // Selection happens on mouse release in main loop
}

bool RadialMenu::selectHoveredMaterial(Inventory::PlayerInventory& inventory)
{
	if (m_hoveredMaterialIndex >= 0 && m_hoveredMaterialIndex < static_cast<int>(m_currentCategoryMaterials.size()))
	{
		// Equip the selected material to the current hotbar slot
		const Inventory::Material& selectedMaterial = m_currentCategoryMaterials[m_hoveredMaterialIndex];
		inventory.setMaterial(inventory.getSelectedSlot(), selectedMaterial);
		return true;
	}
	return false;
}

void RadialMenu::render(int screenWidth, int screenHeight)
{
	if (!m_visible)
		return;

	float centerX = screenWidth * 0.5f;
	float centerY = screenHeight * 0.5f;

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();

	// Draw semi-transparent overlay
	drawList->AddRectFilled(ImVec2(0, 0), ImVec2(static_cast<float>(screenWidth), static_cast<float>(screenHeight)),
							IM_COL32(0, 0, 0, 150));

	// Draw center indicator
	drawList->AddCircleFilled(ImVec2(centerX, centerY), 10.0f, IM_COL32(0, 255, 255, 255), 32);

	// Draw category segments
	constexpr int segments = 64;
	const float angleStep = (2.0f * 3.14159f) / NUM_CATEGORIES;

	for (int i = 0; i < NUM_CATEGORIES; ++i)
	{
		float startAngle, endAngle;
		getCategoryAngleRange(i, startAngle, endAngle);

		bool isHovered = (i == m_hoveredCategoryIndex);

		// Category color based on type
		ImU32 categoryColor = IM_COL32(100, 100, 120, 200);
		Inventory::MaterialCategory category = static_cast<Inventory::MaterialCategory>(i);

		switch (category)
		{
		case Inventory::MaterialCategory::Structure:  categoryColor = IM_COL32(120, 120, 150, 200); break;
		case Inventory::MaterialCategory::Energy:     categoryColor = IM_COL32(200, 180, 50, 200);  break;
		case Inventory::MaterialCategory::Tech:       categoryColor = IM_COL32(50, 150, 200, 200);  break;
		case Inventory::MaterialCategory::Defensive:  categoryColor = IM_COL32(150, 50, 50, 200);   break;
		case Inventory::MaterialCategory::Lighting:   categoryColor = IM_COL32(200, 200, 100, 200); break;
		case Inventory::MaterialCategory::Natural:    categoryColor = IM_COL32(100, 180, 100, 200); break;
		case Inventory::MaterialCategory::Special:    categoryColor = IM_COL32(180, 100, 200, 200); break;
		default: break;
		}

		if (isHovered)
		{
			// Brighten when hovered
			int r = std::min(255, static_cast<int>(((categoryColor >> 0) & 0xFF) + 50));
			int g = std::min(255, static_cast<int>(((categoryColor >> 8) & 0xFF) + 50));
			int b = std::min(255, static_cast<int>(((categoryColor >> 16) & 0xFF) + 50));
			categoryColor = IM_COL32(r, g, b, 255);
		}

		// Draw category wedge
		drawList->PathArcTo(ImVec2(centerX, centerY), OUTER_RADIUS, startAngle, endAngle, segments / NUM_CATEGORIES);
		drawList->PathArcTo(ImVec2(centerX, centerY), INNER_RADIUS, endAngle, startAngle, segments / NUM_CATEGORIES);
		drawList->PathFillConvex(categoryColor);

		// Draw border
		ImU32 borderColor = isHovered ? IM_COL32(0, 255, 255, 255) : IM_COL32(80, 80, 100, 255);
		drawList->PathArcTo(ImVec2(centerX, centerY), OUTER_RADIUS, startAngle, endAngle, segments / NUM_CATEGORIES);
		drawList->PathStroke(borderColor, 0, isHovered ? 3.0f : 2.0f);
		drawList->PathArcTo(ImVec2(centerX, centerY), INNER_RADIUS, startAngle, endAngle, segments / NUM_CATEGORIES);
		drawList->PathStroke(borderColor, 0, isHovered ? 3.0f : 2.0f);

		// Draw category label
		std::string categoryName = Inventory::PlayerInventory::getCategoryName(category);
		float labelAngle = (startAngle + endAngle) * 0.5f;
		float labelRadius = (INNER_RADIUS + OUTER_RADIUS) * 0.5f;
		float labelX = centerX + std::cos(labelAngle) * labelRadius;
		float labelY = centerY + std::sin(labelAngle) * labelRadius;

		ImVec2 textSize = ImGui::CalcTextSize(categoryName.c_str());
		ImVec2 textPos(labelX - textSize.x * 0.5f, labelY - textSize.y * 0.5f);

		// Text shadow
		drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 255), categoryName.c_str());
		drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), categoryName.c_str());
	}

	// Draw materials for selected category
	if (m_hoveredCategoryIndex >= 0 && !m_currentCategoryMaterials.empty())
	{
		int numMaterials = static_cast<int>(m_currentCategoryMaterials.size());
		float materialAngleStep = (2.0f * 3.14159f) / numMaterials;

		for (int i = 0; i < numMaterials; ++i)
		{
			float angle = i * materialAngleStep - (3.14159f * 0.5f); // Start at top
			float matX = centerX + std::cos(angle) * MATERIAL_RING_RADIUS;
			float matY = centerY + std::sin(angle) * MATERIAL_RING_RADIUS;

			bool isHovered = (i == m_hoveredMaterialIndex);
			float iconSize = isHovered ? 45.0f : 35.0f;

			// Material icon background
			ImU32 bgColor = isHovered ? IM_COL32(0, 200, 255, 255) : IM_COL32(50, 50, 70, 220);
			drawList->AddCircleFilled(ImVec2(matX, matY), iconSize, bgColor, 32);

			// Material icon (colored square)
			const Inventory::Material& mat = m_currentCategoryMaterials[i];
			ImU32 iconColor = IM_COL32(150, 150, 150, 255);

			switch (mat.category)
			{
			case Inventory::MaterialCategory::Structure:  iconColor = IM_COL32(120, 120, 150, 255); break;
			case Inventory::MaterialCategory::Energy:     iconColor = IM_COL32(200, 180, 50, 255);  break;
			case Inventory::MaterialCategory::Tech:       iconColor = IM_COL32(50, 150, 200, 255);  break;
			case Inventory::MaterialCategory::Defensive:  iconColor = IM_COL32(150, 50, 50, 255);   break;
			case Inventory::MaterialCategory::Lighting:   iconColor = IM_COL32(200, 200, 100, 255); break;
			case Inventory::MaterialCategory::Natural:    iconColor = IM_COL32(100, 180, 100, 255); break;
			case Inventory::MaterialCategory::Special:    iconColor = IM_COL32(180, 100, 200, 255); break;
			default: break;
			}

			float iconHalfSize = iconSize * 0.5f;
			drawList->AddRectFilled(
				ImVec2(matX - iconHalfSize, matY - iconHalfSize),
				ImVec2(matX + iconHalfSize, matY + iconHalfSize),
				iconColor, 4.0f);

			// Material name
			if (isHovered)
			{
				ImVec2 textSize = ImGui::CalcTextSize(mat.name.c_str());
				ImVec2 textPos(matX - textSize.x * 0.5f, matY + iconSize + 10.0f);

				drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 255), mat.name.c_str());
				drawList->AddText(textPos, IM_COL32(0, 255, 255, 255), mat.name.c_str());
			}
		}
	}

	// Instructions text
	const char* instructions = "Hold Q - Move mouse to select - Release to equip";
	ImVec2 textSize = ImGui::CalcTextSize(instructions);
	ImVec2 textPos(centerX - textSize.x * 0.5f, centerY + MATERIAL_RING_RADIUS + 100.0f);
	drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 255), instructions);
	drawList->AddText(textPos, IM_COL32(200, 200, 200, 255), instructions);
}

void RadialMenu::getCategoryAngleRange(int categoryIndex, float& startAngle, float& endAngle) const
{
	const float angleStep = (2.0f * 3.14159f) / NUM_CATEGORIES;
	const float offset = -3.14159f * 0.5f; // Start at top (12 o'clock)

	startAngle = categoryIndex * angleStep + offset;
	endAngle = (categoryIndex + 1) * angleStep + offset;
}

int RadialMenu::getCategoryUnderMouse(float mouseX, float mouseY, float centerX, float centerY) const
{
	float dx = mouseX - centerX;
	float dy = mouseY - centerY;
	float distSq = dx * dx + dy * dy;

	// Check if within category ring
	if (distSq < INNER_RADIUS * INNER_RADIUS || distSq > OUTER_RADIUS * OUTER_RADIUS)
		return -1;

	// Calculate angle
	float angle = std::atan2(dy, dx);
	if (angle < 0.0f) angle += 2.0f * 3.14159f;

	// Adjust for offset (start at top)
	angle += 3.14159f * 0.5f;
	if (angle >= 2.0f * 3.14159f) angle -= 2.0f * 3.14159f;

	// Determine category
	const float angleStep = (2.0f * 3.14159f) / NUM_CATEGORIES;
	int category = static_cast<int>(angle / angleStep);

	if (category >= 0 && category < NUM_CATEGORIES)
		return category;

	return -1;
}

int RadialMenu::getMaterialUnderMouse(float mouseX, float mouseY, float centerX, float centerY,
									  const std::vector<Inventory::Material>& materials) const
{
	if (materials.empty())
		return -1;

	int numMaterials = static_cast<int>(materials.size());
	float materialAngleStep = (2.0f * 3.14159f) / numMaterials;
	constexpr float HOVER_RADIUS = 50.0f;

	for (int i = 0; i < numMaterials; ++i)
	{
		float angle = i * materialAngleStep - (3.14159f * 0.5f);
		float matX = centerX + std::cos(angle) * MATERIAL_RING_RADIUS;
		float matY = centerY + std::sin(angle) * MATERIAL_RING_RADIUS;

		float dx = mouseX - matX;
		float dy = mouseY - matY;
		float distSq = dx * dx + dy * dy;

		if (distSq < HOVER_RADIUS * HOVER_RADIUS)
			return i;
	}

	return -1;
}

} // namespace UI
