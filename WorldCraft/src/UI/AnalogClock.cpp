#include <UI/AnalogClock.h>
#include <imgui.h>
#include <cmath>
#include <cstdio>

namespace UI
{

void AnalogClock::render(float timeOfDay, int screenWidth, int screenHeight)
{
	if (!m_isVisible)
		return;

	// Calculate clock size - 1/20th of screen width
	float clockSize = static_cast<float>(screenWidth) / 20.0f;
	float radius = clockSize * 0.5f;

	// Position in top-left with padding
	float padding = 20.0f;
	ImVec2 center(padding + radius, padding + radius);

	// Create an invisible window for the clock
	ImGui::SetNextWindowPos(ImVec2(padding, padding), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(clockSize, clockSize), ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
							 ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoScrollbar |
							 ImGuiWindowFlags_NoInputs |
							 ImGuiWindowFlags_NoBackground;

	ImGui::Begin("##AnalogClock", nullptr, flags);

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// Convert timeOfDay (0.0-1.0) to hours (0-24)
	float hours24 = timeOfDay * 24.0f;
	float hours12 = std::fmod(hours24, 12.0f);
	float minutes = std::fmod(hours24 * 60.0f, 60.0f);

	// Calculate lighting based on time of day
	// Dawn: 0.2-0.3 (4:48-7:12), Day: 0.3-0.7 (7:12-16:48), Dusk: 0.7-0.8 (16:48-19:12), Night: 0.8-0.2
	float brightness = 0.0f;
	if (timeOfDay >= 0.25f && timeOfDay <= 0.75f) {
		// Day time - bright (6:00 - 18:00)
		if (timeOfDay <= 0.5f) {
			// Morning - fade in from 0.25 to 0.5
			brightness = (timeOfDay - 0.25f) / 0.25f;
		} else {
			// Afternoon - fade out from 0.5 to 0.75
			brightness = 1.0f - (timeOfDay - 0.5f) / 0.25f;
		}
		brightness = brightness * 0.7f + 0.3f;  // Scale to 0.3-1.0 range
	} else {
		// Night time - dark
		brightness = 0.15f;
	}

	// Clock face background - lerp between dark night and lighter day
	int bgR = static_cast<int>(20 + brightness * 180);   // 20 -> 200
	int bgG = static_cast<int>(20 + brightness * 200);   // 20 -> 220
	int bgB = static_cast<int>(30 + brightness * 210);   // 30 -> 240
	drawList->AddCircleFilled(center, radius, IM_COL32(bgR, bgG, bgB, 200));

	// Clock face border - darker at night, brighter during day
	int borderR = static_cast<int>(100 + brightness * 155);
	int borderG = static_cast<int>(100 + brightness * 155);
	int borderB = static_cast<int>(100 + brightness * 155);
	drawList->AddCircle(center, radius, IM_COL32(borderR, borderG, borderB, 255), 64, 2.5f);

	// Draw hour markers (12, 3, 6, 9)
	const ImU32 markerColor = IM_COL32(200, 200, 200, 255);
	for (int i = 0; i < 12; ++i)
	{
		float angle = (i * 30.0f - 90.0f) * (3.14159f / 180.0f);
		float markerRadius = (i % 3 == 0) ? radius * 0.85f : radius * 0.90f;
		float markerLength = (i % 3 == 0) ? radius * 0.15f : radius * 0.10f;

		ImVec2 start(center.x + std::cos(angle) * markerRadius,
					 center.y + std::sin(angle) * markerRadius);
		ImVec2 end(center.x + std::cos(angle) * (markerRadius + markerLength),
				   center.y + std::sin(angle) * (markerRadius + markerLength));

		float thickness = (i % 3 == 0) ? 2.5f : 1.5f;
		drawList->AddLine(start, end, markerColor, thickness);
	}

	// Draw hour hand (shorter, thicker)
	float hourAngle = (hours12 / 12.0f * 360.0f - 90.0f) * (3.14159f / 180.0f);
	float hourLength = radius * 0.50f;
	ImVec2 hourEnd(center.x + std::cos(hourAngle) * hourLength,
				   center.y + std::sin(hourAngle) * hourLength);

	// Hour hand with gradient effect - draw shadow first
	drawList->AddLine(ImVec2(center.x + 1, center.y + 1), 
					  ImVec2(hourEnd.x + 1, hourEnd.y + 1),
					  IM_COL32(0, 0, 0, 100), 4.0f);
	drawList->AddLine(center, hourEnd, IM_COL32(255, 255, 255, 255), 4.0f);

	// Draw minute hand (longer, thinner)
	float minuteAngle = (minutes / 60.0f * 360.0f - 90.0f) * (3.14159f / 180.0f);
	float minuteLength = radius * 0.75f;
	ImVec2 minuteEnd(center.x + std::cos(minuteAngle) * minuteLength,
					 center.y + std::sin(minuteAngle) * minuteLength);

	// Minute hand with shadow
	drawList->AddLine(ImVec2(center.x + 1, center.y + 1),
					  ImVec2(minuteEnd.x + 1, minuteEnd.y + 1),
					  IM_COL32(0, 0, 0, 100), 3.0f);
	drawList->AddLine(center, minuteEnd, IM_COL32(220, 220, 255, 255), 3.0f);

	// Center dot
	drawList->AddCircleFilled(center, radius * 0.08f, IM_COL32(255, 255, 255, 255));
	drawList->AddCircle(center, radius * 0.08f, IM_COL32(100, 100, 100, 255), 16, 1.5f);

	// Time text below clock (digital format for clarity)
	int hour24 = static_cast<int>(hours24) % 24;
	int min = static_cast<int>(minutes);
	char timeText[16];
	snprintf(timeText, sizeof(timeText), "%02d:%02d", hour24, min);

	ImVec2 textSize = ImGui::CalcTextSize(timeText);
	ImVec2 textPos(center.x - textSize.x * 0.5f, center.y + radius + 5.0f);

	// Text shadow
	drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), 
					  IM_COL32(0, 0, 0, 180), timeText);
	drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), timeText);

	ImGui::End();
}

} // namespace UI
