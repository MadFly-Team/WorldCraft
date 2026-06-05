#include <World/BlockPhysics.h>
#include <World/BlockMassRegistry.h>
#include <World/WaterPhysics.h>
#include <World/WaterSimulation.h>
#include <Effects/ParticleSystem.h>
#include <Chunk/ChunkWorld.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace WorldPhysics {

BlockPhysics::BlockPhysics(Chunk::ChunkWorld* world)
	: m_world(world)
	, m_waterPhysics(std::make_unique<WaterPhysics>(world))
	, m_particleSystem(nullptr)
	, m_waterSimulation(nullptr)
{
}

void BlockPhysics::update(float deltaTime)
{
	// Skip if physics disabled
	if (!m_settings.enabled)
		return;

	// Update cooldowns for recently settled blocks
	for (auto it = m_recentlySettledBlocks.begin(); it != m_recentlySettledBlocks.end(); )
	{
		it->timeRemaining -= deltaTime;
		if (it->timeRemaining <= 0.0f)
		{
			// Cooldown expired - remove from list
			it = m_recentlySettledBlocks.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Debug: Show active falling block count periodically
	static float debugTimer = 0.0f;
	debugTimer += deltaTime;
	if (debugTimer >= 5.0f)
	{
		if (!m_fallingBlocks.empty())
		{
			std::cout << "[Physics] Active falling blocks: " << m_fallingBlocks.size() << std::endl;
		}
		debugTimer = 0.0f;
	}

	// Update water physics
	if (m_waterPhysics && m_settings.enableWaterPhysics)
		m_waterPhysics->update(deltaTime);

	// Update all falling blocks
	for (auto it = m_fallingBlocks.begin(); it != m_fallingBlocks.end(); )
	{
		FallingBlock& block = **it;

		// Update physics
		bool stillActive = block.update(deltaTime);

		if (!stillActive || block.isAtRest())
		{
			// Block has settled or failed - place it back in terrain if at rest
			if (block.isAtRest())
			{
				glm::ivec3 finalPos = glm::ivec3(glm::floor(block.getPosition()));

				// CRITICAL CHECK: Verify there's actually solid support below before placing
				glm::ivec3 belowFinal = finalPos + glm::ivec3(0, -1, 0);
				Voxel::BlockID belowBlock = getBlock(belowFinal);

				// If there's no support below, the block shouldn't be settling yet
				// Give it a small downward velocity to ensure gravity continues working
				if (belowBlock == Voxel::BlockID::Air || belowBlock == Voxel::BlockID::Water)
				{
					// Increment failed settlement counter
					block.incrementFailedSettlements();

					// If too many failed attempts, give up and remove the block
					if (block.getFailedSettlements() >= FallingBlock::MAX_FAILED_SETTLEMENTS)
					{
						std::cout << "[Physics] Block at (" << finalPos.x << ", " << finalPos.y << ", " << finalPos.z 
								  << ") removed after " << block.getFailedSettlements() << " failed settlement attempts" << std::endl;
						it = m_fallingBlocks.erase(it);
						continue;
					}

					// Add a small downward velocity to ensure the block continues falling
					glm::vec3 currentVel = block.getVelocity();
					if (std::abs(currentVel.y) < 0.5f)
					{
						// Velocity too low - give it a small kick downward
						block.setVelocity(glm::vec3(currentVel.x * 0.5f, -2.0f, currentVel.z * 0.5f));
					}
					++it;
					continue;
				}

				// Make sure we're not placing inside solid terrain
				Voxel::BlockID blockAtPos = getBlock(finalPos);
				if (blockAtPos != Voxel::BlockID::Air && blockAtPos != Voxel::BlockID::Water)
				{
					// Position is occupied - try to place above
					glm::ivec3 abovePos = finalPos + glm::ivec3(0, 1, 0);
					Voxel::BlockID blockAbove = getBlock(abovePos);

					// Check if placing above would create a floating block
					if (blockAbove == Voxel::BlockID::Air || blockAbove == Voxel::BlockID::Water)
					{
						// Check if there's support below the "above" position
						// (finalPos should be solid since it's occupied, so this checks if abovePos has support)
						finalPos = abovePos;

						// Validate: Make sure the position below abovePos is actually solid
						glm::ivec3 checkBelow = finalPos + glm::ivec3(0, -1, 0);
						Voxel::BlockID belowType = getBlock(checkBelow);
						if (belowType == Voxel::BlockID::Air || belowType == Voxel::BlockID::Water)
						{
							// Would be floating - don't place yet, let it continue falling
							std::cout << "[Physics] Can't place block - would be floating. Continuing fall..." << std::endl;

							// Give it velocity to continue falling
							glm::vec3 currentVel = block.getVelocity();
							if (std::abs(currentVel.y) < 0.5f)
							{
								block.setVelocity(glm::vec3(currentVel.x * 0.5f, -2.0f, currentVel.z * 0.5f));
							}
							++it;
							continue;
						}
					}
					else
					{
						// Can't place anywhere safe - block is lost
						std::cout << "[Physics] Warning: Can't place falling block at (" 
								  << finalPos.x << ", " << finalPos.y << ", " << finalPos.z 
								  << ") - both position and above are occupied" << std::endl;
						it = m_fallingBlocks.erase(it);
						continue;
					}
				}

				// Place block back in world (replaces water if present)
				setBlock(finalPos, block.getBlockType());

				std::cout << "[Physics] Block settled at (" << finalPos.x << ", " << finalPos.y << ", " << finalPos.z 
						  << ") - adding 0.5s cooldown" << std::endl;

				// Add this position to recently settled blocks to prevent immediate re-triggering
				m_recentlySettledBlocks.push_back({finalPos, SETTLE_COOLDOWN});

				// Queue water fill to be processed after ALL physics updates complete
				// ONLY when block actually settles (not when it fails/is removed after deflections)
				m_pendingWaterFills.push_back(finalPos);
			}
			else
			{
				// Block failed (e.g., removed after 3 deflections) - but it may have created air pockets
				// So we should still queue a water fill at its last position
				glm::ivec3 lastPos = glm::ivec3(
					std::round(block.getPosition().x),
					std::round(block.getPosition().y),
					std::round(block.getPosition().z)
				);

				// Check if there's actually an air gap at or near this position that needs filling
				// Only queue if surrounded by water (prevents filling random air)
				bool hasWaterNearby = false;
				for (int dy = -1; dy <= 1 && !hasWaterNearby; ++dy)
				{
					for (int dx = -1; dx <= 1 && !hasWaterNearby; ++dx)
					{
						for (int dz = -1; dz <= 1 && !hasWaterNearby; ++dz)
						{
							if (dx == 0 && dy == 0 && dz == 0)
								continue;

							glm::ivec3 checkPos = lastPos + glm::ivec3(dx, dy, dz);
							if (getBlock(checkPos) == Voxel::BlockID::Water)
							{
								hasWaterNearby = true;
							}
						}
					}
				}

				if (hasWaterNearby)
				{
					std::cout << "[Physics] Block removed after deflections, but queueing water fill at last position (" 
							  << lastPos.x << ", " << lastPos.y << ", " << lastPos.z << ")" << std::endl;
					m_pendingWaterFills.push_back(lastPos);
				}
				else
				{
					std::cout << "[Physics] Block removed/failed - no water nearby, not queueing fill" << std::endl;
				}
			}

			// Remove from active list
			it = m_fallingBlocks.erase(it);
		}
		else
		{
			// Check for collision with terrain
			processFallingBlockCollision(block);
			++it;
		}
	}

	// Process pending impact events (chain reactions)
	std::vector<ImpactEvent> currentImpacts;
	currentImpacts.swap(m_pendingImpacts);

	for (const auto& impact : currentImpacts)
	{
		propagateImpact(impact);
	}

	// Process all queued water fills AFTER all physics updates complete
	if (m_waterNotifyCallback && !m_pendingWaterFills.empty())
	{
		// Process ALL pending fills to ensure complete coverage
		bool scanTriggered = false;
		size_t processedCount = 0;

		for (const auto& fillPos : m_pendingWaterFills)
		{
			processedCount++;  // Count this position as processed

			if (scanTriggered)
				break;  // Already triggered a scan, remaining fills will be processed next frame

			// Search in expanding radius to find the main air cavity
			// Start with radius 1, then expand up to 6 to reach cavity edges from center destructions
			const int MAX_SEARCH_RADIUS = 6;
			bool foundAndTriggered = false;

			for (int radius = 1; radius <= MAX_SEARCH_RADIUS && !foundAndTriggered; ++radius)
			{
				// Check all positions at this radius
				for (int dy = -radius; dy <= radius && !foundAndTriggered; ++dy)
				{
					for (int dx = -radius; dx <= radius && !foundAndTriggered; ++dx)
					{
						for (int dz = -radius; dz <= radius && !foundAndTriggered; ++dz)
						{
							// Skip if not at edge of radius (we want the outer shell)
							int distFromCenter = std::max({std::abs(dx), std::abs(dy), std::abs(dz)});
							if (distFromCenter != radius)
								continue;

							glm::ivec3 searchPos = fillPos + glm::ivec3(dx, dy, dz);
							Voxel::BlockID searchBlock = getBlock(searchPos);

							if (searchBlock == Voxel::BlockID::Air)
							{
								// Check if this air is adjacent to water
								static const glm::ivec3 adjacentOffsets[] = {
									{-1, 0, 0}, {1, 0, 0},
									{0, -1, 0}, {0, 1, 0},
									{0, 0, -1}, {0, 0, 1}
								};

								bool hasWaterNeighbor = false;
								for (const auto& offset : adjacentOffsets)
								{
									if (getBlock(searchPos + offset) == Voxel::BlockID::Water)
									{
										hasWaterNeighbor = true;
										break;
									}
								}

								if (hasWaterNeighbor)
								{
									m_waterNotifyCallback(searchPos.x, searchPos.y, searchPos.z);
									foundAndTriggered = true;
									scanTriggered = true;  // Mark that we've triggered a scan
									std::cout << "[Physics] Block landed at (" << fillPos.x << ", " << fillPos.y << ", " << fillPos.z 
											  << "), triggered water fill from radius " << radius << " air pocket at (" 
											  << searchPos.x << ", " << searchPos.y << ", " << searchPos.z << ")" << std::endl;
								}
							}
						}
					}
				}
			}

			if (!foundAndTriggered)
			{
				std::cout << "[Physics] Block landed at (" << fillPos.x << ", " << fillPos.y << ", " << fillPos.z 
						  << "), but no suitable air pocket found for water fill" << std::endl;
			}
		}

		// Remove only the processed positions from the queue
		// This keeps any unprocessed positions for the next frame
		if (processedCount > 0)
		{
			m_pendingWaterFills.erase(m_pendingWaterFills.begin(), m_pendingWaterFills.begin() + processedCount);
			if (!m_pendingWaterFills.empty())
			{
				std::cout << "[Physics] Processed " << processedCount << " pending fills, " 
						  << m_pendingWaterFills.size() << " remaining for next frame" << std::endl;
			}
		}
	}
}

void BlockPhysics::triggerBlockFall(const glm::ivec3& blockPos)
{
	Voxel::BlockID blockType = getBlock(blockPos);

	// Don't trigger air or bedrock
	if (blockType == Voxel::BlockID::Air || blockType == Voxel::BlockID::Bedrock)
		return;

	std::cout << "[Physics] Triggering fall for block type " << static_cast<int>(blockType) 
			  << " at (" << blockPos.x << ", " << blockPos.y << ", " << blockPos.z << ")" << std::endl;

	// Remove block from terrain
	removeBlock(blockPos);

	// Create falling block entity
	glm::vec3 worldPos = glm::vec3(blockPos) + glm::vec3(0.5f); // Center of block
	auto fallingBlock = std::make_unique<FallingBlock>(blockType, worldPos);

	// Check if starting underwater
	fallingBlock->setUnderwater(isUnderWater(blockPos));

	m_fallingBlocks.push_back(std::move(fallingBlock));

	std::cout << "[Physics] Now have " << m_fallingBlocks.size() << " active falling blocks" << std::endl;
}

bool BlockPhysics::shouldBlockFall(const glm::ivec3& blockPos) const
{
	// Don't trigger falls if physics is disabled
	if (!m_settings.enabled)
	{
		std::cout << "[Physics] Physics disabled, block won't fall" << std::endl;
		return false;
	}

	Voxel::BlockID blockType = getBlock(blockPos);

	// Air and bedrock never fall
	if (blockType == Voxel::BlockID::Air || blockType == Voxel::BlockID::Bedrock)
		return false;

	// Water has special flow behavior (handled elsewhere)
	if (BlockMassRegistry::isFluid(blockType))
		return false;

	// Check if this block recently settled - don't let it fall again immediately
	for (const auto& cooldown : m_recentlySettledBlocks)
	{
		if (cooldown.pos == blockPos)
		{
			// Block is on cooldown - don't trigger fall
			std::cout << "[Physics] Block at (" << blockPos.x << ", " << blockPos.y << ", " << blockPos.z 
					  << ") is on cooldown (" << cooldown.timeRemaining << "s remaining) - preventing re-trigger" << std::endl;
			return false;
		}
	}

	// Check if there's support below
	glm::ivec3 below = blockPos + glm::ivec3(0, -1, 0);
	Voxel::BlockID belowBlock = getBlock(below);

	// If there's solid support below, block doesn't fall
	if (belowBlock != Voxel::BlockID::Air && belowBlock != Voxel::BlockID::Water)
		return false;

	// No direct support below - check for horizontal neighbors (wall support)
	// If the block has at least 2 solid neighbors, it stays attached to the wall
	int solidNeighbors = 0;
	const glm::ivec3 horizontalOffsets[] = {
		{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}
	};

	for (const auto& offset : horizontalOffsets)
	{
		glm::ivec3 neighborPos = blockPos + offset;
		Voxel::BlockID neighborBlock = getBlock(neighborPos);
		if (neighborBlock != Voxel::BlockID::Air && neighborBlock != Voxel::BlockID::Water)
		{
			solidNeighbors++;
			if (solidNeighbors >= 2)
			{
				// Block is part of a wall or structure - don't fall
				return false;
			}
		}
	}

	// No support below and insufficient horizontal support - should fall
	std::cout << "[Physics] Block at (" << blockPos.x << ", " << blockPos.y << ", " << blockPos.z 
			  << ") should fall - below is " << static_cast<int>(belowBlock) 
			  << ", solid neighbors: " << solidNeighbors << std::endl;

	return true;
}

void BlockPhysics::clear()
{
	m_fallingBlocks.clear();
	m_pendingImpacts.clear();
	m_recentlySettledBlocks.clear();

	if (m_waterPhysics)
		m_waterPhysics->clear();
}

void BlockPhysics::processFallingBlockCollision(FallingBlock& block)
{
	glm::vec3 pos = block.getPosition();
	glm::ivec3 blockPos = glm::ivec3(glm::floor(pos));

	// FIRST: Check if the falling block is currently inside solid terrain
	// This can happen if it fell too fast or spawned in a bad spot
	if (isSolid(blockPos))
	{
		// Block is inside terrain - position it on top of this block
		glm::vec3 restPosition = glm::vec3(
			pos.x,
			static_cast<float>(blockPos.y + 1),  // Place on top
			pos.z
		);
		block.setPosition(restPosition);
		block.forceSettle(); // Immediately settle (no more physics updates)
		return;
	}

	// Check the block below our current position
	glm::ivec3 below = blockPos + glm::ivec3(0, -1, 0);

	// Check if we've hit something below or reached bedrock
	if (isSolid(below) || below.y < 0)
	{
		Voxel::BlockID struckBlock = getBlock(below);

		// Special handling for water - blocks sink through BUT create splash
		if (struckBlock == Voxel::BlockID::Water)
		{
			block.setUnderwater(true);
			float impactEnergy = block.getEnergy();

			if (m_settings.enableWaterPhysics)
			{
				// Create water ripple effect
				handleWaterDisplacement(below, impactEnergy * 0.1f);

				// Create splash particles AND ripple effect if we have a particle system
				if (m_particleSystem && impactEnergy > 50.0f)
				{
					glm::vec3 splashPos = glm::vec3(below) + glm::vec3(0.5f, 1.0f, 0.5f);
					float splashIntensity = std::min(impactEnergy / 200.0f, 3.0f);

					// Create upward splash
					m_particleSystem->createSplash(splashPos, splashIntensity, glm::vec3(0, 1, 0));

					// TEMPORARILY DISABLED: Create outward ripple effect on water surface
					// float rippleRadius = std::min(impactEnergy / 100.0f, 5.0f);
					// m_particleSystem->createRippleEffect(splashPos, rippleRadius);
				}
			}
			// Block continues falling through water but slower
			return;
		}

		// Calculate impact energy based on velocity
		float impactEnergy = block.getEnergy();

		// RE-ENABLE DESTRUCTION with proper threshold
		// Blocks will only destroy terrain if thrown hard enough
		// Threshold: 300 energy units (about 1.5x charged throw directly down)
		bool shouldDestroy = (impactEnergy > 300.0f) && BlockMassRegistry::shouldDestroy(impactEnergy, struckBlock);

		if (shouldDestroy)
		{
			// Destroy the block
			removeBlock(below);
			std::cout << "[Physics] Block destroyed at (" << below.x << ", " << below.y << ", " << below.z 
					  << ") with energy: " << impactEnergy << std::endl;

			// Queue this destruction position for water fill check
			// The destroyed position is likely adjacent to the water cavity
			m_pendingWaterFills.push_back(below);

			// Create impact event for neighbors (propagate destruction)
			ImpactEvent impact;
			impact.position = below;
			impact.energy = impactEnergy * 0.3f; // 30% of impact energy propagates
			impact.direction = block.getVelocity();
			m_pendingImpacts.push_back(impact);

			// Block continues BUT loses significant energy
			block.deflect(glm::vec3(0, -0.3f, 0)); // Moderate downward velocity
		}
		else
		{
			// Block is STOPPED - snap it to rest position above the struck block
			// The block should sit exactly on top of the solid block below
			glm::vec3 restPosition = glm::vec3(
				pos.x,                    // Keep X position
				static_cast<float>(below.y + 1),  // Sit on top of the block below (Y = below + 1)
				pos.z                     // Keep Z position
			);

			block.setPosition(restPosition);
			block.forceSettle(); // Immediately settle (no more physics updates)
		}
	}

	// Check if block is now underwater
	if (getBlock(blockPos) == Voxel::BlockID::Water)
	{
		block.setUnderwater(true);
	}
}

void BlockPhysics::propagateImpact(const ImpactEvent& impact)
{
	// Don't propagate weak impacts
	if (impact.energy < 10.0f)
		return;

	// Check all 6 neighbors
	static const glm::ivec3 neighbors[] = {
		{1, 0, 0}, {-1, 0, 0},
		{0, 1, 0}, {0, -1, 0},
		{0, 0, 1}, {0, 0, -1}
	};

	for (const auto& offset : neighbors)
	{
		glm::ivec3 neighborPos = impact.position + offset;

		if (!isInBounds(neighborPos))
			continue;

		Voxel::BlockID neighborBlock = getBlock(neighborPos);

		// Skip air and bedrock
		if (neighborBlock == Voxel::BlockID::Air || neighborBlock == Voxel::BlockID::Bedrock)
			continue;

		// Water is displaced, not impacted
		if (neighborBlock == Voxel::BlockID::Water)
		{
			handleWaterDisplacement(neighborPos, impact.energy);
			continue;
		}

		// Calculate direction bias (favor impact direction)
		glm::vec3 toNeighbor = glm::vec3(offset);
		float directionBias = glm::dot(glm::normalize(impact.direction), glm::normalize(toNeighbor));
		directionBias = std::max(0.0f, directionBias); // Only forward direction

		float effectiveEnergy = impact.energy * (0.5f + 0.5f * directionBias);

		// Check if block should be destroyed or triggered to fall
		if (BlockMassRegistry::shouldDestroy(effectiveEnergy, neighborBlock))
		{
			// Destroy block
			removeBlock(neighborPos);

			// Queue this destruction position for water fill check
			m_pendingWaterFills.push_back(neighborPos);
		}
		else if (effectiveEnergy > BlockMassRegistry::getMass(neighborBlock))
		{
			// Energy exceeds block mass - trigger fall
			triggerBlockFall(neighborPos);
		}
	}
}

glm::vec3 BlockPhysics::calculateDeflectionDirection(const glm::vec3& originalDir, const glm::ivec3& blockedPos)
{
	// Try 135° anticlockwise from original direction (as specified)
	float angle = glm::radians(135.0f);
	glm::vec3 deflected = originalDir;

	// Rotate in XZ plane
	float cosA = std::cos(angle);
	float sinA = std::sin(angle);
	deflected.x = originalDir.x * cosA - originalDir.z * sinA;
	deflected.z = originalDir.x * sinA + originalDir.z * cosA;

	// Check if deflected direction is clear
	glm::ivec3 deflectedPos = blockedPos + glm::ivec3(glm::sign(deflected));

	if (!isSolid(deflectedPos))
	{
		return deflected;
	}

	// Try reverse direction
	glm::vec3 reversed = -originalDir;
	glm::ivec3 reversedPos = blockedPos + glm::ivec3(glm::sign(reversed));

	if (!isSolid(reversedPos))
	{
		return reversed;
	}

	// No good direction - return zero to signal block should settle
	return glm::vec3(0.0f);
}

bool BlockPhysics::isSolid(const glm::ivec3& pos) const
{
	if (!m_world)
		return true;

	return m_world->isBlockSolid(
		static_cast<float>(pos.x) + 0.5f,
		static_cast<float>(pos.y) + 0.5f,
		static_cast<float>(pos.z) + 0.5f
	);
}

bool BlockPhysics::isInBounds(const glm::ivec3& pos) const
{
	// Simple bounds check (can be made more sophisticated)
	return pos.y >= 0 && pos.y < 256; // Typical world height
}

Voxel::BlockID BlockPhysics::getBlock(const glm::ivec3& pos) const
{
	if (!m_world)
		return Voxel::BlockID::Air;

	return m_world->getBlockAt(
		static_cast<float>(pos.x) + 0.5f,
		static_cast<float>(pos.y) + 0.5f,
		static_cast<float>(pos.z) + 0.5f
	);
}

void BlockPhysics::setBlock(const glm::ivec3& pos, Voxel::BlockID blockType)
{
	if (!m_world)
		return;

	m_world->setBlockAt(
		static_cast<float>(pos.x) + 0.5f,
		static_cast<float>(pos.y) + 0.5f,
		static_cast<float>(pos.z) + 0.5f,
		blockType
	);
}

void BlockPhysics::removeBlock(const glm::ivec3& pos)
{
	// Get the block type before destroying (for particle effects)
	Voxel::BlockID blockType = getBlock(pos);

	// Actually remove the block
	setBlock(pos, Voxel::BlockID::Air);

	// NOTE: Water notifications are NOT sent here during physics updates
	// Instead, water fills are queued and processed at the end of the update cycle
	// This prevents multiple interfering scans during block destruction chains

	// Create debris particles for destroyed blocks (not air or water)
	if (m_particleSystem && blockType != Voxel::BlockID::Air && blockType != Voxel::BlockID::Water)
	{
		glm::vec3 particlePos = glm::vec3(pos) + glm::vec3(0.5f, 0.5f, 0.5f);

		// Choose debris color based on block type
		glm::vec4 debrisColor(0.6f, 0.5f, 0.4f, 1.0f); // Default brown/grey

		// Customize colors for different block types
		if (blockType == Voxel::BlockID::Grass)
			debrisColor = glm::vec4(0.4f, 0.6f, 0.3f, 1.0f); // Green
		else if (blockType == Voxel::BlockID::Stone)
			debrisColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f); // Grey
		else if (blockType == Voxel::BlockID::Sand)
			debrisColor = glm::vec4(0.8f, 0.7f, 0.5f, 1.0f); // Tan

		// Create debris particles
		m_particleSystem->createDebris(particlePos, debrisColor, 12);

		std::cout << "[Physics] Created debris particles for destroyed block type: " << static_cast<int>(blockType) << std::endl;
	}
}

void BlockPhysics::handleWaterDisplacement(const glm::ivec3& impactPos, float energy)
{
	// Delegate to water physics system
	if (m_waterPhysics)
	{
		m_waterPhysics->displaceWater(impactPos, energy);
	}
}

bool BlockPhysics::isUnderWater(const glm::ivec3& pos) const
{
	// Use water physics for accurate underwater detection
	if (m_waterPhysics)
		return m_waterPhysics->isUnderwater(pos);

	return getBlock(pos) == Voxel::BlockID::Water;
}

void BlockPhysics::throwBlock(const glm::ivec3& blockPos, const glm::vec3& direction, float velocityMultiplier)
{
	if (!m_settings.enabled)
		return;

	Voxel::BlockID blockType = getBlock(blockPos);

	// Don't throw air, bedrock, or water
	if (blockType == Voxel::BlockID::Air || 
		blockType == Voxel::BlockID::Bedrock ||
		BlockMassRegistry::isFluid(blockType))
		return;

	// Remove block from terrain
	removeBlock(blockPos);

	// Create falling block entity
	glm::vec3 worldPos = glm::vec3(blockPos) + glm::vec3(0.5f);
	auto fallingBlock = std::make_unique<FallingBlock>(blockType, worldPos);

	// Apply initial velocity based on throw direction
	// Make throw speed reasonable and visible (10-20 blocks/sec)
	float blockMass = BlockMassRegistry::getMass(blockType);
	// Light blocks (dirt=100) throw at ~10 blocks/sec, heavy (rock=500) at ~15 blocks/sec
	float throwSpeed = 5.0f + (blockMass / 100.0f);  // Base 5 + mass scaling
	throwSpeed = std::min(throwSpeed, 20.0f);  // Cap at 20 blocks/sec
	glm::vec3 throwVelocity = glm::normalize(direction) * throwSpeed;

	std::cout << "[Throw] Throwing " << static_cast<int>(blockType) 
			  << " with speed " << throwSpeed << " blocks/sec" << std::endl;

	// Set the initial velocity for the throw
	fallingBlock->setVelocity(throwVelocity);

	// Check if starting underwater
	fallingBlock->setUnderwater(isUnderWater(blockPos));

	m_fallingBlocks.push_back(std::move(fallingBlock));
}

void BlockPhysics::addFallingBlock(std::unique_ptr<FallingBlock> block)
{
	if (!m_settings.enabled || !block)
		return;

	std::cout << "[Physics] Adding pre-created falling block at (" 
			  << block->getPosition().x << ", " << block->getPosition().y << ", " << block->getPosition().z << ")" << std::endl;

	m_fallingBlocks.push_back(std::move(block));
}

} // namespace WorldPhysics
