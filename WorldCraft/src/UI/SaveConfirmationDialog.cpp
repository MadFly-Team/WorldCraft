#include "UI/SaveConfirmationDialog.h"
#include <imgui.h>

namespace UI
{

void SaveConfirmationDialog::show(int chunksCount)
{
	m_isOpen = true;
	m_chunksSaved = chunksCount;
	m_displayTimer = DISPLAY_DURATION;
}

void SaveConfirmationDialog::render()
{
	if (!m_isOpen)
		return;

	// Update timer
	m_displayTimer -= ImGui::GetIO().DeltaTime;
	if (m_displayTimer <= 0.0f)
	{
		m_isOpen = false;
		return;
	}

	// Center the dialog on screen
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.3f);
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(350, 0));

	// Semi-transparent green background for success
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.5f, 0.0f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
							ImGuiWindowFlags_NoMove | 
							ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::Begin("##SaveConfirmation", nullptr, flags))
	{
		ImGui::Spacing();

		// Checkmark icon (using Unicode)
		ImGui::TextWrapped("World Saved Successfully!");
		ImGui::Spacing();

		if (m_chunksSaved > 0)
		{
			ImGui::Text("%d chunk%s saved to disk", m_chunksSaved, m_chunksSaved == 1 ? "" : "s");
		}
		else
		{
			ImGui::TextWrapped("No modified chunks to save");
		}

		ImGui::Spacing();

		// Progress bar showing time remaining
		float progress = m_displayTimer / DISPLAY_DURATION;
		ImGui::ProgressBar(1.0f - progress, ImVec2(-1, 0), "");

		ImGui::Spacing();
	}
	ImGui::End();

	ImGui::PopStyleColor(2);
}

} // namespace UI
