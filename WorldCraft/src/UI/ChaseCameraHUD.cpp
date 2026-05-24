#include <UI/ChaseCameraHUD.h>
#include <imgui.h>
#include <cmath>
#include <cstdio>

namespace UI
{

void ChaseCameraHUD::render(const glm::vec3& currentPos, 
							 const glm::vec3& targetPos, 
							 float currentSpeed,
							 float distanceRemaining)
{
	if (!m_isVisible)
		return;

	ImGuiIO& io = ImGui::GetIO();

	// Window size and positioning
	float windowWidth = 600.0f;  // Increased width to accommodate time display
	float windowHeight = 140.0f;
	float padding = 20.0f;

	// Position at top center
	ImVec2 windowPos(io.DisplaySize.x * 0.5f - windowWidth * 0.5f, padding);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

	// Semi-transparent background with no title bar
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
							 ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoCollapse |
							 ImGuiWindowFlags_NoScrollbar;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));

	if (ImGui::Begin("##ChaseCameraHUD", nullptr, flags))
	{
		// Title
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f)); // Cyan
		const char* title = "CHASE CAMERA - Flying to Destination";
		float titleWidth = ImGui::CalcTextSize(title).x;
		ImGui::SetCursorPosX((windowWidth - titleWidth) * 0.5f);
		ImGui::Text("%s", title);
		ImGui::PopStyleColor();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Current position
		ImGui::Text("Current:  ");
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 1.0f, 0.7f, 1.0f)); // Light green
		ImGui::Text("X: %.1f  Y: %.1f  Z: %.1f", currentPos.x, currentPos.y, currentPos.z);
		ImGui::PopStyleColor();

		// Destination position
		ImGui::Text("Destination: ");
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.7f, 1.0f)); // Light yellow
		ImGui::Text("X: %.1f  Y: %.1f  Z: %.1f", targetPos.x, targetPos.y, targetPos.z);
		ImGui::PopStyleColor();

		ImGui::Spacing();

		// Calculate ETA (time = distance / speed, but account for acceleration/deceleration)
		float eta = 0.0f;
		if (currentSpeed > 0.1f)
		{
			// Simple approximation: use current speed
			eta = distanceRemaining / currentSpeed;
		}
		else
		{
			// Not moving yet, estimate based on average speed
			eta = distanceRemaining / 20.0f;  // Rough average
		}

		// Speed and Time Remaining on same line
		ImGui::Text("Speed: ");
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 1.0f, 1.0f)); // Light blue
		ImGui::Text("%.1f units/s", currentSpeed);
		ImGui::PopStyleColor();

		// Time Remaining - right-aligned
		ImGui::SameLine();

		// Calculate time string first to get its width
		char timeStr[64];
		if (eta < 1.0f)
		{
			snprintf(timeStr, sizeof(timeStr), "< 1s");
		}
		else if (eta < 60.0f)
		{
			snprintf(timeStr, sizeof(timeStr), "%.0fs", eta);
		}
		else
		{
			int minutes = static_cast<int>(eta) / 60;
			int seconds = static_cast<int>(eta) % 60;
			snprintf(timeStr, sizeof(timeStr), "%dm %ds", minutes, seconds);
		}

		// Calculate position for right alignment
		const char* label = "Time Remaining: ";
		float labelWidth = ImGui::CalcTextSize(label).x;
		float timeWidth = ImGui::CalcTextSize(timeStr).x;
		float totalWidth = labelWidth + timeWidth;
		float availableWidth = windowWidth - ImGui::GetCursorPosX() - 20.0f; // 20 for padding

		if (totalWidth < availableWidth)
		{
			ImGui::SetCursorPosX(windowWidth - totalWidth - 20.0f);
		}

		ImGui::Text("%s", label);
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.7f, 1.0f)); // Light yellow
		ImGui::Text("%s", timeStr);
		ImGui::PopStyleColor();
	}
	ImGui::End();

	ImGui::PopStyleColor(); // WindowBg
}

} // namespace UI
