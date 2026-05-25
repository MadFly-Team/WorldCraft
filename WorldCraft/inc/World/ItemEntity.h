#pragma once

#include <Voxel/BlockTypes.h>
#include <glm/glm.hpp>
#include <functional>

namespace World
{

// Callback to check if a position is solid (for collision detection)
using CollisionCheckFunc = std::function<bool(float x, float y, float z)>;

// Represents a dropped item in the world (Minecraft-style pickup)
// Items float with sine-wave animation, attract to nearby players, and despawn after 20 seconds
class ItemEntity
{
public:
	ItemEntity(const glm::vec3& position, Voxel::BlockID blockType, int stackCount = 1);

	// Update item physics, animation, and lifetime
	// collisionCheck: callback to check if a block position is solid
	// Returns false if item should be removed (collected or expired)
	bool update(float deltaTime, const glm::vec3& playerPos, float pickupRadius = 2.5f,
				CollisionCheckFunc collisionCheck = nullptr);

	// Get current world position (includes floating animation offset)
	glm::vec3 getPosition() const;

	// Get the block type this item represents
	Voxel::BlockID getBlockType() const { return m_blockType; }

	// Get stack count
	int getStackCount() const { return m_stackCount; }

	// Check if item is being attracted to player
	bool isBeingCollected() const { return m_isBeingCollected; }

	// Mark item as collected (ready for removal)
	void collect() { m_shouldRemove = true; }

	// Check if item should be removed from world
	bool shouldRemove() const { return m_shouldRemove; }

	// Get rotation for rendering (smooth continuous rotation)
	float getRotation() const { return m_rotation; }

private:
	glm::vec3 m_basePosition;       // Base world position (before float animation)
	glm::vec3 m_velocity;           // Current velocity (used during spawn and attraction)
	Voxel::BlockID m_blockType;     // Block type this item represents
	int m_stackCount;               // Number of items in this stack (1-100)

	float m_lifetime;               // Time since spawn (despawn at 20 seconds)
	float m_floatTime;              // Time accumulator for sine-wave float animation
	float m_rotation;               // Current rotation angle (degrees)

	bool m_isBeingCollected;        // True when magnetically attracted to player
	bool m_shouldRemove;            // True when collected or expired

	static constexpr float LIFETIME_MAX = 20.0f;      // 20 seconds until despawn
	static constexpr float FLOAT_SPEED = 2.0f;        // Sine wave frequency
	static constexpr float FLOAT_AMPLITUDE = 0.15f;   // Float height range
	static constexpr float ROTATION_SPEED = 90.0f;    // Degrees per second
	static constexpr float COLLECTION_SPEED = 8.0f;   // Speed when moving to player
	static constexpr float SPAWN_VELOCITY_UP = 3.0f;  // Initial upward velocity
	static constexpr float GRAVITY = -9.8f;           // Gravity acceleration
};

} // namespace World
