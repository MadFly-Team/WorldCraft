#pragma once

#include <WorldGen/WorldSettings.h>
#include <glm/glm.hpp>
#include <string>
#include <ctime>

namespace Persistence
{

	// Version of the save format - increment when making breaking changes
	constexpr uint32_t SAVE_FORMAT_VERSION = 1;

	// World metadata stored in world.dat
	struct WorldMetadata
	{
		// Save format version
		uint32_t formatVersion = SAVE_FORMAT_VERSION;

		// World identification
		std::string worldName = "New World";
		std::string worldUUID;  // Unique identifier for this world

		// Timestamps
		time_t creationTime = 0;
		time_t lastPlayedTime = 0;

		// World generation settings
		WorldGen::WorldSettings settings;

		// Player state
		glm::vec3 playerPosition{ 0.0f, 100.0f, 0.0f };
		float playerYaw = -90.0f;
		float playerPitch = 0.0f;

		// Camera mode (0 = FreeFly, 1 = Character, 2 = Chase)
		int cameraMode = 0;

		// Time of day state
		float timeOfDay = 0.22f;
		bool timePaused = false;
		bool useLiveTime = false;

		// Statistics
		uint64_t totalPlayTime = 0;  // Total play time in seconds
		int modifiedChunkCount = 0;  // Number of chunks with modifications

		// Create default metadata
		static WorldMetadata createDefault(const std::string& name = "New World");

		// Generate a unique UUID for the world
		static std::string generateUUID();

		// Serialization
		bool saveToFile(const std::string& filepath) const;
		bool loadFromFile(const std::string& filepath);

		// JSON serialization helpers
		std::string toJSON() const;
		bool fromJSON(const std::string& json);
	};

} // namespace Persistence
