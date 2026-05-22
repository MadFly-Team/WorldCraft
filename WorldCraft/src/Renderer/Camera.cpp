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

} // namespace Renderer
