#pragma once

#include <Persistence/WorldMetadata.h>
#include <Persistence/WorldPersistence.h>
#include <imgui.h>
#include <functional>
#include <vector>
#include <string>

namespace UI
{
	// Dialog for selecting and managing saved worlds
	class WorldSelectionDialog
	{
	public:
		using OnLoadWorldCallback = std::function<void(const std::string& worldName)>;

		WorldSelectionDialog();

		// Render the dialog
		// Returns true if dialog is open, false if closed
		bool render(Persistence::WorldPersistence* persistence);

		// Show/hide the dialog
		void show() { m_isOpen = true; refreshWorldList(); }
		void hide() { m_isOpen = false; }
		bool isOpen() const { return m_isOpen; }

		// Set callback for when a world is selected to load
		void setLoadWorldCallback(OnLoadWorldCallback callback) { m_onLoadWorld = callback; }

		// Refresh the list of available worlds
		void refreshWorldList();

		// Set the persistence system (needed to query worlds)
		void setPersistence(Persistence::WorldPersistence* persistence) { m_persistence = persistence; }

	private:
		void renderWorldList();
		void renderWorldDetails(const Persistence::WorldMetadata& world);
		void renderActionButtons();

		std::string formatTimestamp(time_t timestamp) const;
		std::string formatFileSize(uint64_t bytes) const;

		bool m_isOpen;
		Persistence::WorldPersistence* m_persistence;
		std::vector<Persistence::WorldMetadata> m_worlds;
		int m_selectedIndex;
		OnLoadWorldCallback m_onLoadWorld;

		// Confirmation dialog state
		bool m_showDeleteConfirmation;
		std::string m_worldToDelete;
	};
}
