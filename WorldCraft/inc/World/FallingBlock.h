#pragma once

#include <Voxel/BlockTypes.h>
#include <glm/glm.hpp>

namespace WorldPhysics {

// Represents a block that has detached from the terrain and is falling/moving
class FallingBlock
{
public:
	FallingBlock(Voxel::BlockID blockType, const glm::vec3& position);

	// Physics update - returns false when block should be removed
	bool update(float deltaTime);

	// Apply impact - block hits another surface
	// Returns remaining energy after impact
	float applyImpact(Voxel::BlockID struckBlock);

	// Deflect in a new direction (when blocked)
	void deflect(const glm::vec3& deflectionDir);

	// Check if block has come to rest (energy below threshold)
	bool isAtRest() const;

	// Getters
	Voxel::BlockID getBlockType() const { return m_blockType; }
	glm::vec3 getPosition() const { return m_position; }
	glm::vec3 getVelocity() const { return m_velocity; }
	float getCurrentMass() const { return m_currentMass; }
	float getEnergy() const { return m_energy; }
	int getBlocksFallen() const { return m_blocksFallen; }

	// Check if block is underwater (set externally by physics system)
	void setUnderwater(bool underwater) { m_underwater = underwater; }
	bool isUnderwater() const { return m_underwater; }

	// Set initial velocity (for throw mechanic)
	void setVelocity(const glm::vec3& velocity) { m_velocity = velocity; }

	// Snap position to a specific location (for settling on collision)
	void setPosition(const glm::vec3& position) { m_position = position; }

	// Force block to settle immediately (stop all physics)
	void forceSettle() { m_isSettled = true; m_velocity = glm::vec3(0.0f); m_energy = 0.0f; }

	// Track failed settlement attempts (for ghost block cleanup)
	void incrementFailedSettlements() { ++m_failedSettlementAttempts; }
	int getFailedSettlements() const { return m_failedSettlementAttempts; }

	// Constants (public for access by physics system)
	static constexpr int MAX_FAILED_SETTLEMENTS = 30;      // Give up after 30 failed settlement attempts (~0.5s at 60fps)
	static constexpr int MAX_DEFLECTIONS = 3;              // Stop after 3 bounces (gentle physics)

private:
	void updateMass();
	void updateEnergy();

	Voxel::BlockID m_blockType;
	glm::vec3 m_position;
	glm::vec3 m_velocity;
	glm::vec3 m_direction; // Normalized movement direction

	float m_baseMass;      // Original block mass
	float m_currentMass;   // Accumulated mass during fall
	float m_energy;        // Impact/kinetic energy
	int m_blocksFallen;    // Number of blocks fallen so far
	int m_deflectionCount; // Number of times deflected (for giving up)
	float m_lifetime;      // Time since creation (safety timeout)

	bool m_underwater;
	bool m_hasStartedFalling;
	bool m_isSettled;          // Flag to indicate block has been forcibly settled
	int m_failedSettlementAttempts; // Track failed attempts to find a resting place

	static constexpr float GRAVITY = 9.8f;
	static constexpr float MASS_ACCUMULATION_RATE = 0.10f; // 10% per block
	static constexpr float MAX_MASS_MULTIPLIER = 3.0f;     // Up to 3x base mass
	static constexpr float INITIAL_MASS_BOOST = 0.50f;     // 50% initial boost
	static constexpr float REST_ENERGY_THRESHOLD = 5.0f;   // Very low threshold - settle quickly
	static constexpr float MAX_LIFETIME = 10.0f;           // Maximum 10 seconds before cleanup
	static constexpr int MAX_FALL_DISTANCE = 200;          // Maximum blocks to fall before cleanup
};

} // namespace WorldPhysics
