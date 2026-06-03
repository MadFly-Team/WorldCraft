#include <World/FallingBlock.h>
#include <World/BlockMassRegistry.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace WorldPhysics {

FallingBlock::FallingBlock(Voxel::BlockID blockType, const glm::vec3& position)
	: m_blockType(blockType)
	, m_position(position)
	, m_velocity(0.0f)
	, m_direction(0.0f, -1.0f, 0.0f) // Initially falling down
	, m_baseMass(BlockMassRegistry::getMass(blockType))
	, m_currentMass(m_baseMass * (1.0f + INITIAL_MASS_BOOST)) // 50% initial boost
	, m_energy(0.0f)
	, m_blocksFallen(0)
	, m_deflectionCount(0)
	, m_lifetime(0.0f)
	, m_underwater(false)
	, m_hasStartedFalling(false)
	, m_isSettled(false)
	, m_failedSettlementAttempts(0)
{
}

bool FallingBlock::update(float deltaTime)
{
	// If block has been forcibly settled, skip all physics
	if (m_isSettled)
	{
		return true; // Keep it alive until physics system places it
	}

	// Safety: Remove blocks that have been active too long or fallen too far
	m_lifetime += deltaTime;
	if (m_lifetime > MAX_LIFETIME)
	{
		std::cout << "[Physics] Block removed after " << MAX_LIFETIME << "s timeout" << std::endl;
		return false; // Remove this block
	}

	if (m_blocksFallen > MAX_FALL_DISTANCE)
	{
		std::cout << "[Physics] Block removed after falling " << MAX_FALL_DISTANCE << " blocks" << std::endl;
		return false; // Remove this block
	}

	// Check if we've bounced too many times
	if (m_deflectionCount >= MAX_DEFLECTIONS)
	{
		std::cout << "[Physics] Block removed after " << MAX_DEFLECTIONS << " deflections" << std::endl;
		return false; // Remove this block
	}

	// Apply gravity
	m_velocity.y -= GRAVITY * deltaTime;

	// Apply water drag if underwater (slow down significantly)
	if (m_underwater)
	{
		// Apply strong drag force in water (0.92 = 8% speed loss per frame at 60fps)
		m_velocity *= 0.92f;
	}

	// Update position
	glm::vec3 oldPosition = m_position;
	m_position += m_velocity * deltaTime;

	// Check if we've moved down a full block
	if (m_hasStartedFalling)
	{
		int blocksMoved = static_cast<int>(std::floor(oldPosition.y) - std::floor(m_position.y));
		if (blocksMoved > 0)
		{
			m_blocksFallen += blocksMoved;
			updateMass();
		}
	}
	else
	{
		m_hasStartedFalling = true;
	}

	// Update direction based on velocity
	if (glm::length(m_velocity) > 0.01f)
	{
		m_direction = glm::normalize(m_velocity);
	}

	// Update kinetic energy
	updateEnergy();

	return true; // Keep block active
}

float FallingBlock::applyImpact(Voxel::BlockID struckBlock)
{
	// Calculate dampening from the collision
	float dampeningFactor = BlockMassRegistry::calculateDampening(m_blockType, struckBlock);

	// Apply dampening to energy
	float remainingEnergy = m_energy * dampeningFactor;

	// Energy lost in collision
	float energyLost = m_energy - remainingEnergy;

	// Update our energy
	m_energy = remainingEnergy;

	// Return the energy transferred to the struck block
	// (some energy is lost in the collision, so we return less than we lost)
	return energyLost * 0.8f; // 80% of lost energy transfers
}

void FallingBlock::deflect(const glm::vec3& deflectionDir)
{
	// If blocked, try to deflect in the suggested direction
	if (glm::length(deflectionDir) > 0.01f)
	{
		++m_deflectionCount;
		m_direction = glm::normalize(deflectionDir);

		// Reduce velocity but maintain direction
		float currentSpeed = glm::length(m_velocity);
		m_velocity = m_direction * currentSpeed * 0.7f; // Lose 30% speed on deflection

		// Additional energy loss
		m_energy *= 0.7f;
	}
	else
	{
		// No valid deflection means we're STOPPED - don't count as deflection
		// Just kill velocity and energy to trigger at-rest state
		m_velocity = glm::vec3(0.0f);
		m_energy = 0.0f;
	}
}

bool FallingBlock::isAtRest() const
{
	// If forcibly settled, always return true
	if (m_isSettled)
		return true;

	// Block is at rest if velocity is nearly zero (stopped on ground)
	float velocityMagnitude = glm::length(m_velocity);

	// Stricter threshold - block must be almost perfectly still
	// AND moving very slowly downward (not floating or bouncing up)
	return velocityMagnitude < 0.05f && m_velocity.y > -0.5f;
}

void FallingBlock::updateMass()
{
	// Add 10% mass per block fallen
	float massGain = m_baseMass * MASS_ACCUMULATION_RATE * static_cast<float>(m_blocksFallen);

	// Cap at 3x base mass
	float maxMass = m_baseMass * MAX_MASS_MULTIPLIER;

	m_currentMass = std::min(m_baseMass * (1.0f + INITIAL_MASS_BOOST) + massGain, maxMass);
}

void FallingBlock::updateEnergy()
{
	// Kinetic energy = 0.5 * mass * velocity^2
	float velocitySquared = glm::dot(m_velocity, m_velocity);
	m_energy = 0.5f * m_currentMass * velocitySquared;

	// Only add potential energy if block is still moving significantly
	// This prevents stopped blocks from having residual energy
	if (glm::length(m_velocity) > 0.5f)
	{
		// Add potential energy component based on height fallen
		// This represents the accumulated momentum
		m_energy += m_currentMass * GRAVITY * static_cast<float>(m_blocksFallen);
	}
}

} // namespace WorldPhysics
