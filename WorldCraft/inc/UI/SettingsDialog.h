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
		using OnGoToPositionCallback = std::function<void(float x, float y, float z)>;
		using OnTimeSettingsCallback = std::function<void(float timeOfDay, bool isPaused, bool useLiveTime)>;
		using OnSaveWorldCallback = std::function<void()>;

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

		// Set callback for when user clicks "Go To Position"
		void setGoToPositionCallback(OnGoToPositionCallback callback)
		{
			m_onGoToPosition = callback;
		}

		// Set callback for time settings changes
		void setTimeSettingsCallback(OnTimeSettingsCallback callback)
		{
			m_onTimeSettings = callback;
		}

		// Set callback for save world
		void setSaveWorldCallback(OnSaveWorldCallback callback)
		{
			m_onSaveWorld = callback;
		}

		// Set current time (for external updates)
		void setCurrentTime(float timeOfDay, bool isPaused, bool useLiveTime)
		{
			m_timeOfDay = timeOfDay;
			m_timePaused = isPaused;
			m_useLiveTime = useLiveTime;
		}

		// Set modified chunks count (for UI display)
		void setModifiedChunksCount(int count)
		{
			m_modifiedChunksCount = count;
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
		void renderWaterSettings();
		void renderCameraSettings();
		void renderTimeSettings();
		void renderPersistenceSettings();
		void renderGenerateButton();

		WorldGen::WorldSettings m_settings;
		OnGenerateWorldCallback m_onGenerateWorld;
		OnGoToPositionCallback m_onGoToPosition;
		OnTimeSettingsCallback m_onTimeSettings;
		OnSaveWorldCallback m_onSaveWorld;
		bool m_isOpen = false;  // Start closed by default
		int m_selectedPreset = 0;  // 0 = Default, 1 = Flat, 2 = Mountainous, etc.
		int m_modifiedChunksCount = 0;

		// Time settings state
		float m_timeOfDay = 0.25f;  // Start at sunrise
		bool m_timePaused = false;
		bool m_useLiveTime = false;
	};
}
