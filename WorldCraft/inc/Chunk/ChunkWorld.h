#pragma once

#include <WorldCraft.h>
#include <Chunk/Chunk.h>
#include <Meshing/ChunkMesher.h>
#include <WorldGen/TerrainGen.h>
#include <WorldGen/WorldSettings.h>
#include <Persistence/WorldPersistence.h>

#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cstdint>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>

namespace Chunk
{

struct ChunkCoord
{
int x = 0;
int z = 0;
bool operator==(const ChunkCoord& o) const { return x == o.x && z == o.z; }
};

struct ChunkCoordHash
{
std::size_t operator()(const ChunkCoord& c) const noexcept
{
auto h1 = std::hash<int>{}(c.x);
auto h2 = std::hash<int>{}(c.z);
return h1 ^ (h2 * 0x9e3779b97f4a7c15ULL + 0x6c62272e07bb0142ULL + (h1 << 6) + (h1 >> 2));
}
};

struct ChunkEntry
{
	ChunkCoord           coord;
	Chunk                chunk;
	Meshing::ChunkMeshLOD lodMesh;

	// LOD0 (full detail)
	GLuint lod0OpaqueVAO  = 0;
	GLuint lod0OpaqueVBO  = 0;
	GLuint lod0TranspVAO  = 0;
	GLuint lod0TranspVBO  = 0;
	int    lod0OpaqueCount = 0;
	int    lod0TranspCount = 0;

	// LOD1 (step=2)
	GLuint lod1OpaqueVAO  = 0;
	GLuint lod1OpaqueVBO  = 0;
	GLuint lod1TranspVAO  = 0;
	GLuint lod1TranspVBO  = 0;
	int    lod1OpaqueCount = 0;
	int    lod1TranspCount = 0;

	// LOD2 (step=3)
	GLuint lod2OpaqueVAO  = 0;
	GLuint lod2OpaqueVBO  = 0;
	GLuint lod2TranspVAO  = 0;
	GLuint lod2TranspVBO  = 0;
	int    lod2OpaqueCount = 0;
	int    lod2TranspCount = 0;

	// LOD3 (step=4)
	GLuint lod3OpaqueVAO  = 0;
	GLuint lod3OpaqueVBO  = 0;
	GLuint lod3TranspVAO  = 0;
	GLuint lod3TranspVBO  = 0;
	int    lod3OpaqueCount = 0;
	int    lod3TranspCount = 0;

	// LOD4 (step=6)
	GLuint lod4OpaqueVAO  = 0;
	GLuint lod4OpaqueVBO  = 0;
	GLuint lod4TranspVAO  = 0;
	GLuint lod4TranspVBO  = 0;
	int    lod4OpaqueCount = 0;
	int    lod4TranspCount = 0;

	glm::vec3 worldOffset{ 0.0f };

	void upload();
	void destroy();
};

enum class WorkPhase { Generate, Mesh };

struct WorkItem
{
ChunkCoord coord;
WorkPhase  phase = WorkPhase::Generate;
};

class ChunkWorld
{
public:
	explicit ChunkWorld(const WorldGen::WorldSettings& settings, Persistence::WorldPersistence* persistence = nullptr);
	~ChunkWorld();

void update(const glm::vec3& cameraPos);
	void render(GLint mvpLoc, GLint chunkOffsetLoc,
			const glm::vec3& camPos,
			const glm::mat4& view,
			const glm::mat4& proj) const;

	// Pause all worker threads (blocks until every worker is idle), and
	// resume them.  Intended to bracket operations like a fullscreen switch
	// that must not race with background meshing.
	void pauseWorkers();
	void resumeWorkers();

	// Recreate all GPU buffers/VAOs from CPU-side mesh data after an OpenGL
	// context rebuild.
	void rebuildGpuResources();

	// Release all current GPU buffers/VAOs before destroying the active
	// OpenGL context.
	void releaseGpuResources();

	// Collision detection for character camera: check if a world-space position
	// contains a solid (non-air, non-water) block that blocks movement.
	bool isBlockSolid(float worldX, float worldY, float worldZ) const;

	// Get the block at a world-space position (for underwater effect checking)
	Voxel::BlockID getBlockAt(float worldX, float worldY, float worldZ) const;

	// Set a block at a world-space position and trigger chunk remesh.
	// Returns true if the block was successfully modified, false if out of bounds.
	bool setBlockAt(float worldX, float worldY, float worldZ, Voxel::BlockID newBlock);

	// Access chunk for light propagation (returns nullptr if not loaded)
	Chunk* getChunk(int chunkX, int chunkZ);
	const Chunk* getChunk(int chunkX, int chunkZ) const;

	// Check initial loading status - returns percentage loaded (0.0 to 1.0)
	float getLoadingProgress() const;
	bool isInitialLoadComplete() const;
	int getLoadedChunkCount() const { return static_cast<int>(m_chunks.size()); }
	int getTargetChunkCount() const;

	// Persistence operations
	// Save all modified chunks to disk
	int saveModifiedChunks();

	// Get number of modified chunks waiting to be saved
	int getModifiedChunkCount() const { return static_cast<int>(m_modifiedChunks.size()); }

private:
	WorldGen::WorldSettings m_settings;
	int  m_renderDist;

using ChunkMap = std::unordered_map<ChunkCoord, std::unique_ptr<ChunkEntry>, ChunkCoordHash>;
ChunkMap   m_chunks;
ChunkCoord m_lastCamChunk{ INT_MIN, INT_MIN };

using VoxelCache = std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoordHash>;
VoxelCache              m_voxelCache;
mutable std::shared_mutex m_voxelMutex;

std::queue<WorkItem>                                   m_workQueue;
std::mutex                                             m_workMutex;
std::condition_variable                                m_workCV;

std::queue<std::unique_ptr<ChunkEntry>>                m_readyQueue;
std::mutex                                             m_readyMutex;

// Coords currently enqueued or being worked on (either phase).
std::unordered_set<ChunkCoord, ChunkCoordHash>         m_inFlight;
std::mutex                                             m_inFlightMutex;

// Coords whose mesh has been uploaded to the GPU — safe to read from workers.
std::unordered_set<ChunkCoord, ChunkCoordHash>         m_uploadedSet;
std::mutex                                             m_uploadedMutex;

// Coords with a finished mesh waiting in m_readyQueue for main-thread upload.
std::unordered_set<ChunkCoord, ChunkCoordHash>         m_pendingUploadSet;
std::mutex                                             m_pendingUploadMutex;

std::vector<std::thread>  m_workers;
	std::atomic<bool>         m_stopWorkers{ false };

	// Pause/resume — lets the main thread quiesce all workers (e.g. before a
	// fullscreen switch) without tearing down the thread pool.
	std::atomic<bool>       m_pauseRequested{ false };
	std::mutex              m_pauseMutex;
	std::condition_variable m_pauseCV;      // workers block here while paused
	std::condition_variable m_allPausedCV;  // main thread waits here until all paused
	int                     m_pausedWorkerCount{ 0 }; // guarded by m_pauseMutex

	// Persistence system
	Persistence::WorldPersistence* m_persistence;  // Not owned - managed by WorldCraft.cpp

	// Track which chunks have been modified by the player
	std::unordered_set<ChunkCoord, ChunkCoordHash> m_modifiedChunks;
	std::mutex m_modifiedChunksMutex;

ChunkCoord toChunkCoord(const glm::vec3& worldPos) const;
void       enqueueGenerate(const ChunkCoord& coord);
void       tryEnqueueMesh(const ChunkCoord& coord);
	void       drainReadyQueue();
	void       evictDistant(const ChunkCoord& camChunk);
	void       workerLoop();
};

} // namespace Chunk
