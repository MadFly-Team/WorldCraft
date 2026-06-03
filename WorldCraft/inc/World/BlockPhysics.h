#pragma once

#include <World/FallingBlock.h>
#include <World/WaterPhysics.h>
#include <World/PhysicsSettings.h>
#include <Voxel/BlockTypes.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>

// Forward declarations
namespace Chunk {
	class ChunkWorld;
}

namespace World {
	class WaterSimulation;
}

namespace Effects {
	class ParticleSystem;
}

namespace WorldPhysics {

// Impact event to propagate to neighboring blocks
struct ImpactEvent
{
	glm::ivec3 position;   // Block position
	float energy;          // Impact energy
	glm::vec3 direction;   // Impact direction
};

// Main physics simulation manager
class BlockPhysics
{
public:
	explicit BlockPhysics(Chunk::ChunkWorld* world);

	// Update all falling blocks and physics simulation
	void update(float deltaTime);

	// Trigger a block to start falling
	void triggerBlockFall(const glm::ivec3& blockPos);

	// Check if a block should fall (no support below)
	bool shouldBlockFall(const glm::ivec3& blockPos) const;

	// Get count of active falling blocks
	size_t getActiveFallingBlockCount() const { return m_fallingBlocks.size(); }

	// Get read-only access to falling blocks list (for rendering)
	const std::vector<std::unique_ptr<FallingBlock>>& getFallingBlocks() const { return m_fallingBlocks; }

	// Add a pre-created falling block (for pick-up-and-throw mechanic)
	void addFallingBlock(std::unique_ptr<FallingBlock> block);

	// Clear all physics state (for world reload)
	void clear();

	// Get water physics subsystem
	WaterPhysics* getWaterPhysics() { return m_waterPhysics.get(); }

	// Physics settings access
	PhysicsSettings& getSettings() { return m_settings; }
	const PhysicsSettings& getSettings() const { return m_settings; }
	void setSettings(const PhysicsSettings& settings) { m_settings = settings; }

	// Throw a block with initial velocity (for throw mechanic)
	void throwBlock(const glm::ivec3& blockPos, const glm::vec3& direction, float velocityMultiplier = 2.0f);

	// Set particle system for splash effects
	void setParticleSystem(Effects::ParticleSystem* particleSystem) { m_particleSystem = particleSystem; }

	// Set callback for water simulation notification (called when blocks are destroyed)
	using WaterNotifyCallback = std::function<void(int x, int y, int z)>;
	void setWaterNotifyCallback(WaterNotifyCallback callback) { m_waterNotifyCallback = callback; }

	// Set water simulation pointer (used to check if scan is in progress)
	void setWaterSimulation(World::WaterSimulation* waterSim) { m_waterSimulation = waterSim; }

private:
	// Process a falling block collision with terrain
	void processFallingBlockCollision(FallingBlock& block);

	// Propagate impact energy to neighboring blocks
	void propagateImpact(const ImpactEvent& impact);

	// Try to deflect a block in a new direction when blocked
	glm::vec3 calculateDeflectionDirection(const glm::vec3& originalDir, const glm::ivec3& blockedPos);

	// Check if position is solid (blocks movement)
	bool isSolid(const glm::ivec3& pos) const;

	// Check if position is within world bounds
	bool isInBounds(const glm::ivec3& pos) const;

	// Get block at position
	Voxel::BlockID getBlock(const glm::ivec3& pos) const;

	// Set block at position
	void setBlock(const glm::ivec3& pos, Voxel::BlockID blockType);

	// Remove block at position (set to air)
	void removeBlock(const glm::ivec3& pos);

	// Water-specific handling
	void handleWaterDisplacement(const glm::ivec3& impactPos, float energy);
	bool isUnderWater(const glm::ivec3& pos) const;

	Chunk::ChunkWorld* m_world;
	std::unique_ptr<WaterPhysics> m_waterPhysics;
	std::vector<std::unique_ptr<FallingBlock>> m_fallingBlocks;
	std::vector<ImpactEvent> m_pendingImpacts;
	PhysicsSettings m_settings;  // Runtime-configurable settings
	Effects::ParticleSystem* m_particleSystem;  // Optional particle system for effects
	WaterNotifyCallback m_waterNotifyCallback;  // Callback to notify water simulation of changes
	World::WaterSimulation* m_waterSimulation;  // Pointer to water simulation (to check scan status)
	std::vector<glm::ivec3> m_pendingWaterFills; // Queue water fill positions to process after physics update completes

	// Recently settled blocks cooldown (prevents immediate re-triggering)
	struct SettledBlockCooldown
	{
		glm::ivec3 pos;
		float timeRemaining;
	};
	std::vector<SettledBlockCooldown> m_recentlySettledBlocks;
	static constexpr float SETTLE_COOLDOWN = 0.5f;  // 0.5 second cooldown before block can fall again

	// Constants
	static constexpr float COLLISION_THRESHOLD = 0.5f; // Distance to detect collision
	static constexpr int MAX_IMPACT_PROPAGATION = 5;   // Max chain reaction depth
};

} // namespace WorldPhysics
