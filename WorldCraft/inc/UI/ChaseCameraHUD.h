#pragma once

#include <glm/glm.hpp>

namespace UI
{
	// HUD overlay shown during chase camera flight
	class ChaseCameraHUD
	{
	public:
		ChaseCameraHUD() = default;

		// Render the chase camera info overlay
		void render(const glm::vec3& currentPos, 
					const glm::vec3& targetPos, 
					float currentSpeed,
					float distanceRemaining);

		// Show/hide the HUD
		void show() { m_isVisible = true; }
		void hide() { m_isVisible = false; }
		bool isVisible() const { return m_isVisible; }

	private:
		bool m_isVisible = false;
	};
}
