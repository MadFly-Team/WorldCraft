#pragma once

#include <WorldCraft.h>
#include <Chunk/Chunk.h>
#include <Meshing/VoxelVertex.h>
#include <Voxel/BlockTypes.h>
#include <memory>
#include <vector>

namespace Meshing
{

	// ---------------------------------------------------------------------------
	// ChunkMesh — two vertex lists: opaque faces and transparent faces.
	// Upload uploads both to the GPU; each list has its own VAO/VBO pair.
	// ---------------------------------------------------------------------------
	struct ChunkMesh
	{
		std::vector<ChunkVertex> opaque;       // solid blocks
		std::vector<ChunkVertex> transparent;  // water, leaves

		// Upload / re-upload vertex data to the GPU.
		// Pass valid references; buffers are created on first call.
		void uploadOpaque     (GLuint& vao, GLuint& vbo) const;
		void uploadTransparent(GLuint& vao, GLuint& vbo) const;

		int opaqueCount()      const { return static_cast<int>(opaque.size());      }
		int transparentCount() const { return static_cast<int>(transparent.size()); }

		bool empty() const { return opaque.empty() && transparent.empty(); }
	};

	// ---------------------------------------------------------------------------
	// ChunkMeshLOD — holds 5 levels of detail for a single chunk.
	// Smoother LOD transitions with more granular detail levels.
	// ---------------------------------------------------------------------------
	struct ChunkMeshLOD
	{
		ChunkMesh lod0;  // full detail (step=1)
		ChunkMesh lod1;  // 2×2×2 regions (step=2)
		ChunkMesh lod2;  // 3×3×3 regions (step=3)
		ChunkMesh lod3;  // 4×4×4 regions (step=4)
		ChunkMesh lod4;  // 6×6×6 regions (step=6)
	};

	// ---------------------------------------------------------------------------
	// ChunkNeighbours — pointers to the four XZ-adjacent chunks used during
	// meshing to correctly cull faces at chunk boundaries.  Any pointer may be
	// null (chunk not yet loaded); a null neighbour is treated as Air.
	// ---------------------------------------------------------------------------
	struct ChunkNeighbours
	{
		std::shared_ptr<const Chunk::Chunk> posX;  // +X neighbour
		std::shared_ptr<const Chunk::Chunk> negX;  // -X neighbour
		std::shared_ptr<const Chunk::Chunk> posZ;  // +Z neighbour
		std::shared_ptr<const Chunk::Chunk> negZ;  // -Z neighbour
	};

	// ---------------------------------------------------------------------------
	// ChunkMesher — builds a face-culled mesh for a 16×128×16 chunk.
	// Each visible face emits two triangles (6 vertices, no index buffer).
	// ---------------------------------------------------------------------------
	class ChunkMesher
	{
	public:
		// Build a mesh for 'chunk', sampling neighbours for boundary culling.
		static ChunkMesh build(const Chunk::Chunk& chunk,
							   const ChunkNeighbours& neighbours = {});

		// Build LOD meshes: 5 levels from full detail to very distant
		static ChunkMeshLOD buildLOD(const Chunk::Chunk& chunk,
									 const ChunkNeighbours& neighbours = {});

	private:
		// Returns true when the neighbouring voxel occludes the face.
		// 'neighbours' is consulted when (nx,ny,nz) falls outside local bounds.
		static bool shouldCull(const Chunk::Chunk& chunk,
							   const ChunkNeighbours& neighbours,
							   int nx, int ny, int nz,
							   Voxel::BlockID emitterId);

		// Emit 6 vertices (two tris) for one quad face into the target list.
		// lights[4] holds per-corner AO-baked brightness values (corner order
		// matches kFaceQuads[face][0..3]).  The quad winding may be flipped to
		// keep the AO gradient consistent across the diagonal.
		// skyLights[4] holds per-corner sky light levels (0-15).
		static void emitFace(std::vector<ChunkVertex>& out,
							 int x, int y, int z,
							 Voxel::FaceDir face,
							 int texLayer,
							 const float lights[4],
							 const float skyLights[4],
							 bool isWaterSurface = false);
	};

} // namespace Meshing
