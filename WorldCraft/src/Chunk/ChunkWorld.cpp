#include <Chunk/ChunkWorld.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace Chunk
{

// ---------------------------------------------------------------------------
// ChunkEntry
// ---------------------------------------------------------------------------

void ChunkEntry::upload()
{
	auto uploadVerts = [](const std::vector<Meshing::ChunkVertex>& verts,
						  GLuint& vao, GLuint& vbo)
	{
		if (verts.empty()) return;
		if ((verts.size() % 3) != 0)
		{
			std::cerr << "Chunk upload rejected: non-triangle vertex count " << verts.size() << "\n";
			return;
		}

		// Pre-allocate VAO/VBO if needed
		if (vao == 0) glGenVertexArrays(1, &vao);
		if (vbo == 0) glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		// Use GL_STREAM_DRAW for chunks that may be regenerated frequently
		// This hints to the driver to use faster memory that's optimized for one-time use
		glBufferData(GL_ARRAY_BUFFER,
					 static_cast<GLsizeiptr>(verts.size() * sizeof(Meshing::ChunkVertex)),
					 verts.data(), GL_STREAM_DRAW);

		const GLsizei stride = sizeof(Meshing::ChunkVertex);
		using CV = Meshing::ChunkVertex;

		// Set up vertex attributes - only once per VAO
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(CV, x)));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(CV, u)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(CV, texLayer)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(CV, light)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(CV, faceType)));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(CV, skyLight)));
		glEnableVertexAttribArray(5);
	};

	// Upload all 5 LOD levels separately
	uploadVerts(lodMesh.lod0.opaque,      lod0OpaqueVAO, lod0OpaqueVBO);
	uploadVerts(lodMesh.lod0.transparent, lod0TranspVAO, lod0TranspVBO);
	uploadVerts(lodMesh.lod1.opaque,      lod1OpaqueVAO, lod1OpaqueVBO);
	uploadVerts(lodMesh.lod1.transparent, lod1TranspVAO, lod1TranspVBO);
	uploadVerts(lodMesh.lod2.opaque,      lod2OpaqueVAO, lod2OpaqueVBO);
	uploadVerts(lodMesh.lod2.transparent, lod2TranspVAO, lod2TranspVBO);
	uploadVerts(lodMesh.lod3.opaque,      lod3OpaqueVAO, lod3OpaqueVBO);
	uploadVerts(lodMesh.lod3.transparent, lod3TranspVAO, lod3TranspVBO);
	uploadVerts(lodMesh.lod4.opaque,      lod4OpaqueVAO, lod4OpaqueVBO);
	uploadVerts(lodMesh.lod4.transparent, lod4TranspVAO, lod4TranspVBO);

	// CRITICAL: Save vertex counts BEFORE clearing mesh data
	// These counts are needed for rendering; once on GPU, we don't need CPU-side mesh anymore
	lod0OpaqueCount = lodMesh.lod0.opaqueCount();
	lod0TranspCount = lodMesh.lod0.transparentCount();
	lod1OpaqueCount = lodMesh.lod1.opaqueCount();
	lod1TranspCount = lodMesh.lod1.transparentCount();
	lod2OpaqueCount = lodMesh.lod2.opaqueCount();
	lod2TranspCount = lodMesh.lod2.transparentCount();
	lod3OpaqueCount = lodMesh.lod3.opaqueCount();
	lod3TranspCount = lodMesh.lod3.transparentCount();
	lod4OpaqueCount = lodMesh.lod4.opaqueCount();
	lod4TranspCount = lodMesh.lod4.transparentCount();

	// CRITICAL: Free CPU-side mesh data after GPU upload to prevent memory leak!
	// The vertex data is now on the GPU; keeping it in RAM wastes hundreds of KB per chunk
	// At 3000 chunks with 5 LODs each, this can waste 2-3GB of RAM
	lodMesh.lod0.opaque.clear();      lodMesh.lod0.opaque.shrink_to_fit();
	lodMesh.lod0.transparent.clear(); lodMesh.lod0.transparent.shrink_to_fit();
	lodMesh.lod1.opaque.clear();      lodMesh.lod1.opaque.shrink_to_fit();
	lodMesh.lod1.transparent.clear(); lodMesh.lod1.transparent.shrink_to_fit();
	lodMesh.lod2.opaque.clear();      lodMesh.lod2.opaque.shrink_to_fit();
	lodMesh.lod2.transparent.clear(); lodMesh.lod2.transparent.shrink_to_fit();
	lodMesh.lod3.opaque.clear();      lodMesh.lod3.opaque.shrink_to_fit();
	lodMesh.lod3.transparent.clear(); lodMesh.lod3.transparent.shrink_to_fit();
	lodMesh.lod4.opaque.clear();      lodMesh.lod4.opaque.shrink_to_fit();
	lodMesh.lod4.transparent.clear(); lodMesh.lod4.transparent.shrink_to_fit();

	// Unbind VAO to prevent accidental modifications
	glBindVertexArray(0);
}

void ChunkEntry::destroy()
{
	// Delete all LOD level buffers
	if (lod0OpaqueVAO) { glDeleteVertexArrays(1, &lod0OpaqueVAO); lod0OpaqueVAO = 0; }
	if (lod0OpaqueVBO) { glDeleteBuffers(1, &lod0OpaqueVBO);      lod0OpaqueVBO = 0; }
	if (lod0TranspVAO) { glDeleteVertexArrays(1, &lod0TranspVAO); lod0TranspVAO = 0; }
	if (lod0TranspVBO) { glDeleteBuffers(1, &lod0TranspVBO);      lod0TranspVBO = 0; }

	if (lod1OpaqueVAO) { glDeleteVertexArrays(1, &lod1OpaqueVAO); lod1OpaqueVAO = 0; }
	if (lod1OpaqueVBO) { glDeleteBuffers(1, &lod1OpaqueVBO);      lod1OpaqueVBO = 0; }
	if (lod1TranspVAO) { glDeleteVertexArrays(1, &lod1TranspVAO); lod1TranspVAO = 0; }
	if (lod1TranspVBO) { glDeleteBuffers(1, &lod1TranspVBO);      lod1TranspVBO = 0; }

	if (lod2OpaqueVAO) { glDeleteVertexArrays(1, &lod2OpaqueVAO); lod2OpaqueVAO = 0; }
	if (lod2OpaqueVBO) { glDeleteBuffers(1, &lod2OpaqueVBO);      lod2OpaqueVBO = 0; }
	if (lod2TranspVAO) { glDeleteVertexArrays(1, &lod2TranspVAO); lod2TranspVAO = 0; }
	if (lod2TranspVBO) { glDeleteBuffers(1, &lod2TranspVBO);      lod2TranspVBO = 0; }

	if (lod3OpaqueVAO) { glDeleteVertexArrays(1, &lod3OpaqueVAO); lod3OpaqueVAO = 0; }
	if (lod3OpaqueVBO) { glDeleteBuffers(1, &lod3OpaqueVBO);      lod3OpaqueVBO = 0; }
	if (lod3TranspVAO) { glDeleteVertexArrays(1, &lod3TranspVAO); lod3TranspVAO = 0; }
	if (lod3TranspVBO) { glDeleteBuffers(1, &lod3TranspVBO);      lod3TranspVBO = 0; }

	if (lod4OpaqueVAO) { glDeleteVertexArrays(1, &lod4OpaqueVAO); lod4OpaqueVAO = 0; }
	if (lod4OpaqueVBO) { glDeleteBuffers(1, &lod4OpaqueVBO);      lod4OpaqueVBO = 0; }
	if (lod4TranspVAO) { glDeleteVertexArrays(1, &lod4TranspVAO); lod4TranspVAO = 0; }
	if (lod4TranspVBO) { glDeleteBuffers(1, &lod4TranspVBO);      lod4TranspVBO = 0; }
}

// ---------------------------------------------------------------------------
// ChunkWorld
// ---------------------------------------------------------------------------

ChunkWorld::ChunkWorld(const WorldGen::WorldSettings& settings)
	: m_settings(settings)
	, m_renderDist(settings.renderDistance)
{
	const unsigned int hw = std::thread::hardware_concurrency();
	// Leave more cores free for main thread and GPU work
	// Use at most 75% of available threads, minimum 2
	const unsigned int maxWorkers = std::max(2u, (hw * 3u) / 4u);
	const unsigned int numWorkers = std::min(maxWorkers, hw > 1 ? hw - 1 : 1);
	for (unsigned int i = 0; i < numWorkers; ++i)
		m_workers.emplace_back(&ChunkWorld::workerLoop, this);
}

ChunkWorld::~ChunkWorld()
{
	// Signal workers to stop
	{
		std::lock_guard<std::mutex> lk(m_workMutex);
		m_stopWorkers = true;
	}
	
	// Wake all workers from work queue wait
	m_workCV.notify_all();
	
	// CRITICAL FIX: If workers are paused, wake them so they can exit cleanly
	{
		std::lock_guard<std::mutex> lk(m_pauseMutex);
		m_pauseRequested.store(false);
	}
	m_pauseCV.notify_all();
	
	// Now safely wait for all workers to finish
	for (auto& t : m_workers)
	{
		if (t.joinable())
			t.join();
	}
}

// ---------------------------------------------------------------------------
// pauseWorkers / resumeWorkers
// ---------------------------------------------------------------------------

void ChunkWorld::pauseWorkers()
{
	// Signal all workers to pause at their next checkpoint.
	m_pauseRequested.store(true);
	// Wake any workers that are sleeping in m_workCV so they notice the flag.
	m_workCV.notify_all();

	// Block until every worker has reached its pause checkpoint.
	std::unique_lock<std::mutex> lk(m_pauseMutex);
	m_allPausedCV.wait(lk, [this] {
		return m_pausedWorkerCount == static_cast<int>(m_workers.size());
	});
}

void ChunkWorld::resumeWorkers()
{
	{
		std::lock_guard<std::mutex> lk(m_pauseMutex);
		m_pauseRequested.store(false);
	}
	m_pauseCV.notify_all();
}

void ChunkWorld::rebuildGpuResources()
{
	for (auto& [coord, entry] : m_chunks)
	{
		// Reset all LOD VAOs/VBOs
		entry->lod0OpaqueVAO = 0;
		entry->lod0OpaqueVBO = 0;
		entry->lod0TranspVAO = 0;
		entry->lod0TranspVBO = 0;
		entry->lod1OpaqueVAO = 0;
		entry->lod1OpaqueVBO = 0;
		entry->lod1TranspVAO = 0;
		entry->lod1TranspVBO = 0;
		entry->lod2OpaqueVAO = 0;
		entry->lod2OpaqueVBO = 0;
		entry->lod2TranspVAO = 0;
		entry->lod2TranspVBO = 0;
		entry->lod3OpaqueVAO = 0;
		entry->lod3OpaqueVBO = 0;
		entry->lod3TranspVAO = 0;
		entry->lod3TranspVBO = 0;
		entry->lod4OpaqueVAO = 0;
		entry->lod4OpaqueVBO = 0;
		entry->lod4TranspVAO = 0;
		entry->lod4TranspVBO = 0;
		entry->upload();
	}
}

void ChunkWorld::releaseGpuResources()
{
	for (auto& [coord, entry] : m_chunks)
		entry->destroy();
}

// ---------------------------------------------------------------------------
// Worker loop — handles Generate and Mesh phases
// ---------------------------------------------------------------------------

void ChunkWorld::workerLoop()
{
	WorldGen::TerrainGen terrainGen(m_settings);

	while (true)
	{
		WorkItem item{};

		// --- Pause checkpoint ---
		{
			std::unique_lock<std::mutex> plk(m_pauseMutex);
			if (m_pauseRequested.load())
			{
				++m_pausedWorkerCount;
				m_allPausedCV.notify_one();
				m_pauseCV.wait(plk, [this] {
					return !m_pauseRequested.load() || m_stopWorkers.load();
				});
				--m_pausedWorkerCount;
				if (m_stopWorkers.load()) return;
			}
		}

		// Acquire work item from queue
		{
			std::unique_lock<std::mutex> lk(m_workMutex);
			m_workCV.wait(lk, [this] {
				return m_stopWorkers.load() || !m_workQueue.empty() || m_pauseRequested.load();
			});
			
			if (m_stopWorkers && m_workQueue.empty()) return;
			if (m_pauseRequested.load()) continue;
			
			if (m_workQueue.empty())
			{
				continue;
			}
			
			item = m_workQueue.front();
			m_workQueue.pop();
		} // ← CRITICAL: Lock released here before processing the work item!

		if (item.phase == WorkPhase::Generate)
		{
			auto genStart = std::chrono::high_resolution_clock::now();

			// Generate voxel data and store in the shared cache.
			auto chunkPtr = std::make_shared<Chunk>();
			terrainGen.generate(*chunkPtr, item.coord.x, item.coord.z);

			auto genEnd = std::chrono::high_resolution_clock::now();
			auto genDuration = std::chrono::duration_cast<std::chrono::milliseconds>(genEnd - genStart).count();

			{
				std::unique_lock<std::shared_mutex> lk(m_voxelMutex);
				m_voxelCache[item.coord] = chunkPtr;
			}

			// Remove from in-flight BEFORE trying to enqueue mesh jobs
			{
				std::lock_guard<std::mutex> lk(m_inFlightMutex);
				m_inFlight.erase(item.coord);
			}

			// Try to start mesh jobs for this chunk and its four neighbours
			const ChunkCoord neighbours[5] = {
				item.coord,
				{ item.coord.x + 1, item.coord.z },
				{ item.coord.x - 1, item.coord.z },
				{ item.coord.x,     item.coord.z + 1 },
				{ item.coord.x,     item.coord.z - 1 },
			};
			for (const auto& nc : neighbours)
				tryEnqueueMesh(nc);
		}
		else // WorkPhase::Mesh
		{
			// Gather chunk + neighbours from the voxel cache.
			std::shared_ptr<Chunk> self;
			Meshing::ChunkNeighbours nbrs;

			{
				std::shared_lock<std::shared_mutex> lk(m_voxelMutex);

				auto it = m_voxelCache.find(item.coord);
				if (it == m_voxelCache.end())
				{
					// Should not happen, but guard against it.
					std::lock_guard<std::mutex> il(m_inFlightMutex);
					m_inFlight.erase(item.coord);
					continue;
				}
				self = it->second;

				auto find = [&](ChunkCoord c) -> std::shared_ptr<const Chunk> {
					auto jt = m_voxelCache.find(c);
					return (jt != m_voxelCache.end()) ? jt->second : nullptr;
				};

				nbrs.posX = find({ item.coord.x + 1, item.coord.z });
				nbrs.negX = find({ item.coord.x - 1, item.coord.z });
				nbrs.posZ = find({ item.coord.x,     item.coord.z + 1 });
				nbrs.negZ = find({ item.coord.x,     item.coord.z - 1 });
			}

			auto entry = std::make_unique<ChunkEntry>();
			entry->coord = item.coord;
			entry->chunk = *self;
			entry->worldOffset = glm::vec3(
				static_cast<float>(item.coord.x * CHUNK_SIZE_X),
				0.0f,
				static_cast<float>(item.coord.z * CHUNK_SIZE_Z));
			entry->lodMesh = Meshing::ChunkMesher::buildLOD(*self, nbrs);

			{
				std::lock_guard<std::mutex> lk(m_readyMutex);
				m_readyQueue.push(std::move(entry));
			}

			{
				std::lock_guard<std::mutex> lk(m_pendingUploadMutex);
				m_pendingUploadSet.insert(item.coord);
			}

			{
				std::lock_guard<std::mutex> lk(m_inFlightMutex);
				m_inFlight.erase(item.coord);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

ChunkCoord ChunkWorld::toChunkCoord(const glm::vec3& worldPos) const
{
return { static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE_X)),
 static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE_Z)) };
}

void ChunkWorld::enqueueGenerate(const ChunkCoord& coord)
{
	// Main-thread only. Already uploaded?
	if (m_chunks.count(coord)) return;

	// Already in-flight for any phase?
	{
		std::lock_guard<std::mutex> lk(m_inFlightMutex);
		if (m_inFlight.count(coord)) return;
	}

	// Does voxel data already exist in the cache?
	bool hasCachedVoxels = false;
	{
		std::shared_lock<std::shared_mutex> lk(m_voxelMutex);
		hasCachedVoxels = m_voxelCache.count(coord) > 0;
	}

	if (hasCachedVoxels)
	{
		// Skip generate — try to push a mesh job if neighbours are ready.
		tryEnqueueMesh(coord);
	}
	else
	{
		// CRITICAL FIX: Don't enqueue if work queue is getting too large
		// This prevents the queue from being flooded with Generate jobs
		// and allows Mesh jobs to be processed
		size_t queueSize = 0;
		{
			std::lock_guard<std::mutex> wl(m_workMutex);
			queueSize = m_workQueue.size();
		}
		
		// If queue has more than 50 items, skip for now
		if (queueSize > 50)
		{
			return;
		}
		
		// Mark as in-flight and enqueue the work
		{
			std::lock_guard<std::mutex> lk(m_inFlightMutex);
			if (m_inFlight.count(coord)) return;  // re-check
			m_inFlight.insert(coord);
		}

		{
			std::lock_guard<std::mutex> wl(m_workMutex);
			m_workQueue.push({ coord, WorkPhase::Generate });
			m_workCV.notify_one();
		}
	}
}

void ChunkWorld::tryEnqueueMesh(const ChunkCoord& coord)
{
	// Guard: already uploaded? (called from both main thread and workers)
	{
		std::lock_guard<std::mutex> lk(m_uploadedMutex);
		if (m_uploadedSet.count(coord)) return;
	}

	// Guard: finished mesh already waiting for main-thread upload?
	{
		std::lock_guard<std::mutex> lk(m_pendingUploadMutex);
		if (m_pendingUploadSet.count(coord)) return;
	}

	// All five voxel chunks (self + 4 neighbours) must exist.
	bool hasAllNeighbours = false;
	{
		std::shared_lock<std::shared_mutex> lk(m_voxelMutex);
		hasAllNeighbours = m_voxelCache.count(coord) > 0 &&
		                   m_voxelCache.count({ coord.x + 1, coord.z }) > 0 &&
		                   m_voxelCache.count({ coord.x - 1, coord.z }) > 0 &&
		                   m_voxelCache.count({ coord.x,     coord.z + 1 }) > 0 &&
		                   m_voxelCache.count({ coord.x,     coord.z - 1 }) > 0;
	}
	
	if (!hasAllNeighbours)
	{
		return;
	}

	// Avoid duplicate mesh jobs.
	{
		std::lock_guard<std::mutex> lk(m_inFlightMutex);
		if (m_inFlight.count(coord)) return;
		m_inFlight.insert(coord);
	}


	{
		std::lock_guard<std::mutex> lk(m_workMutex);
		m_workQueue.push({ coord, WorkPhase::Mesh });
	}
	m_workCV.notify_one();
}

void ChunkWorld::drainReadyQueue()
{
	// DIAGNOSTIC: Print ready queue size occasionally
	static int drainCallCount = 0;
	bool shouldPrint = (++drainCallCount % 60 == 0);
	
	size_t queueSize = 0;
	{
		std::lock_guard<std::mutex> lk(m_readyMutex);
		queueSize = m_readyQueue.size();
	}
	
	// Limit both the number of uploads and the total time spent per frame.
	constexpr int kMaxUploadsPerFrame = 6;
	constexpr double kMaxUploadTimeMs = 3.0;

	int uploaded = 0;
	auto startTime = std::chrono::high_resolution_clock::now();

	while (uploaded < kMaxUploadsPerFrame)
	{
		// Check time budget before attempting another upload
		if (uploaded > 0)
		{
			auto now = std::chrono::high_resolution_clock::now();
			double elapsedMs = std::chrono::duration<double, std::milli>(now - startTime).count();
			if (elapsedMs > kMaxUploadTimeMs)
				break;
		}

		std::unique_ptr<ChunkEntry> entry;
		{
			std::lock_guard<std::mutex> lk(m_readyMutex);
			if (m_readyQueue.empty()) break;

			entry = std::move(m_readyQueue.front());
			m_readyQueue.pop();
		}

		const ChunkCoord c = entry->coord;
		const int dx = std::abs(c.x - m_lastCamChunk.x);
		const int dz = std::abs(c.z - m_lastCamChunk.z);

		{
			std::lock_guard<std::mutex> lk(m_pendingUploadMutex);
			m_pendingUploadSet.erase(c);
		}

		// Drop stale ready results for chunks that are no longer near the camera.
		if (dx > m_renderDist + 1 || dz > m_renderDist + 1)
		{
			continue;
		}

		// If this chunk already exists, destroy old and replace
		auto existingIt = m_chunks.find(c);
		if (existingIt != m_chunks.end())
		{
			existingIt->second->destroy();
			m_chunks.erase(existingIt);
		}

		entry->upload();
		m_chunks.emplace(c, std::move(entry));

		{
			std::lock_guard<std::mutex> lk(m_uploadedMutex);
			m_uploadedSet.insert(c);
		}
		++uploaded;
	}
}

void ChunkWorld::evictDistant(const ChunkCoord& camChunk)
{
	// Memory leak is fixed (mesh data cleared after upload), so we can use gentler eviction
	// Keep a small buffer beyond render distance for smoother streaming
	const int evictDist = m_renderDist + 1;  // Small 1-chunk buffer

	// Moderate eviction rate - enough to clean up but not overly aggressive
	// Since mesh memory is freed, we don't need such high rates anymore
	constexpr int kMaxEvictionsPerFrame = 12;  // Reduced from 200
	int evicted = 0;

	for (auto it = m_chunks.begin(); it != m_chunks.end() && evicted < kMaxEvictionsPerFrame; )
	{
		const int dx = std::abs(it->first.x - camChunk.x);
		const int dz = std::abs(it->first.z - camChunk.z);
		if (dx > evictDist || dz > evictDist)
		{
			const ChunkCoord c = it->first;
			it->second->destroy();
			it = m_chunks.erase(it);
			{
				std::lock_guard<std::mutex> lk(m_uploadedMutex);
				m_uploadedSet.erase(c);
			}
			{
				std::lock_guard<std::mutex> lk(m_pendingUploadMutex);
				m_pendingUploadSet.erase(c);
			}
			++evicted;
		}
		else ++it;
	}

	// Moderate cache eviction
	constexpr int kMaxCacheEvictionsPerFrame = 24;  // Reduced from 400
	int cacheEvicted = 0;
	std::unique_lock<std::shared_mutex> lk(m_voxelMutex);
	for (auto it = m_voxelCache.begin(); it != m_voxelCache.end() && cacheEvicted < kMaxCacheEvictionsPerFrame; )
	{
		const int dx = std::abs(it->first.x - camChunk.x);
		const int dz = std::abs(it->first.z - camChunk.z);
		if (dx > evictDist || dz > evictDist)
		{
			it = m_voxelCache.erase(it);
			++cacheEvicted;
		}
		else
			++it;
	}
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

void ChunkWorld::update(const glm::vec3& cameraPos)
{
	const ChunkCoord camChunk = toChunkCoord(cameraPos);

	// ALWAYS try to enqueue chunks around the camera, not just when moving
	// This ensures chunks continue generating until the area is fully loaded
	{
		// DIAGNOSTIC: Print when we're about to generate chunks
		static bool printedOnce = false;
		if (!printedOnce)
		{
			printf("ChunkWorld::update - Camera at chunk (%d, %d), render distance: %d\n",
			       camChunk.x, camChunk.z, m_renderDist);
			printedOnce = true;
		}

		// Request one extra ring of generate jobs around the render distance so
		// every rendered chunk has all four neighbours ready for meshing.
		const int genDist = m_renderDist + 1;
		for (int dz = -genDist; dz <= genDist; ++dz)
			for (int dx = -genDist; dx <= genDist; ++dx)
				enqueueGenerate({ camChunk.x + dx, camChunk.z + dz });
	}

	// Update camera tracking only when it actually moves chunks
	if (!(camChunk == m_lastCamChunk))
	{
		printf("ChunkWorld::update - Camera moved to chunk (%d, %d)\n",
		       camChunk.x, camChunk.z);
		m_lastCamChunk = camChunk;
	}

	// CRITICAL: Run eviction EVERY FRAME, not just when changing chunks!
	// This prevents memory leaks when flying around or standing still
	evictDistant(camChunk);

	drainReadyQueue();
}

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------

// Simple 6-plane frustum extracted from the combined proj*view matrix.
// Tests an AABB against each half-space; returns false if fully outside any plane.
namespace
{
struct Plane { float nx, ny, nz, d; };

static void extractFrustum(const glm::mat4& m, Plane planes[6])
{
	// Columns of m (OpenGL column-major).
	auto row = [&](int r) -> glm::vec4 {
		return glm::vec4(m[0][r], m[1][r], m[2][r], m[3][r]);
	};
	glm::vec4 r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);
	// Left, Right, Bottom, Top, Near, Far
	auto store = [&](int i, glm::vec4 p) {
		float len = glm::sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
		planes[i] = { p.x/len, p.y/len, p.z/len, p.w/len };
	};
	store(0, r3 + r0);  // left
	store(1, r3 - r0);  // right
	store(2, r3 + r1);  // bottom
	store(3, r3 - r1);  // top
	store(4, r3 + r2);  // near
	store(5, r3 - r2);  // far
}

static bool aabbInFrustum(const Plane planes[6], glm::vec3 mn, glm::vec3 mx)
{
	for (int i = 0; i < 6; ++i)
	{
		const Plane& p = planes[i];
		// p-vertex (most positive along plane normal).
		float px = (p.nx >= 0.0f) ? mx.x : mn.x;
		float py = (p.ny >= 0.0f) ? mx.y : mn.y;
		float pz = (p.nz >= 0.0f) ? mx.z : mn.z;
		if (p.nx*px + p.ny*py + p.nz*pz + p.d < 0.0f)
			return false;
	}
	return true;
}
} // anonymous namespace

void ChunkWorld::render(GLint mvpLoc, GLint chunkOffsetLoc,
						const glm::vec3& camPos,
						const glm::mat4& view,
						const glm::mat4& proj) const
{
	const glm::mat4 vp = proj * view;
	Plane frustum[6];
	extractFrustum(vp, frustum);

	const glm::vec3 chunkSize(
		static_cast<float>(CHUNK_SIZE_X),
		static_cast<float>(CHUNK_SIZE_Y),
		static_cast<float>(CHUNK_SIZE_Z));

	// LOD distance thresholds - spread across 40-chunk view distance
	constexpr float kLOD0Distance = 16.0f * 16.0f;  // ~256 blocks - full detail (same as before)
	constexpr float kLOD1Distance = 25.0f * 16.0f;  // ~400 blocks - step=2 (subtle)
	constexpr float kLOD2Distance = 35.0f * 16.0f;  // ~560 blocks - step=3 (further out)
	constexpr float kLOD3Distance = 45.0f * 16.0f;  // ~720 blocks - step=4 (very far)
	// LOD4 used beyond kLOD3Distance - step=6 for distant horizon (beyond 720 blocks)

	// Helper to select LOD based on distance
	auto selectLOD = [&](const ChunkEntry* entry, GLuint& vao, int& count, bool transparent) -> bool
	{
		const glm::vec3 chunkCenter = entry->worldOffset + glm::vec3(CHUNK_SIZE_X * 0.5f, 0.0f, CHUNK_SIZE_Z * 0.5f);
		const float distSq = glm::dot(chunkCenter - camPos, chunkCenter - camPos);

		if (distSq < kLOD0Distance * kLOD0Distance)
		{
			// LOD0 - full detail
			vao = transparent ? entry->lod0TranspVAO : entry->lod0OpaqueVAO;
			count = transparent ? entry->lod0TranspCount : entry->lod0OpaqueCount;
		}
		else if (distSq < kLOD1Distance * kLOD1Distance)
		{
			// LOD1 - step=2
			vao = transparent ? entry->lod1TranspVAO : entry->lod1OpaqueVAO;
			count = transparent ? entry->lod1TranspCount : entry->lod1OpaqueCount;
		}
		else if (distSq < kLOD2Distance * kLOD2Distance)
		{
			// LOD2 - step=3
			vao = transparent ? entry->lod2TranspVAO : entry->lod2OpaqueVAO;
			count = transparent ? entry->lod2TranspCount : entry->lod2OpaqueCount;
		}
		else if (distSq < kLOD3Distance * kLOD3Distance)
		{
			// LOD3 - step=4
			vao = transparent ? entry->lod3TranspVAO : entry->lod3OpaqueVAO;
			count = transparent ? entry->lod3TranspCount : entry->lod3OpaqueCount;
		}
		else
		{
			// LOD4 - step=6 (very distant)
			vao = transparent ? entry->lod4TranspVAO : entry->lod4OpaqueVAO;
			count = transparent ? entry->lod4TranspCount : entry->lod4OpaqueCount;
		}

		return count > 0 && vao != 0;
	};

	// --- Opaque pass ---------------------------------------------------------
	for (const auto& [coord, entry] : m_chunks)
	{
		const glm::vec3& off = entry->worldOffset;
		if (!aabbInFrustum(frustum, off, off + chunkSize)) continue;

		GLuint vao = 0;
		int count = 0;
		if (!selectLOD(entry.get(), vao, count, false)) continue;

		const glm::mat4 mvp = vp * glm::translate(glm::mat4(1.0f), off);
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
		glUniform3f(chunkOffsetLoc, off.x, off.y, off.z);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, count);
	}
	glBindVertexArray(0);

	// --- Transparent pass — back-to-front ------------------------------------
	std::vector<const ChunkEntry*> transpChunks;
	transpChunks.reserve(m_chunks.size());
	for (const auto& [coord, entry] : m_chunks)
	{
		const glm::vec3& off = entry->worldOffset;
		if (!aabbInFrustum(frustum, off, off + chunkSize)) continue;

		GLuint dummyVao = 0;
		int count = 0;
		if (!selectLOD(entry.get(), dummyVao, count, true)) continue;

		transpChunks.push_back(entry.get());
	}

	std::sort(transpChunks.begin(), transpChunks.end(),
	[&camPos](const ChunkEntry* a, const ChunkEntry* b)
	{
		auto centre = [](const ChunkEntry* e) {
			return e->worldOffset + glm::vec3(CHUNK_SIZE_X * 0.5f, 0.0f, CHUNK_SIZE_Z * 0.5f);
		};
		const float da = glm::dot(centre(a) - camPos, centre(a) - camPos);
		const float db = glm::dot(centre(b) - camPos, centre(b) - camPos);
		return da > db;
	});

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	for (const ChunkEntry* entry : transpChunks)
	{
		const glm::vec3& off = entry->worldOffset;
		const glm::mat4 mvp  = vp * glm::translate(glm::mat4(1.0f), off);
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
		glUniform3f(chunkOffsetLoc, off.x, off.y, off.z);

		GLuint vao = 0;
		int count = 0;
		if (!selectLOD(entry, vao, count, true)) continue;

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, count);
	}
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

// ---------------------------------------------------------------------------
// Collision detection for character camera
// ---------------------------------------------------------------------------

bool ChunkWorld::isBlockSolid(float worldX, float worldY, float worldZ) const
{
	// Convert world position to chunk coordinates
	const int cx = static_cast<int>(std::floor(worldX / CHUNK_SIZE_X));
	const int cz = static_cast<int>(std::floor(worldZ / CHUNK_SIZE_Z));

	// Convert to local chunk coordinates
	const int lx = static_cast<int>(std::floor(worldX)) - (cx * CHUNK_SIZE_X);
	const int ly = static_cast<int>(std::floor(worldY));
	const int lz = static_cast<int>(std::floor(worldZ)) - (cz * CHUNK_SIZE_Z);

	// Bounds check
	if (ly < 0 || ly >= CHUNK_SIZE_Y) return false;  // Out of world bounds
	if (lx < 0 || lx >= CHUNK_SIZE_X) return false;
	if (lz < 0 || lz >= CHUNK_SIZE_Z) return false;

	// Look up the chunk
	ChunkCoord coord{ cx, cz };

	// First check the main chunk map (uploaded chunks)
	{
		auto it = m_chunks.find(coord);
		if (it != m_chunks.end())
		{
			Voxel::BlockID block = it->second->chunk.getBlock(lx, ly, lz);

			// Solid blocks block movement, air and water don't
			return block != Voxel::BlockID::Air && block != Voxel::BlockID::Water;
		}
	}

	// Fall back to voxel cache (chunks being generated/meshed)
	{
		std::shared_lock<std::shared_mutex> lock(m_voxelMutex);
		auto it = m_voxelCache.find(coord);
		if (it != m_voxelCache.end())
		{
			Voxel::BlockID block = it->second->getBlock(lx, ly, lz);
			return block != Voxel::BlockID::Air && block != Voxel::BlockID::Water;
		}
	}

	// Chunk not loaded yet - treat as solid to prevent falling through the world
	return true;
}

Voxel::BlockID ChunkWorld::getBlockAt(float worldX, float worldY, float worldZ) const
{
	// Convert world position to chunk coordinates
	const int cx = static_cast<int>(std::floor(worldX / CHUNK_SIZE_X));
	const int cz = static_cast<int>(std::floor(worldZ / CHUNK_SIZE_Z));

	// Convert to local chunk coordinates
	const int lx = static_cast<int>(std::floor(worldX)) - (cx * CHUNK_SIZE_X);
	const int ly = static_cast<int>(std::floor(worldY));
	const int lz = static_cast<int>(std::floor(worldZ)) - (cz * CHUNK_SIZE_Z);

	// Bounds check
	if (ly < 0 || ly >= CHUNK_SIZE_Y) return Voxel::BlockID::Air;
	if (lx < 0 || lx >= CHUNK_SIZE_X) return Voxel::BlockID::Air;
	if (lz < 0 || lz >= CHUNK_SIZE_Z) return Voxel::BlockID::Air;

	// Look up the chunk
	ChunkCoord coord{ cx, cz };

	// First check the main chunk map (uploaded chunks)
	{
		auto it = m_chunks.find(coord);
		if (it != m_chunks.end())
		{
			return it->second->chunk.getBlock(lx, ly, lz);
		}
	}

	// Fall back to voxel cache (chunks being generated/meshed)
	{
		std::shared_lock<std::shared_mutex> lock(m_voxelMutex);
		auto it = m_voxelCache.find(coord);
		if (it != m_voxelCache.end())
		{
			return it->second->getBlock(lx, ly, lz);
		}
	}

	// Chunk not loaded yet - return air
	return Voxel::BlockID::Air;
}

bool ChunkWorld::setBlockAt(float worldX, float worldY, float worldZ, Voxel::BlockID newBlock)
{
	// Convert world position to chunk coordinates
	const int cx = static_cast<int>(std::floor(worldX / CHUNK_SIZE_X));
	const int cz = static_cast<int>(std::floor(worldZ / CHUNK_SIZE_Z));

	// Convert to local chunk coordinates
	const int lx = static_cast<int>(std::floor(worldX)) - (cx * CHUNK_SIZE_X);
	const int ly = static_cast<int>(std::floor(worldY));
	const int lz = static_cast<int>(std::floor(worldZ)) - (cz * CHUNK_SIZE_Z);

	// Bounds check
	if (ly < 0 || ly >= CHUNK_SIZE_Y) return false;
	if (lx < 0 || lx >= CHUNK_SIZE_X) return false;
	if (lz < 0 || lz >= CHUNK_SIZE_Z) return false;

	ChunkCoord coord{ cx, cz };

	// Modify the block in the voxel cache (persistent chunk data)
	{
		std::unique_lock<std::shared_mutex> lock(m_voxelMutex);
		auto it = m_voxelCache.find(coord);
		if (it != m_voxelCache.end())
		{
			it->second->setBlock(lx, ly, lz, newBlock);
		}
		else
		{
			// Chunk not in voxel cache yet - can't modify
			return false;
		}
	}

	// Trigger a remesh of the modified chunk and its neighbors
	// Define neighbor coordinates
	const ChunkCoord neighbors[4] = {
		{ cx - 1, cz },      // West
		{ cx + 1, cz },      // East
		{ cx, cz - 1 },      // North
		{ cx, cz + 1 }       // South
	};

	// Remove modified chunk and all neighbors from tracking sets so they can be remeshed
	{
		std::lock_guard<std::mutex> lock(m_inFlightMutex);
		m_inFlight.erase(coord);
		for (const auto& nb : neighbors)
			m_inFlight.erase(nb);
	}
	{
		std::lock_guard<std::mutex> lock(m_uploadedMutex);
		m_uploadedSet.erase(coord);
		for (const auto& nb : neighbors)
			m_uploadedSet.erase(nb);
	}
	{
		std::lock_guard<std::mutex> lock(m_pendingUploadMutex);
		m_pendingUploadSet.erase(coord);
		for (const auto& nb : neighbors)
			m_pendingUploadSet.erase(nb);
	}

	// Also update the chunk entry if it's loaded, but don't destroy it yet
	// (it will be replaced when the new mesh is uploaded)
	{
		auto it = m_chunks.find(coord);
		if (it != m_chunks.end())
		{
			it->second->chunk.setBlock(lx, ly, lz, newBlock);
		}
	}

	// Enqueue mesh jobs for modified chunk and all neighbors
	tryEnqueueMesh(coord);
	for (const auto& nb : neighbors)
		tryEnqueueMesh(nb);

	return true;
}

Chunk* ChunkWorld::getChunk(int chunkX, int chunkZ)
{
	ChunkCoord coord{ chunkX, chunkZ };

	// First check the main chunk map (uploaded chunks)
	auto it = m_chunks.find(coord);
	if (it != m_chunks.end())
	{
		return &it->second->chunk;
	}

	// Fall back to voxel cache (chunks being generated/meshed)
	std::shared_lock<std::shared_mutex> lock(m_voxelMutex);
	auto cacheIt = m_voxelCache.find(coord);
	if (cacheIt != m_voxelCache.end())
	{
		return cacheIt->second.get();
	}

	return nullptr;
}

const Chunk* ChunkWorld::getChunk(int chunkX, int chunkZ) const
{
	ChunkCoord coord{ chunkX, chunkZ };

	// First check the main chunk map (uploaded chunks)
	auto it = m_chunks.find(coord);
	if (it != m_chunks.end())
	{
		return &it->second->chunk;
	}

	// Fall back to voxel cache (chunks being generated/meshed)
	std::shared_lock<std::shared_mutex> lock(m_voxelMutex);
	auto cacheIt = m_voxelCache.find(coord);
	if (cacheIt != m_voxelCache.end())
	{
		return cacheIt->second.get();
	}

	return nullptr;
}

} // namespace Chunk
