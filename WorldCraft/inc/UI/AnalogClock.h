#pragma once

namespace UI
{
	// Analog clock display showing in-game time
	class AnalogClock
	{
	public:
		AnalogClock() = default;

		// Render the analog clock
		// timeOfDay: 0.0 = midnight, 0.25 = 6am, 0.5 = noon, 0.75 = 6pm
		void render(float timeOfDay, int screenWidth, int screenHeight);

		// Show/hide the clock
		void show() { m_isVisible = true; }
		void hide() { m_isVisible = false; }
		bool isVisible() const { return m_isVisible; }

	private:
		bool m_isVisible = true;
	};
}
