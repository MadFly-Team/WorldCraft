#include <UI/LoadingScreen.h>
#include <imgui.h>
#include <cstdio>

namespace UI
{

void LoadingScreen::render(float progress, int loadedChunks, int targetChunks)
{
	if (!m_isActive)
		return;

	// Create a centered modal-style window
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 windowSize(400, 150);
	ImVec2 windowPos(io.DisplaySize.x * 0.5f - windowSize.x * 0.5f,
					 io.DisplaySize.y * 0.5f - windowSize.y * 0.5f);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	// Modal style window with no close button
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
							 ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoCollapse |
							 ImGuiWindowFlags_NoScrollbar;

	if (ImGui::Begin("##LoadingScreen", nullptr, flags))
	{
		ImGui::SetWindowFontScale(1.2f);

		// Title
		ImGui::Spacing();
		ImGui::Spacing();
		const char* title = "Loading World...";
		float titleWidth = ImGui::CalcTextSize(title).x;
		ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5f);
		ImGui::Text("%s", title);

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		// Progress bar
		ImGui::SetCursorPosX(20);
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
		ImGui::ProgressBar(progress, ImVec2(windowSize.x - 40, 30), "");
		ImGui::PopStyleColor();

		ImGui::Spacing();

		// Percentage text
		char progressText[64];
		snprintf(progressText, sizeof(progressText), "%.0f%% (%d / %d chunks)",
				 progress * 100.0f, loadedChunks, targetChunks);
		float textWidth = ImGui::CalcTextSize(progressText).x;
		ImGui::SetCursorPosX((windowSize.x - textWidth) * 0.5f);
		ImGui::Text("%s", progressText);

		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::End();
}

} // namespace UI
