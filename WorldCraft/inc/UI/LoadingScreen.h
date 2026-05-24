#pragma once

namespace UI
{
	// Simple loading screen that shows chunk loading progress
	class LoadingScreen
	{
	public:
		LoadingScreen() = default;

		// Render the loading screen with progress (0.0 to 1.0)
		// Returns true if should continue showing, false if dismissed
		void render(float progress, int loadedChunks, int targetChunks);

		// Check if loading screen is active
		bool isActive() const { return m_isActive; }
		void setActive(bool active) { m_isActive = active; }

	private:
		bool m_isActive = true;
	};
}
