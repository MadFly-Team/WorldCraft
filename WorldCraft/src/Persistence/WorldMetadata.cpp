#include <Persistence/WorldMetadata.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>

namespace Persistence
{

WorldMetadata WorldMetadata::createDefault(const std::string& name)
{
	WorldMetadata meta;
	meta.worldName = name;
	meta.worldUUID = generateUUID();
	meta.creationTime = std::time(nullptr);
	meta.lastPlayedTime = meta.creationTime;
	meta.settings = WorldGen::WorldSettings::createDefault();
	return meta;
}

std::string WorldMetadata::generateUUID()
{
	// Simple UUID v4 generation
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 15);
	std::uniform_int_distribution<> dis2(8, 11);

	std::stringstream ss;
	ss << std::hex;
	for (int i = 0; i < 8; i++) ss << dis(gen);
	ss << "-";
	for (int i = 0; i < 4; i++) ss << dis(gen);
	ss << "-4";
	for (int i = 0; i < 3; i++) ss << dis(gen);
	ss << "-";
	ss << dis2(gen);
	for (int i = 0; i < 3; i++) ss << dis(gen);
	ss << "-";
	for (int i = 0; i < 12; i++) ss << dis(gen);

	return ss.str();
}

std::string WorldMetadata::toJSON() const
{
	std::stringstream ss;
	ss << "{\n";
	ss << "  \"formatVersion\": " << formatVersion << ",\n";
	ss << "  \"worldName\": \"" << worldName << "\",\n";
	ss << "  \"worldUUID\": \"" << worldUUID << "\",\n";
	ss << "  \"creationTime\": " << creationTime << ",\n";
	ss << "  \"lastPlayedTime\": " << lastPlayedTime << ",\n";
	ss << "  \"totalPlayTime\": " << totalPlayTime << ",\n";
	ss << "  \"modifiedChunkCount\": " << modifiedChunkCount << ",\n";

	// World settings
	ss << "  \"settings\": {\n";
	ss << "    \"seed\": " << settings.seed << ",\n";
	ss << "    \"renderDistance\": " << settings.renderDistance << ",\n";
	ss << "    \"terrainScale\": " << settings.terrainScale << ",\n";
	ss << "    \"terrainAmplitude\": " << settings.terrainAmplitude << ",\n";
	ss << "    \"seaLevel\": " << settings.seaLevel << ",\n";
	ss << "    \"biomeScale\": " << settings.biomeScale << ",\n";
	ss << "    \"biomeBlend\": " << settings.biomeBlend << ",\n";
	ss << "    \"enableMountainCaves\": " << (settings.enableMountainCaves ? "true" : "false") << ",\n";
	ss << "    \"mountainCaveRarity\": " << settings.mountainCaveRarity << ",\n";
	ss << "    \"mountainCaveSize\": " << settings.mountainCaveSize << ",\n";
	ss << "    \"enableDeepCaverns\": " << (settings.enableDeepCaverns ? "true" : "false") << ",\n";
	ss << "    \"cavernSize\": " << settings.cavernSize << ",\n";
	ss << "    \"cavernMinDepth\": " << settings.cavernMinDepth << ",\n";
	ss << "    \"cavernMaxDepth\": " << settings.cavernMaxDepth << ",\n";
	ss << "    \"enableOres\": " << (settings.enableOres ? "true" : "false") << ",\n";
	ss << "    \"oreAbundance\": " << settings.oreAbundance << ",\n";
	ss << "    \"enableTrees\": " << (settings.enableTrees ? "true" : "false") << ",\n";
	ss << "    \"treeDensity\": " << settings.treeDensity << ",\n";
	ss << "    \"enableWaterFlow\": " << (settings.enableWaterFlow ? "true" : "false") << ",\n";
	ss << "    \"enableWaterWaves\": " << (settings.enableWaterWaves ? "true" : "false") << ",\n";
	ss << "    \"waterFlowRate\": " << settings.waterFlowRate << ",\n";
	ss << "    \"enableAutoSave\": " << (settings.enableAutoSave ? "true" : "false") << ",\n";
	ss << "    \"loadLastWorldOnStartup\": " << (settings.loadLastWorldOnStartup ? "true" : "false") << ",\n";
	ss << "    \"worldName\": \"" << settings.worldName << "\",\n";
	ss << "    \"startX\": " << settings.startX << ",\n";
	ss << "    \"startY\": " << settings.startY << ",\n";
	ss << "    \"startZ\": " << settings.startZ << "\n";
	ss << "  },\n";

	// Player state
	ss << "  \"player\": {\n";
	ss << "    \"position\": [" << playerPosition.x << ", " << playerPosition.y << ", " << playerPosition.z << "],\n";
	ss << "    \"yaw\": " << playerYaw << ",\n";
	ss << "    \"pitch\": " << playerPitch << ",\n";
	ss << "    \"cameraMode\": " << cameraMode << "\n";
	ss << "  },\n";

	// Time state
	ss << "  \"time\": {\n";
	ss << "    \"timeOfDay\": " << timeOfDay << ",\n";
	ss << "    \"timePaused\": " << (timePaused ? "true" : "false") << ",\n";
	ss << "    \"useLiveTime\": " << (useLiveTime ? "true" : "false") << "\n";
	ss << "  },\n";

	// Inventory state
	ss << "  \"inventory\": {\n";
	ss << "    \"capacity\": " << inventoryCapacity << ",\n";
	ss << "    \"selectedSlot\": " << selectedSlot << ",\n";
	ss << "    \"slots\": [\n";
	for (size_t i = 0; i < inventorySlots.size(); ++i) {
		if (i > 0) ss << ",\n";
		ss << "      {\"blockID\": " << inventorySlots[i].blockID 
		   << ", \"stackCount\": " << inventorySlots[i].stackCount << "}";
	}
	ss << "\n    ],\n";
	ss << "    \"unlockedBlueprints\": [";
	for (size_t i = 0; i < unlockedBlueprints.size(); ++i) {
		if (i > 0) ss << ", ";
		ss << unlockedBlueprints[i];
	}
	ss << "]\n";
	ss << "  }\n";

	ss << "}\n";
	return ss.str();
}

bool WorldMetadata::fromJSON(const std::string& json)
{
	// Simple JSON parser - in production would use a library like nlohmann/json
	// For now, implement basic parsing for the fields we need

	auto findValue = [&json](const std::string& key) -> std::string {
		std::string searchKey = "\"" + key + "\":";
		size_t pos = json.find(searchKey);
		if (pos == std::string::npos) return "";

		pos += searchKey.length();
		// Skip whitespace
		while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) pos++;

		size_t end = pos;
		if (json[pos] == '"') {
			// String value
			pos++;
			end = json.find('"', pos);
			if (end == std::string::npos) return "";
			return json.substr(pos, end - pos);
		} else if (json[pos] == '[') {
			// Array value
			end = json.find(']', pos);
			if (end == std::string::npos) return "";
			return json.substr(pos, end - pos + 1);
		} else {
			// Numeric or boolean
			while (end < json.length() && json[end] != ',' && json[end] != '\n' && json[end] != '}') end++;
			std::string val = json.substr(pos, end - pos);
			// Trim whitespace
			size_t trimEnd = val.find_last_not_of(" \t\n\r");
			if (trimEnd != std::string::npos) val = val.substr(0, trimEnd + 1);
			return val;
		}
	};

	auto safeParseInt = [&findValue](const std::string& key, int defaultValue) -> int {
		try {
			std::string val = findValue(key);
			return val.empty() ? defaultValue : std::stoi(val);
		} catch (...) {
			return defaultValue;
		}
	};

	auto safeParseFloat = [&findValue](const std::string& key, float defaultValue) -> float {
		try {
			std::string val = findValue(key);
			return val.empty() ? defaultValue : std::stof(val);
		} catch (...) {
			return defaultValue;
		}
	};

	auto safeParseBool = [&findValue](const std::string& key, bool defaultValue) -> bool {
		std::string val = findValue(key);
		if (val.empty()) return defaultValue;
		return (val == "true");
	};

	auto safeParseLongLong = [&findValue](const std::string& key, long long defaultValue) -> long long {
		try {
			std::string val = findValue(key);
			return val.empty() ? defaultValue : std::stoll(val);
		} catch (...) {
			return defaultValue;
		}
	};

	auto safeParseULongLong = [&findValue](const std::string& key, unsigned long long defaultValue) -> unsigned long long {
		try {
			std::string val = findValue(key);
			return val.empty() ? defaultValue : std::stoull(val);
		} catch (...) {
			return defaultValue;
		}
	};

	try {
		formatVersion = safeParseInt("formatVersion", SAVE_FORMAT_VERSION);
		worldName = findValue("worldName");
		if (worldName.empty()) worldName = "Unknown World";
		worldUUID = findValue("worldUUID");
		creationTime = safeParseLongLong("creationTime", std::time(nullptr));
		lastPlayedTime = safeParseLongLong("lastPlayedTime", creationTime);
		totalPlayTime = safeParseULongLong("totalPlayTime", 0);
		modifiedChunkCount = safeParseInt("modifiedChunkCount", 0);

		// World settings (with defaults from WorldSettings::createDefault())
		WorldGen::WorldSettings defaultSettings = WorldGen::WorldSettings::createDefault();
		settings.seed = safeParseInt("seed", defaultSettings.seed);
		settings.renderDistance = safeParseInt("renderDistance", defaultSettings.renderDistance);
		settings.terrainScale = safeParseFloat("terrainScale", defaultSettings.terrainScale);
		settings.terrainAmplitude = safeParseFloat("terrainAmplitude", defaultSettings.terrainAmplitude);
		settings.seaLevel = safeParseInt("seaLevel", defaultSettings.seaLevel);
		settings.biomeScale = safeParseFloat("biomeScale", defaultSettings.biomeScale);
		settings.biomeBlend = safeParseFloat("biomeBlend", defaultSettings.biomeBlend);

		settings.enableMountainCaves = safeParseBool("enableMountainCaves", defaultSettings.enableMountainCaves);
		settings.mountainCaveRarity = safeParseFloat("mountainCaveRarity", defaultSettings.mountainCaveRarity);
		settings.mountainCaveSize = safeParseFloat("mountainCaveSize", defaultSettings.mountainCaveSize);

		settings.enableDeepCaverns = safeParseBool("enableDeepCaverns", defaultSettings.enableDeepCaverns);
		settings.cavernSize = safeParseFloat("cavernSize", defaultSettings.cavernSize);
		settings.cavernMinDepth = safeParseInt("cavernMinDepth", defaultSettings.cavernMinDepth);
		settings.cavernMaxDepth = safeParseInt("cavernMaxDepth", defaultSettings.cavernMaxDepth);

		settings.enableOres = safeParseBool("enableOres", defaultSettings.enableOres);
		settings.oreAbundance = safeParseFloat("oreAbundance", defaultSettings.oreAbundance);

		settings.enableTrees = safeParseBool("enableTrees", defaultSettings.enableTrees);
		settings.treeDensity = safeParseFloat("treeDensity", defaultSettings.treeDensity);

		settings.enableWaterFlow = safeParseBool("enableWaterFlow", defaultSettings.enableWaterFlow);
		settings.enableWaterWaves = safeParseBool("enableWaterWaves", defaultSettings.enableWaterWaves);
		settings.waterFlowRate = safeParseFloat("waterFlowRate", defaultSettings.waterFlowRate);
		settings.enableAutoSave = safeParseBool("enableAutoSave", defaultSettings.enableAutoSave);
		settings.loadLastWorldOnStartup = safeParseBool("loadLastWorldOnStartup", defaultSettings.loadLastWorldOnStartup);

		// Parse array for position (with default position)
		std::string posStr = findValue("position");
		if (!posStr.empty()) {
			try {
				std::stringstream ss(posStr);
				char ch;
				ss >> ch; // skip '['
				ss >> playerPosition.x >> ch >> playerPosition.y >> ch >> playerPosition.z;
			} catch (...) {
				playerPosition = glm::vec3(0.0f, 100.0f, 0.0f);
			}
		} else {
			playerPosition = glm::vec3(0.0f, 100.0f, 0.0f);
		}

		playerYaw = safeParseFloat("yaw", -90.0f);
		playerPitch = safeParseFloat("pitch", 0.0f);
		cameraMode = safeParseInt("cameraMode", 0);

		timeOfDay = safeParseFloat("timeOfDay", 0.22f);
		timePaused = safeParseBool("timePaused", false);
		useLiveTime = safeParseBool("useLiveTime", false);

		// Parse inventory system (new format with full inventory and blueprints)
		inventoryCapacity = safeParseInt("capacity", 10);
		selectedSlot = safeParseInt("selectedSlot", 0);

		// Parse inventory slots
		inventorySlots.clear();
		std::string slotsStr = findValue("slots");
		if (!slotsStr.empty() && slotsStr[0] == '[') {
			// Parse slot array
			size_t pos = 1; // skip '['
			while (pos < slotsStr.length()) {
				// Find blockID
				size_t blockIDPos = slotsStr.find("\"blockID\":", pos);
				if (blockIDPos == std::string::npos) break;
				blockIDPos += 10; // skip "blockID":

				// Parse blockID value
				uint16_t blockID = 0;
				std::stringstream ss1(slotsStr.substr(blockIDPos));
				ss1 >> blockID;

				// Find stackCount
				size_t stackPos = slotsStr.find("\"stackCount\":", blockIDPos);
				if (stackPos == std::string::npos) break;
				stackPos += 13; // skip "stackCount":

				// Parse stackCount value
				int stackCount = 0;
				std::stringstream ss2(slotsStr.substr(stackPos));
				ss2 >> stackCount;

				InventorySlotData slot;
				slot.blockID = blockID;
				slot.stackCount = stackCount;
				inventorySlots.push_back(slot);

				// Move to next slot
				pos = slotsStr.find("},", stackPos);
				if (pos == std::string::npos) break;
				pos += 2;
			}
		}

		// Backward compatibility: try loading old hotbarSlots format
		if (inventorySlots.empty()) {
			std::string hotbarStr = findValue("hotbarSlots");
			if (!hotbarStr.empty()) {
				try {
					std::stringstream ss(hotbarStr);
					char ch;
					ss >> ch; // skip '['

					uint16_t blockID;
					while (ss >> blockID) {
						InventorySlotData slot;
						slot.blockID = blockID;
						slot.stackCount = (blockID != 0) ? 1 : 0;
						inventorySlots.push_back(slot);
						ss >> ch; // skip ',' or ']'
						if (ch == ']') break;
					}
				} catch (...) {
					inventorySlots.clear();
				}
			}
		}

		// Parse unlocked blueprints
		unlockedBlueprints.clear();
		std::string blueprintsStr = findValue("unlockedBlueprints");
		if (!blueprintsStr.empty() && blueprintsStr[0] == '[') {
			try {
				std::stringstream ss(blueprintsStr);
				char ch;
				ss >> ch; // skip '['

				uint16_t blockID;
				while (ss >> blockID) {
					unlockedBlueprints.push_back(blockID);
					ss >> ch; // skip ',' or ']'
					if (ch == ']') break;
				}
			} catch (...) {
				unlockedBlueprints.clear();
			}
		}

		return true;
	}
	catch (...) {
		// If parsing fails completely, return false
		return false;
	}
}

bool WorldMetadata::saveToFile(const std::string& filepath) const
{
	std::ofstream file(filepath);
	if (!file.is_open())
		return false;

	file << toJSON();
	file.close();
	return true;
}

bool WorldMetadata::loadFromFile(const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open())
		return false;

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	return fromJSON(buffer.str());
}

} // namespace Persistence
