#include <UI/SettingsDialog.h>
#include <imgui.h>
#include <cstring>
#include <ctime>

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
		m_settings.seed = static_cast<int>(time(nullptr));
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

}
