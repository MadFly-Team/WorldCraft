#pragma once

namespace WorldPhysics {

// Runtime-configurable physics settings
struct PhysicsSettings
{
	bool enabled = true;               // Master enable/disable for physics
	float gravityMultiplier = 1.0f;    // Multiply gravity (0.5 = half, 2.0 = double)
	float massAccumulationRate = 0.10f; // Percentage mass gain per block fallen
	float maxMassMultiplier = 3.0f;    // Maximum mass (as multiple of base)
	float energyDampeningFactor = 1.0f; // Global dampening (higher = more energy loss)
	float destructionThreshold = 100.0f; // Multiplier for destruction (100.0 = very hard to destroy)
	int maxChainReactionDepth = 5;     // Maximum propagation steps
	bool enableWaterPhysics = true;    // Enable water displacement/ripples

	// Default constructor with sensible defaults
	PhysicsSettings() = default;
};

} // namespace WorldPhysics
