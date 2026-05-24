#include <Renderer/Camera.h>
#include <Chunk/ChunkWorld.h>
#include <algorithm>

namespace Renderer
{

// ---------------------------------------------------------------------------
// SDL2 uses a polling model instead of callbacks for input.
// Mouse motion is handled directly in update() via SDL_GetRelativeMouseState.
// ---------------------------------------------------------------------------

FlyCamera::FlyCamera(glm::vec3 position)
	: m_pos(position)
{}

void FlyCamera::init(SDL_Window* window)
{
	if (!window) return;

	// Capture and hide the mouse cursor
	SDL_SetRelativeMouseMode(SDL_TRUE);

	// Initialize mouse position
	SDL_GetMouseState(&m_lastX, &m_lastY);
	m_firstMouse = true;
}

void FlyCamera::onWindowAboutToBeDestroyed(SDL_Window* window)
{
	if (!window) return;

	// Release mouse capture
	SDL_SetRelativeMouseMode(SDL_FALSE);
	m_firstMouse = true;
}

void FlyCamera::update(SDL_Window* window, float dt)
{
	if (!window) return;

	// Get keyboard state
	const Uint8* keyState = SDL_GetKeyboardState(nullptr);

	// Build the current forward and right vectors from yaw/pitch.
	glm::vec3 fwd = forward();
	glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0.0f, 1.0f, 0.0f)));

	// Left Ctrl held = 5× speed boost.
	const float speed = (keyState[SDL_SCANCODE_LCTRL])
						? m_speed * 5.0f : m_speed;

	// Keyboard movement — all directions relative to look direction.
	if (keyState[SDL_SCANCODE_W])      m_pos += fwd   * speed * dt;
	if (keyState[SDL_SCANCODE_S])      m_pos -= fwd   * speed * dt;
	if (keyState[SDL_SCANCODE_A])      m_pos -= right * speed * dt;
	if (keyState[SDL_SCANCODE_D])      m_pos += right * speed * dt;
	if (keyState[SDL_SCANCODE_SPACE])  m_pos.y += speed * dt;
	if (keyState[SDL_SCANCODE_LSHIFT]) m_pos.y -= speed * dt;

	// Mouse look - get relative mouse motion
	int mouseX, mouseY;
	SDL_GetRelativeMouseState(&mouseX, &mouseY);

	if (m_firstMouse)
	{
		m_firstMouse = false;
		// Ignore first frame delta to avoid jump
		return;
	}

	const float dx = static_cast<float>(mouseX) * m_sensivity;
	const float dy = static_cast<float>(-mouseY) * m_sensivity; // inverted Y

	m_yaw   += dx;
	m_pitch  = std::clamp(m_pitch + dy, -89.0f, 89.0f);
}

void FlyCamera::onWindowModeChanged(SDL_Window* window)
{
	if (!window) return;

	// Re-enable mouse capture after window mode change
	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_GetMouseState(&m_lastX, &m_lastY);
	m_firstMouse = true;
}

glm::vec3 FlyCamera::forward() const
{
	// Convert yaw/pitch (degrees) to a unit direction vector.
	const float yawR   = glm::radians(m_yaw);
	const float pitchR = glm::radians(m_pitch);

	return glm::normalize(glm::vec3(
		std::cos(yawR) * std::cos(pitchR),
		std::sin(pitchR),
		std::sin(yawR) * std::cos(pitchR)
	));
}

glm::mat4 FlyCamera::viewMatrix() const
{
	return glm::lookAt(m_pos, m_pos + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

// ===========================================================================
// CharacterCamera — Grounded walking/jumping camera with physics
// ===========================================================================

CharacterCamera::CharacterCamera(glm::vec3 position)
	: m_pos(position)
	, m_velocity(0.0f)
{}

void CharacterCamera::init(SDL_Window* window)
{
	if (!window) return;
	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_GetMouseState(&m_lastX, &m_lastY);
	m_firstMouse = true;
}

void CharacterCamera::onWindowAboutToBeDestroyed(SDL_Window* window)
{
	if (!window) return;
	SDL_SetRelativeMouseMode(SDL_FALSE);
	m_firstMouse = true;
}

void CharacterCamera::update(SDL_Window* window, float dt, const Chunk::ChunkWorld* world)
{
	if (!window || !world) return;

	// Get keyboard state
	const Uint8* keyState = SDL_GetKeyboardState(nullptr);

	// Build forward and right vectors (horizontal plane only for walking)
	glm::vec3 fwd = forward();
	glm::vec3 fwdFlat = glm::normalize(glm::vec3(fwd.x, 0.0f, fwd.z));
	glm::vec3 right = glm::normalize(glm::cross(fwdFlat, glm::vec3(0.0f, 1.0f, 0.0f)));

	// Check if character is on ground
	m_onGround = isOnGround(m_pos, world);

	// Horizontal movement input
	glm::vec3 inputDir(0.0f);
	if (keyState[SDL_SCANCODE_W]) inputDir += fwdFlat;
	if (keyState[SDL_SCANCODE_S]) inputDir -= fwdFlat;
	if (keyState[SDL_SCANCODE_A]) inputDir -= right;
	if (keyState[SDL_SCANCODE_D]) inputDir += right;

	// Normalize input if moving diagonally
	if (glm::length(inputDir) > 0.0f)
		inputDir = glm::normalize(inputDir);

	// Apply movement (reduced control in air)
	float moveSpeed = m_walkSpeed;
	if (!m_onGround)
		moveSpeed *= m_airControl;

	glm::vec3 targetVelXZ = inputDir * moveSpeed;
	m_velocity.x = targetVelXZ.x;
	m_velocity.z = targetVelXZ.z;

	// Jumping (only when on ground)
	if (m_onGround && keyState[SDL_SCANCODE_SPACE])
	{
		m_velocity.y = m_jumpVelocity;
		m_onGround = false;  // Leave ground immediately
	}

	// Apply gravity
	if (!m_onGround)
	{
		m_velocity.y += m_gravity * dt;
		// Terminal velocity cap
		if (m_velocity.y < -50.0f)
			m_velocity.y = -50.0f;
	}
	else
	{
		// On ground, reset vertical velocity
		m_velocity.y = 0.0f;
	}

	// Move with collision detection
	glm::vec3 newPos = m_pos + m_velocity * dt;

	// Horizontal collision (X axis)
	glm::vec3 testPos = m_pos;
	testPos.x = newPos.x;
	if (!checkCollision(testPos, world))
		m_pos.x = newPos.x;
	else
		m_velocity.x = 0.0f;  // Stop horizontal movement on collision

	// Horizontal collision (Z axis)
	testPos = m_pos;
	testPos.z = newPos.z;
	if (!checkCollision(testPos, world))
		m_pos.z = newPos.z;
	else
		m_velocity.z = 0.0f;

	// Vertical collision
	testPos = m_pos;
	testPos.y = newPos.y;
	if (!checkCollision(testPos, world))
	{
		m_pos.y = newPos.y;
	}
	else
	{
		// Hit ceiling or floor
		if (m_velocity.y < 0.0f)
		{
			// Landing on ground
			m_onGround = true;
			m_velocity.y = 0.0f;
		}
		else if (m_velocity.y > 0.0f)
		{
			// Hit ceiling
			m_velocity.y = 0.0f;
		}
	}

	// Mouse look (same as FlyCamera)
	int mouseX, mouseY;
	SDL_GetRelativeMouseState(&mouseX, &mouseY);

	if (m_firstMouse)
	{
		m_firstMouse = false;
		return;
	}

	const float dx = static_cast<float>(mouseX) * m_sensivity;
	const float dy = static_cast<float>(-mouseY) * m_sensivity;

	m_yaw   += dx;
	m_pitch  = std::clamp(m_pitch + dy, -89.0f, 89.0f);
}

void CharacterCamera::onWindowModeChanged(SDL_Window* window)
{
	if (!window) return;
	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_GetMouseState(&m_lastX, &m_lastY);
	m_firstMouse = true;
}

glm::vec3 CharacterCamera::forward() const
{
	const float yawR   = glm::radians(m_yaw);
	const float pitchR = glm::radians(m_pitch);

	return glm::normalize(glm::vec3(
		std::cos(yawR) * std::cos(pitchR),
		std::sin(pitchR),
		std::sin(yawR) * std::cos(pitchR)
	));
}

glm::mat4 CharacterCamera::viewMatrix() const
{
	// View from eye position (feet + eye height)
	glm::vec3 eyePos = m_pos + glm::vec3(0.0f, m_eyeHeight, 0.0f);
	return glm::lookAt(eyePos, eyePos + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

bool CharacterCamera::checkCollision(const glm::vec3& pos, const Chunk::ChunkWorld* world) const
{
	// Character is a box: width × height centered on pos (feet)
	const float halfWidth = m_width * 0.5f;

	// Check 8 corners of the character bounding box
	// Bottom corners (feet level)
	glm::vec3 corners[8] = {
		pos + glm::vec3(-halfWidth, 0.0f, -halfWidth),
		pos + glm::vec3( halfWidth, 0.0f, -halfWidth),
		pos + glm::vec3(-halfWidth, 0.0f,  halfWidth),
		pos + glm::vec3( halfWidth, 0.0f,  halfWidth),
		// Top corners (head level)
		pos + glm::vec3(-halfWidth, m_height, -halfWidth),
		pos + glm::vec3( halfWidth, m_height, -halfWidth),
		pos + glm::vec3(-halfWidth, m_height,  halfWidth),
		pos + glm::vec3( halfWidth, m_height,  halfWidth)
	};

	// Check if any corner is inside a solid block
	for (const auto& corner : corners)
	{
		if (world->isBlockSolid(corner.x, corner.y, corner.z))
			return true;  // Collision detected
	}

	// Also check middle of character (prevents tunneling through thin walls)
	glm::vec3 center = pos + glm::vec3(0.0f, m_height * 0.5f, 0.0f);
	if (world->isBlockSolid(center.x, center.y, center.z))
		return true;

	return false;  // No collision
}

bool CharacterCamera::isOnGround(const glm::vec3& pos, const Chunk::ChunkWorld* world) const
{
	// Check slightly below feet to detect ground
	const float checkDist = 0.1f;
	const float halfWidth = m_width * 0.5f;

	// Check four corners at foot level, slightly below
	glm::vec3 checkPoints[4] = {
		pos + glm::vec3(-halfWidth, -checkDist, -halfWidth),
		pos + glm::vec3( halfWidth, -checkDist, -halfWidth),
		pos + glm::vec3(-halfWidth, -checkDist,  halfWidth),
		pos + glm::vec3( halfWidth, -checkDist,  halfWidth)
	};

	for (const auto& point : checkPoints)
	{
		if (world->isBlockSolid(point.x, point.y, point.z))
			return true;  // Ground detected
	}

	return false;
}

// ===========================================================================
// ChaseCamera — Cinematic camera that flies to target position
// ===========================================================================

ChaseCamera::ChaseCamera(glm::vec3 startPos)
	: m_pos(startPos)
	, m_target(startPos)
{}

void ChaseCamera::init(SDL_Window* window)
{
	if (!window) return;
	// Chase camera doesn't need mouse capture (cinematic mode)
}

void ChaseCamera::onWindowAboutToBeDestroyed(SDL_Window* window)
{
	// No special cleanup needed
}

bool ChaseCamera::update(SDL_Window* window, float dt, const Chunk::ChunkWorld* world, int seaLevel)
{
	if (!window || !world || !m_hasTarget)
		return false;

	// Calculate direction to target
	glm::vec3 toTarget = m_target - m_pos;
	float distToTarget = glm::length(toTarget);

	// Check if we've arrived - use a more generous check for arrival
	// Allow arrival if we're within 5 blocks horizontally and 5 blocks vertically
	float horizontalDist = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
	float verticalDist = std::abs(toTarget.y);

	if (horizontalDist < 3.0f && verticalDist < 5.0f)
	{
		// Very close - snap to target and stop
		m_pos = m_target;
		m_hasTarget = false;
		m_currentSpeed = 0.0f;
		return false;  // Reached destination
	}

	// Normalize direction
	glm::vec3 direction = toTarget / distToTarget;

	// Helper function to get terrain height at a position
	auto getTerrainHeight = [&](float x, float z) -> float {
		const float maxHeight = 200.0f;
		for (float y = maxHeight; y >= 0.0f; y -= 1.0f)
		{
			if (world->isBlockSolid(x, y, z))
				return y + 1.0f;  // One block above the solid surface
		}
		return 0.0f;
	};

	// CRITICAL: Check if camera is currently inside terrain - immediate escape needed
	bool insideTerrain = world->isBlockSolid(m_pos.x, m_pos.y, m_pos.z);
	if (insideTerrain)
	{
		// Emergency: We're stuck in terrain, rise VERY quickly
		m_pos.y += m_maxSpeed * 2.0f * dt;  // Double max speed upward
		// Don't move horizontally when escaping
		return true;
	}

	// Helper function to check if there's a collision along a ray
	auto checkCollisionAhead = [&](const glm::vec3& start, const glm::vec3& dir, float distance) -> bool {
		// Sample several points along the path
		const int samples = 8;  // Increased samples for better detection
		for (int i = 1; i <= samples; ++i)
		{
			float t = (distance * i) / samples;
			glm::vec3 testPos = start + dir * t;
			// Check if this position would be inside terrain
			if (world->isBlockSolid(testPos.x, testPos.y, testPos.z))
				return true;
		}
		return false;
	};

	// Query terrain height at look-ahead position
	glm::vec3 lookAheadPos = m_pos + direction * m_lookAhead;
	float terrainHeightAhead = getTerrainHeight(lookAheadPos.x, lookAheadPos.z);

	// Also check terrain at current position
	float terrainHeightCurrent = getTerrainHeight(m_pos.x, m_pos.z);

	// Calculate desired height above terrain - use the higher of current or ahead
	float maxTerrainHeight = std::max(terrainHeightCurrent, terrainHeightAhead);
	float minHeight = static_cast<float>(seaLevel) + m_terrainHeight;
	float targetHeight = std::max(maxTerrainHeight + m_terrainHeight, minHeight);

	// When very close to destination (< 5 blocks), allow descending to target height
	// even if it's below the normal terrain clearance
	if (horizontalDist < 5.0f)
	{
		targetHeight = std::min(targetHeight, m_target.y);
	}

	// Check for collision along the movement path
	bool collisionDetected = checkCollisionAhead(m_pos, direction, m_lookAhead);

	// Check if we're dangerously close to terrain below
	bool tooCloseToGround = (m_pos.y - terrainHeightCurrent) < (m_terrainHeight * 0.5f);

	// Calculate target speed based on distance and collision
	float targetSpeed;
	if (collisionDetected || tooCloseToGround)
	{
		// Slow down significantly if collision detected or too close to ground
		targetSpeed = m_maxSpeed * 0.15f;  // Reduce to 15% speed
	}
	else if (distToTarget > m_decelDist)
	{
		// Accelerate to max speed when clear path
		targetSpeed = m_maxSpeed;
	}
	else
	{
		// Improved deceleration - quadratic falloff for smoother stop
		float distRatio = distToTarget / m_decelDist;
		targetSpeed = m_maxSpeed * distRatio * distRatio;  // Quadratic slowdown

		// More aggressive minimum speed reduction near the end
		if (distToTarget < 10.0f)
		{
			// Final approach - very slow and precise
			targetSpeed = m_maxSpeed * 0.05f * distRatio;  // 5% or less of max speed
		}

		// Very close to destination - allow speed to drop to zero for smooth stop
		if (distToTarget < 2.0f)
		{
			// Scale linearly to zero in the final approach
			targetSpeed = m_maxSpeed * 0.05f * (distToTarget / 2.0f);
		}
	}

	// If we're below target height, prioritize rising over horizontal movement
	float heightDeficit = targetHeight - m_pos.y;
	if (heightDeficit > 5.0f)  // More than 5 blocks below target
	{
		// Significantly reduce horizontal speed when we need to climb
		targetSpeed = std::min(targetSpeed, m_maxSpeed * 0.1f);
	}

	// Smoothly adjust current speed towards target speed
	if (m_currentSpeed < targetSpeed)
	{
		m_currentSpeed += m_acceleration * dt;
		if (m_currentSpeed > targetSpeed)
			m_currentSpeed = targetSpeed;
	}
	else if (m_currentSpeed > targetSpeed)
	{
		m_currentSpeed -= m_acceleration * dt;
		if (m_currentSpeed < targetSpeed)
			m_currentSpeed = targetSpeed;
	}

	// Calculate horizontal movement
	float moveAmount = m_currentSpeed * dt;
	if (moveAmount > distToTarget)
		moveAmount = distToTarget;

	glm::vec3 newPosFlat = m_pos + direction * moveAmount;

	// Check if the new horizontal position would cause collision
	bool horizontalBlocked = false;
	if (world->isBlockSolid(newPosFlat.x, m_pos.y, newPosFlat.z))
	{
		horizontalBlocked = true;
		newPosFlat = m_pos;  // Stay in place horizontally
	}

	// Calculate vertical movement - VERY aggressive when any threat detected
	float verticalSpeed;

	// When very close to destination, allow faster descent
	bool nearDestination = (horizontalDist < 5.0f);

	if (collisionDetected || horizontalBlocked || tooCloseToGround || heightDeficit > 0.0f)
	{
		// AGGRESSIVE rise when any terrain threat or below target height
		verticalSpeed = m_maxSpeed * 1.5f;  // 1.5x max speed upward
	}
	else if (nearDestination && heightDeficit < 0.0f)
	{
		// Near destination and above target - descend faster to reach target height
		verticalSpeed = m_maxSpeed;  // Full speed descent when safe and near goal
	}
	else
	{
		// Normal descent when safely above terrain
		verticalSpeed = m_currentSpeed * 0.5f;
	}

	float verticalMove = heightDeficit * dt * 5.0f;  // Increased interpolation factor

	// Clamp vertical movement speed
	if (std::abs(verticalMove) > verticalSpeed * dt)
		verticalMove = (verticalMove > 0.0f ? 1.0f : -1.0f) * verticalSpeed * dt;

	// Apply movement
	glm::vec3 candidatePos = glm::vec3(newPosFlat.x, m_pos.y + verticalMove, newPosFlat.z);

	// Final collision check - don't move if it would place us inside a block
	if (!world->isBlockSolid(candidatePos.x, candidatePos.y, candidatePos.z))
	{
		m_pos = candidatePos;
	}
	else
	{
		// If we'd collide, ONLY move up and move up FAST
		glm::vec3 escapeUp = glm::vec3(m_pos.x, m_pos.y + m_maxSpeed * 1.5f * dt, m_pos.z);
		if (!world->isBlockSolid(escapeUp.x, escapeUp.y, escapeUp.z))
		{
			m_pos = escapeUp;
		}
		else
		{
			// Still blocked? Try straight up at double speed
			m_pos.y += m_maxSpeed * 2.0f * dt;
		}
	}

	// Update camera orientation - keep horizontal view (pitch = 0)
	glm::vec3 lookDir = glm::normalize(toTarget);

	// Calculate yaw (horizontal rotation only)
	m_yaw = glm::degrees(std::atan2(lookDir.z, lookDir.x));

	// Keep pitch at 0 for horizontal view
	m_pitch = 0.0f;

	return true;  // Still moving
}

void ChaseCamera::onWindowModeChanged(SDL_Window* window)
{
	// No special handling needed for chase camera
}

glm::vec3 ChaseCamera::forward() const
{
	const float yawR = glm::radians(m_yaw);
	const float pitchR = glm::radians(m_pitch);

	return glm::normalize(glm::vec3(
		std::cos(yawR) * std::cos(pitchR),
		std::sin(pitchR),
		std::sin(yawR) * std::cos(pitchR)
	));
}

glm::mat4 ChaseCamera::viewMatrix() const
{
	return glm::lookAt(m_pos, m_pos + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace Renderer
