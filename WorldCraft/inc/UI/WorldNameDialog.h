#pragma once

#include <imgui.h>
#include <string>
#include <functional>

namespace UI
{
	// Simple dialog for entering a new world name
	class WorldNameDialog
	{
	public:
		using OnConfirmCallback = std::function<void(const std::string& worldName)>;

		WorldNameDialog();

		// Render the dialog
		// Returns true if dialog is open, false if closed/confirmed
		bool render();

		// Show/hide the dialog
		void show(const std::string& suggestedName = "");
		void hide() { m_isOpen = false; }
		bool isOpen() const { return m_isOpen; }

		// Set callback for when user confirms name
		void setConfirmCallback(OnConfirmCallback callback) { m_onConfirm = callback; }

		// Set list of existing world names (for validation)
		void setExistingWorldNames(const std::vector<std::string>& names) { m_existingNames = names; }

	private:
		bool validateWorldName(const std::string& name);

		bool m_isOpen;
		char m_worldName[64];
		std::string m_errorMessage;
		std::vector<std::string> m_existingNames;
		OnConfirmCallback m_onConfirm;
	};
}
