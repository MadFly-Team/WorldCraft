#include <UI/SettingsDialog.h>
#include <imgui.h>
#include <cstring>
#include <random>

namespace UI
{

SettingsDialog::SettingsDialog()
	: m_settings(WorldGen::WorldSettings::createDefault())
{
}

bool SettingsDialog::render()
{
	if (!m_isOpen)
		return false;

	// Main settings window - modal style
	ImGui::SetNextWindowSize(ImVec2(750, 850), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), 
							 ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

	// Make window more prominent with flags
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	if (!ImGui::Begin("World Generation Settings", &m_isOpen, flags))
	{
		ImGui::End();
		return m_isOpen;
	}

	// Handle ESC key to close dialog
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		m_isOpen = false;
	}

	// Render different sections
	renderPresets();
	ImGui::Separator();

	renderBasicSettings();
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Terrain Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		renderTerrainSettings();
	}

	if (ImGui::CollapsingHeader("Biome Settings"))
	{
		renderBiomeSettings();
	}

	if (ImGui::CollapsingHeader("Cave Settings"))
	{
		renderCaveSettings();
	}

	if (ImGui::CollapsingHeader("Ore Settings"))
	{
		renderOreSettings();
	}

	if (ImGui::CollapsingHeader("Tree Settings"))
	{
		renderTreeSettings();
	}

	if (ImGui::CollapsingHeader("Water Settings"))
	{
		renderWaterSettings();
	}

	if (ImGui::CollapsingHeader("Camera Settings"))
	{
		renderCameraSettings();
	}

	if (ImGui::CollapsingHeader("Time Settings"))
	{
		renderTimeSettings();
	}

	if (ImGui::CollapsingHeader("World Persistence"))
	{
		renderPersistenceSettings();
	}

	ImGui::Separator();
	renderGenerateButton();

	ImGui::End();
	return m_isOpen;
}

void SettingsDialog::renderPresets()
{
	ImGui::Text("Presets:");
	ImGui::SameLine();

	const char* presets[] = { "Default", "Flat", "Mountainous", "Islands", "Cave World" };

	if (ImGui::Combo("##Preset", &m_selectedPreset, presets, IM_ARRAYSIZE(presets)))
	{
		// Apply preset
		switch (m_selectedPreset)
		{
		case 0: m_settings = WorldGen::WorldSettings::createDefault(); break;
		case 1: m_settings = WorldGen::WorldSettings::createFlat(); break;
		case 2: m_settings = WorldGen::WorldSettings::createMountainous(); break;
		case 3: m_settings = WorldGen::WorldSettings::createIslands(); break;
		case 4: m_settings = WorldGen::WorldSettings::createCaveWorld(); break;
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Reset to Defaults"))
	{
		m_settings = WorldGen::WorldSettings::createDefault();
		m_selectedPreset = 0;
	}
}

void SettingsDialog::renderBasicSettings()
{
	ImGui::Text("Basic Settings");

	ImGui::InputText("World Name", m_settings.worldName, sizeof(m_settings.worldName));

	ImGui::InputInt("Seed", &m_settings.seed);
	ImGui::SameLine();
	if (ImGui::Button("Random"))
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dist(1, 999999999);
		m_settings.seed = dist(gen);
	}

	ImGui::SliderInt("Render Distance", &m_settings.renderDistance, 4, 32);
	ImGui::TextDisabled("(Higher values require more memory)");
}

void SettingsDialog::renderTerrainSettings()
{
	ImGui::SliderFloat("Terrain Scale", &m_settings.terrainScale, 20.0f, 300.0f, "%.1f");
	ImGui::TextDisabled("Larger = smoother, more gradual terrain");

	ImGui::SliderFloat("Terrain Amplitude", &m_settings.terrainAmplitude, 0.0f, 150.0f, "%.1f");
	ImGui::TextDisabled("Larger = taller mountains and deeper valleys");

	ImGui::SliderInt("Sea Level", &m_settings.seaLevel, 32, 128);
	ImGui::TextDisabled("Y coordinate where water spawns");
}

void SettingsDialog::renderBiomeSettings()
{
	ImGui::SliderFloat("Biome Scale", &m_settings.biomeScale, 100.0f, 1000.0f, "%.1f");
	ImGui::TextDisabled("Larger = bigger biome regions");

	ImGui::SliderFloat("Biome Blend", &m_settings.biomeBlend, 0.0f, 1.0f, "%.2f");
	ImGui::TextDisabled("Controls smoothness of biome transitions");
}

void SettingsDialog::renderCaveSettings()
{
	// Mountain entrance caves
	ImGui::Checkbox("Enable Mountain Caves", &m_settings.enableMountainCaves);
	ImGui::TextDisabled("Large cave entrances on mountainsides");

	if (m_settings.enableMountainCaves)
	{
		ImGui::Indent();
		ImGui::SliderFloat("Mountain Cave Rarity", &m_settings.mountainCaveRarity, 200.0f, 2000.0f, "%.0f blocks");
		ImGui::TextDisabled("Distance between mountain caves (higher = rarer)");

		ImGui::SliderFloat("Mountain Cave Size", &m_settings.mountainCaveSize, 15.0f, 50.0f, "%.0f blocks");
		ImGui::TextDisabled("Size of cave entrance and tunnels");
		ImGui::Unindent();
	}

	ImGui::Spacing();

	// Deep underground caverns
	ImGui::Checkbox("Enable Deep Caverns", &m_settings.enableDeepCaverns);
	ImGui::TextDisabled("Massive underground chambers with ore veins");

	if (m_settings.enableDeepCaverns)
	{
		ImGui::Indent();
		ImGui::SliderFloat("Cavern Size", &m_settings.cavernSize, 20.0f, 80.0f, "%.0f");
		ImGui::TextDisabled("Size of underground chambers (higher = larger)");

		ImGui::SliderInt("Cavern Max Depth", &m_settings.cavernMaxDepth, 20, 100);
		ImGui::TextDisabled("Caverns spawn below this Y level");

		ImGui::SliderInt("Cavern Min Depth", &m_settings.cavernMinDepth, 1, 30);
		ImGui::TextDisabled("Caverns don't spawn above this Y level");
		ImGui::Unindent();
	}
}

void SettingsDialog::renderOreSettings()
{
	ImGui::Checkbox("Enable Ores", &m_settings.enableOres);

	if (m_settings.enableOres)
	{
		ImGui::Indent();
		ImGui::SliderFloat("Ore Abundance", &m_settings.oreAbundance, 0.1f, 5.0f, "%.1fx");
		ImGui::TextDisabled("Multiplier for ore spawn rates");
		ImGui::Unindent();
	}
}

void SettingsDialog::renderTreeSettings()
{
	ImGui::Checkbox("Enable Trees", &m_settings.enableTrees);

	if (m_settings.enableTrees)
	{
		ImGui::Indent();
		ImGui::SliderFloat("Tree Density", &m_settings.treeDensity, 0.0f, 0.1f, "%.3f");
		ImGui::TextDisabled("Probability of tree spawning per suitable block");
		ImGui::Unindent();
	}
}

void SettingsDialog::renderWaterSettings()
{
	ImGui::Checkbox("Enable Water Flow", &m_settings.enableWaterFlow);
	ImGui::TextDisabled("Water spreads into adjacent air spaces when blocks are removed");

	if (m_settings.enableWaterFlow)
	{
		ImGui::Indent();
		ImGui::SliderFloat("Flow Rate", &m_settings.waterFlowRate, 2.0f, 10.0f, "%.1f ticks/sec");
		ImGui::TextDisabled("Controls water speed and lake activation rate");
		ImGui::Unindent();
	}

	ImGui::Spacing();
	ImGui::Checkbox("Enable Water Waves", &m_settings.enableWaterWaves);
	ImGui::TextDisabled("Visual wave effect on water surfaces (performance impact)");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Text("Persistence Settings");
	ImGui::Spacing();

	ImGui::Checkbox("Enable Auto-Save", &m_settings.enableAutoSave);
	ImGui::TextDisabled("Automatically save modified chunks every 5 minutes");
}

void SettingsDialog::renderCameraSettings()
{
	ImGui::Text("Start Position for Camera Navigation");
	ImGui::Spacing();

	ImGui::InputFloat("X Position", &m_settings.startX, 1.0f, 10.0f, "%.1f");
	ImGui::InputFloat("Y Position", &m_settings.startY, 1.0f, 10.0f, "%.1f");
	ImGui::InputFloat("Z Position", &m_settings.startZ, 1.0f, 10.0f, "%.1f");

	ImGui::Spacing();
	ImGui::TextDisabled("Click 'Go To Position' to fly the camera to this location");

	if (ImGui::Button("Go To Position", ImVec2(150.0f, 30.0f)))
	{
		if (m_onGoToPosition)
		{
			m_onGoToPosition(m_settings.startX, m_settings.startY, m_settings.startZ);
		}
	}
}

void SettingsDialog::renderGenerateButton()
{
	ImGui::Spacing();
	ImGui::Spacing();

	// Center the button
	float buttonWidth = 200.0f;
	float windowWidth = ImGui::GetWindowWidth();
	ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

	if (ImGui::Button("Generate New World", ImVec2(buttonWidth, 40.0f)))
	{
		if (m_onGenerateWorld)
		{
			m_onGenerateWorld(m_settings);
		}
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// Help text centered
	ImGui::SetCursorPosX((windowWidth - 350.0f) * 0.5f);
	ImGui::TextDisabled("F11: Toggle  |  ESC: Close  |  Alt+Enter: Fullscreen");
}

void SettingsDialog::renderTimeSettings()
{
	ImGui::Text("Day/Night Cycle Control");
	ImGui::Spacing();

	// Time of day slider (0.0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset, 1.0 = midnight)
	float hours24 = m_timeOfDay * 24.0f;
	int hour = static_cast<int>(hours24) % 24;
	int minute = static_cast<int>((hours24 - hour) * 60.0f);

	ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Current Time: %02d:%02d", hour, minute);

	if (ImGui::SliderFloat("Time of Day", &m_timeOfDay, 0.0f, 1.0f, ""))
	{
		// Only apply if not using live time
		if (!m_useLiveTime && m_onTimeSettings)
		{
			m_onTimeSettings(m_timeOfDay, m_timePaused, m_useLiveTime);
		}
	}

	// Time presets
	ImGui::Spacing();
	ImGui::Text("Quick Presets:");

	if (ImGui::Button("Sunrise (06:00)", ImVec2(130.0f, 0.0f)))
	{
		m_timeOfDay = 0.25f;
		m_useLiveTime = false;
		if (m_onTimeSettings)
			m_onTimeSettings(m_timeOfDay, m_timePaused, m_useLiveTime);
	}
	ImGui::SameLine();
	if (ImGui::Button("Noon (12:00)", ImVec2(130.0f, 0.0f)))
	{
		m_timeOfDay = 0.5f;
		m_useLiveTime = false;
		if (m_onTimeSettings)
			m_onTimeSettings(m_timeOfDay, m_timePaused, m_useLiveTime);
	}
	ImGui::SameLine();
	if (ImGui::Button("Sunset (18:00)", ImVec2(130.0f, 0.0f)))
	{
		m_timeOfDay = 0.75f;
		m_useLiveTime = false;
		if (m_onTimeSettings)
			m_onTimeSettings(m_timeOfDay, m_timePaused, m_useLiveTime);
	}

	if (ImGui::Button("Midnight (00:00)", ImVec2(130.0f, 0.0f)))
	{
		m_timeOfDay = 0.0f;
		m_useLiveTime = false;
		if (m_onTimeSettings)
			m_onTimeSettings(m_timeOfDay, m_timePaused, m_useLiveTime);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Pause/Resume time
	if (ImGui::Checkbox("Pause Day/Night Cycle", &m_timePaused))
	{
		if (m_onTimeSettings)
			m_onTimeSettings(m_timeOfDay, m_timePaused, m_useLiveTime);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("When paused, time will remain fixed at the current value");
	}

	ImGui::Spacing();

	// Use real-world time
	if (ImGui::Checkbox("Use Real-World Local Time", &m_useLiveTime))
	{
		if (m_useLiveTime)
		{
			// Disable pause when enabling live time
			m_timePaused = false;
		}
		if (m_onTimeSettings)
			m_onTimeSettings(m_timeOfDay, m_timePaused, m_useLiveTime);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Syncs the in-game time with your computer's local time");
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Note: Real-world time overrides pause and manual time settings");
}

void SettingsDialog::renderPersistenceSettings()
{
	ImGui::Text("World Persistence");
	ImGui::Spacing();

	// Load last world on startup toggle
	ImGui::Checkbox("Load Last World on Startup", &m_settings.loadLastWorldOnStartup);
	ImGui::TextDisabled("Automatically load the most recently played world when starting the game");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Auto-save toggle
	ImGui::Checkbox("Enable Auto-Save", &m_settings.enableAutoSave);
	ImGui::TextDisabled("Automatically save modified chunks every 5 minutes");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Show modified chunks count
	ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Modified Chunks: %d", m_modifiedChunksCount);

	if (m_modifiedChunksCount > 0)
	{
		ImGui::TextDisabled("These changes will be saved when you save the world");
	}
	else
	{
		ImGui::TextDisabled("No unsaved terrain changes");
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Save World button
	ImVec4 saveColor = m_modifiedChunksCount > 0 
		? ImVec4(0.3f, 0.7f, 0.3f, 1.0f)  // Green if there are changes
		: ImVec4(0.4f, 0.4f, 0.4f, 1.0f); // Gray if no changes

	ImGui::PushStyleColor(ImGuiCol_Button, saveColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(saveColor.x * 1.2f, saveColor.y * 1.2f, saveColor.z * 1.2f, saveColor.w));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(saveColor.x * 0.8f, saveColor.y * 0.8f, saveColor.z * 0.8f, saveColor.w));

	if (ImGui::Button("Save World", ImVec2(200.0f, 30.0f)))
	{
		if (m_onSaveWorld)
		{
			m_onSaveWorld();
		}
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Saves all terrain modifications to disk.\nThe world will automatically load these changes next time.");
	}

	ImGui::Spacing();
}

}

