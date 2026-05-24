#include <UI/WorldSelectionDialog.h>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace UI
{

WorldSelectionDialog::WorldSelectionDialog()
	: m_isOpen(false)
	, m_persistence(nullptr)
	, m_selectedIndex(-1)
	, m_showDeleteConfirmation(false)
{
}

bool WorldSelectionDialog::render(Persistence::WorldPersistence* persistence)
{
	if (!m_isOpen) return false;

	m_persistence = persistence;

	ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
							ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

	if (!ImGui::Begin("Select World", &m_isOpen, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return m_isOpen;
	}

	// Header
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Available Worlds");
	ImGui::Separator();
	ImGui::Spacing();

	// World list
	renderWorldList();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Action buttons
	renderActionButtons();

	// Delete confirmation popup
	if (m_showDeleteConfirmation)
	{
		ImGui::OpenPopup("Delete World?");
		m_showDeleteConfirmation = false;
	}

	if (ImGui::BeginPopupModal("Delete World?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to delete this world?");
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_worldToDelete.c_str());
		ImGui::Spacing();
		ImGui::Text("This action cannot be undone!");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button("Delete", ImVec2(120, 0)))
		{
			if (m_persistence && m_persistence->deleteWorld(m_worldToDelete))
			{
				refreshWorldList();
				m_selectedIndex = -1;
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::End();
	return m_isOpen;
}

void WorldSelectionDialog::refreshWorldList()
{
	if (!m_persistence) return;

	m_worlds = m_persistence->listWorlds();
	if (m_selectedIndex >= static_cast<int>(m_worlds.size()))
	{
		m_selectedIndex = -1;
	}
}

void WorldSelectionDialog::renderWorldList()
{
	if (m_worlds.empty())
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No saved worlds found.");
		ImGui::Spacing();
		ImGui::Text("Create a new world from the main menu.");
		return;
	}

	// Show count
	ImGui::Text("Found %zu world(s)", m_worlds.size());
	ImGui::Spacing();

	// Create a scrollable list box for worlds
	ImGui::BeginChild("WorldList", ImVec2(0, 300), true, ImGuiWindowFlags_None);

	for (size_t i = 0; i < m_worlds.size(); ++i)
	{
		const auto& world = m_worlds[i];

		bool isSelected = (static_cast<int>(i) == m_selectedIndex);
		bool isCurrent = (m_persistence && m_persistence->getCurrentWorldName() == world.worldName);

		ImGui::PushID(static_cast<int>(i));

		// Highlight current world
		if (isCurrent)
		{
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.2f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.3f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.45f, 0.25f, 0.8f));
		}

		if (ImGui::Selectable(("##world" + std::to_string(i)).c_str(), isSelected, 0, ImVec2(0, 80)))
		{
			m_selectedIndex = static_cast<int>(i);
		}

		if (isCurrent)
		{
			ImGui::PopStyleColor(3);
		}

		// Draw world info on the same line
		ImGui::SameLine();

		ImGui::BeginGroup();

		// World name with index for duplicates
		if (isCurrent)
		{
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", world.worldName.c_str());
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[CURRENT]");
		}
		else
		{
			ImGui::Text("%s", world.worldName.c_str());
		}

		// Last played
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Last played: %s", formatTimestamp(world.lastPlayedTime).c_str());

		// World details including UUID snippet for identification
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Seed: %d | Modified chunks: %d | ID: %s", 
						   world.settings.seed, world.modifiedChunkCount, world.worldUUID.substr(0, 8).c_str());

		ImGui::EndGroup();

		ImGui::PopID();
	}

	ImGui::EndChild();

	// Show selected world details
	if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_worlds.size()))
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		renderWorldDetails(m_worlds[m_selectedIndex]);
	}
}

void WorldSelectionDialog::renderWorldDetails(const Persistence::WorldMetadata& world)
{
	ImGui::Text("World Details:");
	ImGui::Spacing();

	ImGui::Columns(2, "worlddetails", false);
	ImGui::SetColumnWidth(0, 150);

	ImGui::Text("Name:");
	ImGui::NextColumn();
	ImGui::Text("%s", world.worldName.c_str());
	ImGui::NextColumn();

	ImGui::Text("Seed:");
	ImGui::NextColumn();
	ImGui::Text("%d", world.settings.seed);
	ImGui::NextColumn();

	ImGui::Text("Created:");
	ImGui::NextColumn();
	ImGui::Text("%s", formatTimestamp(world.creationTime).c_str());
	ImGui::NextColumn();

	ImGui::Text("Modified Chunks:");
	ImGui::NextColumn();
	ImGui::Text("%d", world.modifiedChunkCount);
	ImGui::NextColumn();

	ImGui::Columns(1);
}

void WorldSelectionDialog::renderActionButtons()
{
	bool hasSelection = m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_worlds.size());
	bool isCurrent = hasSelection && m_persistence && 
					 m_persistence->getCurrentWorldName() == m_worlds[m_selectedIndex].worldName;

	ImGui::BeginDisabled(!hasSelection || isCurrent);
	if (ImGui::Button("Load World", ImVec2(150, 30)))
	{
		if (hasSelection && m_onLoadWorld)
		{
			m_onLoadWorld(m_worlds[m_selectedIndex].worldName);
			hide();
		}
	}
	ImGui::EndDisabled();

	if (!hasSelection)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a world to load");
	}
	else if (isCurrent)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "This world is currently loaded");
	}

	ImGui::SameLine(ImGui::GetWindowWidth() - 170);
	ImGui::BeginDisabled(!hasSelection || isCurrent);
	if (ImGui::Button("Delete", ImVec2(80, 30)))
	{
		if (hasSelection)
		{
			m_worldToDelete = m_worlds[m_selectedIndex].worldName;
			m_showDeleteConfirmation = true;
		}
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(80, 30)))
	{
		hide();
	}
}

std::string WorldSelectionDialog::formatTimestamp(time_t timestamp) const
{
	if (timestamp == 0)
		return "Never";

	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &timestamp);
#else
	localtime_r(&timestamp, &tm);
#endif

	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	return oss.str();
}

std::string WorldSelectionDialog::formatFileSize(uint64_t bytes) const
{
	const char* units[] = { "B", "KB", "MB", "GB" };
	int unit = 0;
	double size = static_cast<double>(bytes);

	while (size >= 1024.0 && unit < 3)
	{
		size /= 1024.0;
		++unit;
	}

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
	return oss.str();
}

} // namespace UI
