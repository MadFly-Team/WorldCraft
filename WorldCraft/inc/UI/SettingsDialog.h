#pragma once

#include <WorldGen/WorldSettings.h>
#include <functional>

namespace UI
{
	// Settings dialog for configuring world generation parameters
	class SettingsDialog
	{
	public:
		using OnGenerateWorldCallback = std::function<void(const WorldGen::WorldSettings&)>;

		SettingsDialog();

		// Render the settings UI
		// Returns true if the dialog is open, false if closed
		bool render();

		// Show/hide the settings dialog
		void show() { m_isOpen = true; }
		void hide() { m_isOpen = false; }
		void toggle() { m_isOpen = !m_isOpen; }
		bool isOpen() const { return m_isOpen; }

		// Set callback for when user clicks "Generate World"
		void setGenerateWorldCallback(OnGenerateWorldCallback callback)
		{
			m_onGenerateWorld = callback;
		}

		// Get current settings
		const WorldGen::WorldSettings& getSettings() const { return m_settings; }

		// Set settings (e.g., to restore from saved state)
		void setSettings(const WorldGen::WorldSettings& settings) { m_settings = settings; }

	private:
		void renderPresets();
		void renderBasicSettings();
		void renderTerrainSettings();
		void renderBiomeSettings();
		void renderCaveSettings();
		void renderOreSettings();
		void renderTreeSettings();
		void renderGenerateButton();

		WorldGen::WorldSettings m_settings;
		OnGenerateWorldCallback m_onGenerateWorld;
		bool m_isOpen = true;  // Start open by default
		int m_selectedPreset = 0;  // 0 = Default, 1 = Flat, 2 = Mountainous, etc.
	};
}
