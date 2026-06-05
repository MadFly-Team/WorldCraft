#include <World/ItemEntity.h>
#include <cmath>

namespace World
{

ItemEntity::ItemEntity(const glm::vec3& position, Voxel::BlockID blockType, int stackCount)
	: m_basePosition(position)
	, m_blockType(blockType)
	, m_stackCount(stackCount)
	, m_lifetime(0.0f)
	, m_floatTime(0.0f)
	, m_rotation(0.0f)
	, m_isBeingCollected(false)
	, m_shouldRemove(false)
{
	// Initial spawn velocity: upward with small random horizontal offset
	float randomX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f;
	float randomZ = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f;
	m_velocity = glm::vec3(randomX, SPAWN_VELOCITY_UP, randomZ);
}

bool ItemEntity::update(float deltaTime, const glm::vec3& playerPos, float pickupRadius, CollisionCheckFunc collisionCheck)
{
	// Update lifetime
	m_lifetime += deltaTime;
	if (m_lifetime >= LIFETIME_MAX)
	{
		m_shouldRemove = true;
		return false;
	}

	// Update floating animation time
	m_floatTime += deltaTime;

	// Update rotation
	m_rotation += ROTATION_SPEED * deltaTime;
	if (m_rotation >= 360.0f)
		m_rotation -= 360.0f;

	// Check distance to player for magnetic collection
	float distanceToPlayer = glm::distance(m_basePosition, playerPos);

	if (distanceToPlayer <= pickupRadius && !m_isBeingCollected)
	{
		m_isBeingCollected = true;
	}

	if (m_isBeingCollected)
	{
		// Magnetic attraction: move toward player
		glm::vec3 directionToPlayer = glm::normalize(playerPos - m_basePosition);
		m_velocity = directionToPlayer * COLLECTION_SPEED;

		// Check if close enough to collect (within 0.5 blocks)
		if (distanceToPlayer < 0.5f)
		{
			m_shouldRemove = true;
			return false;
		}
	}
	else
	{
		// Apply gravity when not being collected
		m_velocity.y += GRAVITY * deltaTime;

		// Terrain collision detection
		if (collisionCheck)
		{
			// Calculate next position
			glm::vec3 nextPos = m_basePosition + m_velocity * deltaTime;

			// Check Y collision (vertical - most important to prevent falling through)
			if (m_velocity.y < 0.0f)  // Only check when falling
			{
				// Check block below the item
				// Items are small (0.25 scale), check a bit below center
				float checkY = nextPos.y - 0.3f;

				if (collisionCheck(nextPos.x, checkY, nextPos.z))
				{
					// Hit ground - snap to just above the block surface
					float groundY = std::floor(nextPos.y) + 0.3f;  // Rest slightly above block
					m_basePosition.y = groundY;

					// Bounce with energy loss
					if (std::abs(m_velocity.y) > 0.5f)
					{
						m_velocity.y = -m_velocity.y * 0.3f;  // Bounce to 30% of fall speed
					}
					else
					{
						// Too slow to bounce, just stop
						m_velocity.y = 0.0f;
					}

					// Friction
					m_velocity.x *= 0.7f;
					m_velocity.z *= 0.7f;

					// Don't apply the rest of velocity if we hit ground
					return true;
				}
			}

			// Check horizontal collisions (X and Z)
			// Check if moving into a solid block
			bool xBlocked = (m_velocity.x != 0.0f) && 
							collisionCheck(nextPos.x, m_basePosition.y, m_basePosition.z);
			bool zBlocked = (m_velocity.z != 0.0f) && 
							collisionCheck(m_basePosition.x, m_basePosition.y, nextPos.z);

			if (xBlocked)
			{
				m_velocity.x = 0.0f;
				nextPos.x = m_basePosition.x;
			}
			if (zBlocked)
			{
				m_velocity.z = 0.0f;
				nextPos.z = m_basePosition.z;
			}

			// Update position with collision-checked velocity
			m_basePosition = nextPos;
		}
		else
		{
			// Fallback: simple ground collision at y=1 if no collision check provided
			if (m_basePosition.y <= 1.0f && m_velocity.y < 0.0f)
			{
				m_basePosition.y = 1.0f;
				m_velocity.y = 0.0f;
				m_velocity.x *= 0.8f;
				m_velocity.z *= 0.8f;
			}

			// Update base position with velocity
			m_basePosition += m_velocity * deltaTime;
		}
	}

	// When being collected, always update position (magnetic pull ignores physics)
	if (m_isBeingCollected)
	{
		m_basePosition += m_velocity * deltaTime;
	}

	return true;
}

glm::vec3 ItemEntity::getPosition() const
{
	// Add sine-wave floating animation to base position
	float floatOffset = std::sin(m_floatTime * FLOAT_SPEED) * FLOAT_AMPLITUDE;
	return m_basePosition + glm::vec3(0.0f, floatOffset, 0.0f);
}

} // namespace World
