#pragma once

#include <WorldCraft.h>

// Forward declare ChunkWorld for collision queries
namespace Chunk { class ChunkWorld; }

namespace Renderer
{

	// ---------------------------------------------------------------------------
	// FlyCamera — first-person free-fly camera driven by keyboard + mouse.
	//
	// Controls (all speeds are units/second):
	//   W / S        — move forward / backward along the look direction
	//   A / D        — strafe left / right
	//   Space        — move straight up
	//   Left Shift   — move straight down
	//   Mouse move   — pitch (up/down) and yaw (left/right)
	//   ESC          — releases cursor / signals app to quit
	// ---------------------------------------------------------------------------
	class FlyCamera
	{
	public:
		// Construct with an initial world-space position.
		explicit FlyCamera(glm::vec3 position = glm::vec3(8.0f, 3.0f, 8.0f));

		// Call once after the GL context and window are created.
		// Captures and hides the cursor.
		void init(SDL_Window* window);

		// Process keyboard and mouse input; dt is the frame delta in seconds.
		void update(SDL_Window* window, float dt);

		// Re-attach cursor capture state after a window mode change.
		void onWindowModeChanged(SDL_Window* window);

		// Detach cursor state before destroying the window.
		void onWindowAboutToBeDestroyed(SDL_Window* window);

		// Returns the view matrix (world → camera space).
		glm::mat4 viewMatrix() const;

		// Current world-space position.
		glm::vec3 position() const { return m_pos; }

		// Unit vector the camera is looking along.
		glm::vec3 forward() const;

		// Camera orientation for mode switching
		float yaw() const { return m_yaw; }
		float pitch() const { return m_pitch; }
		float getYaw() const { return m_yaw; }
		float getPitch() const { return m_pitch; }
		void setOrientation(float yaw, float pitch) { m_yaw = yaw; m_pitch = pitch; }
		void setYaw(float yaw) { m_yaw = yaw; }
		void setPitch(float pitch) { m_pitch = pitch; }
		void setPosition(glm::vec3 pos) { m_pos = pos; }

	private:
		glm::vec3 m_pos;
		float     m_yaw    = -90.0f; // degrees; -90 faces along -Z initially
		float     m_pitch  =   0.0f; // degrees; clamped to [-89, 89]

		float     m_speed     = 8.0f;   // units per second
		float     m_sensivity = 0.12f;  // degrees per pixel

		int       m_lastX = 0;
		int       m_lastY = 0;
		bool      m_firstMouse = true;
	};

	// ---------------------------------------------------------------------------
	// CharacterCamera — grounded walking/jumping camera with physics and collision.
	//
	// Controls:
	//   W / A / S / D — walk forward / left / backward / right (relative to look direction)
	//   Space         — jump (if on ground)
	//   Mouse move    — pitch (up/down) and yaw (left/right)
	//
	// Physics:
	//   - Gravity pulls character down
	//   - Collision with terrain (solid blocks)
	//   - Ground detection for jumping
	//   - Character dimensions: 0.6 blocks wide × 1.8 blocks tall
	//   - Eye height: 1.6 blocks above feet
	// ---------------------------------------------------------------------------
	class CharacterCamera
	{
	public:
		// Construct with an initial world-space position (feet position).
		explicit CharacterCamera(glm::vec3 position = glm::vec3(8.0f, 100.0f, 8.0f));

		// Call once after the GL context and window are created.
		void init(SDL_Window* window);

		// Process input and physics; dt is frame delta in seconds.
		// Requires ChunkWorld pointer for collision detection.
		void update(SDL_Window* window, float dt, const Chunk::ChunkWorld* world);

		// Re-attach cursor capture state after a window mode change.
		void onWindowModeChanged(SDL_Window* window);

		// Detach cursor state before destroying the window.
		void onWindowAboutToBeDestroyed(SDL_Window* window);

		// Returns the view matrix (world → camera space).
		glm::mat4 viewMatrix() const;

		// Current world-space position (EYE position, not feet).
		glm::vec3 position() const { return m_pos + glm::vec3(0.0f, m_eyeHeight, 0.0f); }

		// Feet position for collision
		glm::vec3 feetPosition() const { return m_pos; }

		// Unit vector the camera is looking along.
		glm::vec3 forward() const;

		// Camera orientation for mode switching
		float yaw() const { return m_yaw; }
		float pitch() const { return m_pitch; }
		float getYaw() const { return m_yaw; }
		float getPitch() const { return m_pitch; }
		void setOrientation(float yaw, float pitch) { m_yaw = yaw; m_pitch = pitch; }
		void setYaw(float yaw) { m_yaw = yaw; }
		void setPitch(float pitch) { m_pitch = pitch; }
		void setPosition(glm::vec3 eyePos) { m_pos = eyePos - glm::vec3(0.0f, m_eyeHeight, 0.0f); }

	private:
		glm::vec3 m_pos;          // Feet position (base of character)
		glm::vec3 m_velocity;     // Current velocity (m/s)
		float     m_yaw    = -90.0f;
		float     m_pitch  =   0.0f;

		// Physics constants
		static constexpr float m_gravity      = -20.0f;  // units/s²
		static constexpr float m_jumpVelocity =   8.0f;  // units/s (gives ~2 block jump)
		static constexpr float m_walkSpeed    =   4.3f;  // units/s
		static constexpr float m_airControl   =   0.8f;  // Reduced air control

		// Character dimensions (like Minecraft)
		static constexpr float m_width       = 0.6f;   // character width
		static constexpr float m_height      = 1.8f;   // character height
		static constexpr float m_eyeHeight   = 1.6f;   // eye height above feet

		float     m_sensivity = 0.12f;
		bool      m_onGround  = false;

		int       m_lastX = 0;
		int       m_lastY = 0;
		bool      m_firstMouse = true;

		// Collision helpers
		bool checkCollision(const glm::vec3& pos, const Chunk::ChunkWorld* world) const;
		bool isOnGround(const glm::vec3& pos, const Chunk::ChunkWorld* world) const;
	};

	// ---------------------------------------------------------------------------
	// ChaseCamera — Cinematic camera that flies to a target position
	//
	// Features:
	//   - Smoothly interpolates from current position to target
	//   - Hugs terrain contours, maintaining height above ground
	//   - Maintains minimum height above sea level
	//   - Automatically signals completion when destination reached
	//
	// Physics:
	//   - No user input during flight
	//   - Queries terrain height for ground-hugging behavior
	//   - Smooth velocity-based interpolation
	// ---------------------------------------------------------------------------
	class ChaseCamera
	{
	public:
		// Construct with initial position and target
		explicit ChaseCamera(glm::vec3 startPos = glm::vec3(0.0f, 100.0f, 0.0f));

		// Initialize camera
		void init(SDL_Window* window);

		// Set the target position to fly towards
		void setTarget(glm::vec3 target) { m_target = target; m_hasTarget = true; m_currentSpeed = 0.0f; }

		// Update camera position, returns true if still moving, false if reached target
		bool update(SDL_Window* window, float dt, const Chunk::ChunkWorld* world, int seaLevel);

		// Re-attach cursor capture state after a window mode change
		void onWindowModeChanged(SDL_Window* window);

		// Detach cursor state before destroying the window
		void onWindowAboutToBeDestroyed(SDL_Window* window);

		// Returns the view matrix (world → camera space)
		glm::mat4 viewMatrix() const;

		// Current world-space position
		glm::vec3 position() const { return m_pos; }

		// Unit vector the camera is looking along (towards target)
		glm::vec3 forward() const;

		// Camera orientation
		float yaw() const { return m_yaw; }
		float pitch() const { return m_pitch; }
		void setPosition(glm::vec3 pos) { m_pos = pos; }

		// Check if camera has reached target
		bool hasReachedTarget() const { return !m_hasTarget; }

		// Get chase camera info for HUD
		glm::vec3 getTarget() const { return m_target; }
		float getCurrentSpeed() const { return m_currentSpeed; }
		float getDistanceToTarget() const 
		{ 
			if (!m_hasTarget) return 0.0f;
			return glm::length(m_target - m_pos); 
		}

	private:
		glm::vec3 m_pos;
		glm::vec3 m_target;
		bool      m_hasTarget = false;

		float     m_yaw = -90.0f;
		float     m_pitch = 0.0f;
		float     m_currentSpeed = 0.0f;  // Current speed for acceleration/deceleration

		// Chase camera constants
		static constexpr float m_maxSpeed = 80.0f;       // max speed (2x boost speed: 8.0f * 5.0f * 2.0f)
		static constexpr float m_acceleration = 15.0f;   // acceleration rate (units/s²)
		static constexpr float m_terrainHeight = 15.0f;  // height above terrain/sea level (increased for safety)
		static constexpr float m_arrivalDist = 1.0f;     // distance to consider "arrived" (reduced for closer approach)
		static constexpr float m_lookAhead = 15.0f;      // how far ahead to look for terrain (increased)
		static constexpr float m_decelDist = 40.0f;      // distance at which to start decelerating (increased for smoother stop)
	};

} // namespace Renderer
