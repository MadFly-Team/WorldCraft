#include <Meshing/ChunkMesher.h>
#include <Voxel/BlockRegistry.h>
#include <unordered_map>
#include <vector>
#include <utility>

namespace Meshing
{

// ---------------------------------------------------------------------------
// Per-face directional light factors
// ---------------------------------------------------------------------------
static constexpr float kFaceLight[6] = {
	0.80f,  // PosX
	0.80f,  // NegX
	1.00f,  // PosY (top)
	0.50f,  // NegY (bottom)
	0.90f,  // PosZ
	0.90f,  // NegZ
};

// ---------------------------------------------------------------------------
// Per-face quad corner offsets (4 corners × dx/dy/dz + uv).
//
// Winding: CCW when viewed from OUTSIDE the block (the direction the face normal
// points).  OpenGL culls back faces (GL_BACK / GL_CCW default).
//
// For each face the outward normal is:
//   PosX: +X   NegX: -X   PosY: +Y   NegY: -Y   PosZ: +Z   NegZ: -Z
//
// A quad is laid out as four corners [A,B,C,D] and the two triangles are
// ABC + ACD (indices 0,1,2,0,2,3).  The cross product (B-A)×(C-A) must
// point in the outward normal direction.
// ---------------------------------------------------------------------------
struct FaceCorner { float dx, dy, dz, u, v; };

static constexpr FaceCorner kFaceQuads[6][4] = {
	// PosX (+X face, normal = +X)
	// viewed from +X: Z increases left, Y increases up
	// A=(1,0,1) B=(1,0,0) C=(1,1,0) D=(1,1,1)   (B-A)×(C-A) = +X ✓
	{{ 1,0,1, 0,0 }, { 1,0,0, 1,0 }, { 1,1,0, 1,1 }, { 1,1,1, 0,1 }},

	// NegX (-X face, normal = -X)
	// viewed from -X: Z increases right, Y increases up
	// A=(0,0,0) B=(0,0,1) C=(0,1,1) D=(0,1,0)   (B-A)×(C-A) = -X ✓
	{{ 0,0,0, 0,0 }, { 0,0,1, 1,0 }, { 0,1,1, 1,1 }, { 0,1,0, 0,1 }},

	// PosY (+Y face, normal = +Y, top face)
	// A=(0,1,0) B=(0,1,1) C=(1,1,1) D=(1,1,0)
	// (B-A)=(0,0,1)  (C-A)=(1,0,1)  cross=(0,1,0)=+Y ✓
	{{ 0,1,0, 0,0 }, { 0,1,1, 0,1 }, { 1,1,1, 1,1 }, { 1,1,0, 1,0 }},

	// NegY (-Y face, normal = -Y, bottom face)
	// A=(0,0,0) B=(1,0,0) C=(1,0,1) D=(0,0,1)
	// (B-A)=(1,0,0)  (C-A)=(1,0,1)  cross=(0,-1,0)=-Y ✓
	{{ 0,0,0, 0,0 }, { 1,0,0, 1,0 }, { 1,0,1, 1,1 }, { 0,0,1, 0,1 }},

	// PosZ (+Z face, normal = +Z, front face)
	// viewed from +Z: X increases right, Y increases up
	// A=(0,0,1) B=(1,0,1) C=(1,1,1) D=(0,1,1)   (B-A)×(C-A) = +Z ✓
	{{ 0,0,1, 0,0 }, { 1,0,1, 1,0 }, { 1,1,1, 1,1 }, { 0,1,1, 0,1 }},

	// NegZ (-Z face, normal = -Z, back face)
	// viewed from -Z: X increases left, Y increases up
	// A=(1,0,0) B=(0,0,0) C=(0,1,0) D=(1,1,0)   (B-A)×(C-A) = -Z ✓
	{{ 1,0,0, 0,0 }, { 0,0,0, 1,0 }, { 0,1,0, 1,1 }, { 1,1,0, 0,1 }},
};

static constexpr int kTriIdx[6] = { 0, 1, 2, 0, 2, 3 };

// ---------------------------------------------------------------------------
// Ambient Occlusion helpers
// (defined here — after FaceCorner and kFaceQuads which they reference)
// ---------------------------------------------------------------------------

// AO darkening factors: 0 occluding neighbours = fully lit, 3 = darkest.
static constexpr float kAOTable[4] = { 1.00f, 0.80f, 0.65f, 0.50f };

// Per-face integer normal vectors.
static constexpr int kFaceNX[6] = {  1,-1, 0, 0, 0, 0 };
static constexpr int kFaceNY[6] = {  0, 0, 1,-1, 0, 0 };
static constexpr int kFaceNZ[6] = {  0, 0, 0, 0, 1,-1 };

// Per-face tangent T and B axes that span each quad plane.
//   face 0 PosX : T=(0,1,0)  B=(0,0,1)
//   face 1 NegX : T=(0,1,0)  B=(0,0,1)
//   face 2 PosY : T=(1,0,0)  B=(0,0,1)
//   face 3 NegY : T=(1,0,0)  B=(0,0,1)
//   face 4 PosZ : T=(1,0,0)  B=(0,1,0)
//   face 5 NegZ : T=(1,0,0)  B=(0,1,0)
static constexpr int kTX[6] = { 0,0,1,1,1,1 };
static constexpr int kTY[6] = { 1,1,0,0,0,0 };
// kTZ is 0 for all faces.
static constexpr int kBY[6] = { 0,0,0,0,1,1 };
static constexpr int kBZ[6] = { 1,1,1,1,0,0 };
// kBX is 0 for all faces.

// Returns true when block (nx,ny,nz) should count as an AO occluder.
static bool isSolidForAO(const Chunk::Chunk& chunk,
						 const ChunkNeighbours& nb,
						 int nx, int ny, int nz)
{
	if (ny < 0 || ny >= Chunk::CHUNK_SIZE_Y) return false;

	Voxel::BlockID id = Voxel::BlockID::Air;
	if (!Chunk::Chunk::inBounds(nx, ny, nz))
	{
		const Chunk::Chunk* nbChunk = nullptr;
		int lx = nx, lz = nz;
		if      (nx < 0)                        { nbChunk = nb.negX.get(); lx = nx + Chunk::CHUNK_SIZE_X; }
		else if (nx >= Chunk::CHUNK_SIZE_X)     { nbChunk = nb.posX.get(); lx = nx - Chunk::CHUNK_SIZE_X; }
		else if (nz < 0)                        { nbChunk = nb.negZ.get(); lz = nz + Chunk::CHUNK_SIZE_Z; }
		else                                    { nbChunk = nb.posZ.get(); lz = nz - Chunk::CHUNK_SIZE_Z; }
		if (!nbChunk) return true; // unloaded = treat as solid
		id = nbChunk->getBlock(lx, ny, lz);
	}
	else
	{
		id = chunk.getBlock(nx, ny, nz);
	}
	if (id == Voxel::BlockID::Air) return false;
	return Voxel::BlockRegistry::get().propertiesOf(id).isSolid;
}

// Compute per-corner AO-baked light values (kFaceLight * AO factor) for all
// 4 corners of one face.  outLights must point to float[4].
static void computeFaceAO(const Chunk::Chunk& chunk,
						  const ChunkNeighbours& nb,
						  int bx, int by, int bz,
						  int fi,
						  float outLights[4])
{
	const int nnx = kFaceNX[fi];
	const int nny = kFaceNY[fi];
	const int nnz = kFaceNZ[fi];
	const int tx  = kTX[fi], ty = kTY[fi];
	const int by_ = kBY[fi], bz_ = kBZ[fi]; // B direction (kBX=0 always)

	for (int ci = 0; ci < 4; ++ci)
	{
		const FaceCorner& c = kFaceQuads[fi][ci];

		// Determine sign of this corner along T and B axes.
		const float tComp = c.dx * tx + c.dy * ty; // 0 or 1
		const float bComp = c.dy * by_ + c.dz * bz_; // 0 or 1
		const int   sT    = (tComp > 0.5f) ? 1 : -1;
		const int   sB    = (bComp > 0.5f) ? 1 : -1;

		// Three AO sample positions in the outer face plane (shifted by N).
		const int s1x = bx + nnx + sT * tx;
		const int s1y = by + nny + sT * ty;
		const int s1z = bz + nnz; // kTZ=0

		const int s2x = bx + nnx;
		const int s2y = by + nny + sB * by_;
		const int s2z = bz + nnz + sB * bz_;

		const int crx = bx + nnx + sT * tx;
		const int cry = by + nny + sT * ty + sB * by_;
		const int crz = bz + nnz + sB * bz_;

		const bool side1  = isSolidForAO(chunk, nb, s1x, s1y, s1z);
		const bool side2  = isSolidForAO(chunk, nb, s2x, s2y, s2z);
		const bool corner = isSolidForAO(chunk, nb, crx, cry, crz);

		// When both sides solid, corner is fully blocked (can't see diagonal).
		const int occluders = (side1 && side2)
			? 3
			: (static_cast<int>(side1) + static_cast<int>(side2) + static_cast<int>(corner));

		outLights[ci] = kFaceLight[fi] * kAOTable[occluders];
	}
}

// ---------------------------------------------------------------------------
// Upload helper
// ---------------------------------------------------------------------------

static void uploadVertices(const std::vector<ChunkVertex>& verts,
						   GLuint& vao, GLuint& vbo)
{
	if (vao == 0) glGenVertexArrays(1, &vao);
	if (vbo == 0) glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER,
				 static_cast<GLsizeiptr>(verts.size() * sizeof(ChunkVertex)),
				 verts.data(),
				 GL_DYNAMIC_DRAW);

	const GLsizei stride = sizeof(ChunkVertex);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
						  reinterpret_cast<void*>(offsetof(ChunkVertex, x)));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
						  reinterpret_cast<void*>(offsetof(ChunkVertex, u)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride,
						  reinterpret_cast<void*>(offsetof(ChunkVertex, texLayer)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride,
						  reinterpret_cast<void*>(offsetof(ChunkVertex, light)));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);
}

void ChunkMesh::uploadOpaque(GLuint& vao, GLuint& vbo) const
{
	uploadVertices(opaque, vao, vbo);
}

void ChunkMesh::uploadTransparent(GLuint& vao, GLuint& vbo) const
{
	uploadVertices(transparent, vao, vbo);
}

// ---------------------------------------------------------------------------
// ChunkMesher
// ---------------------------------------------------------------------------

bool ChunkMesher::shouldCull(const Chunk::Chunk& chunk,
							  const ChunkNeighbours& neighbours,
							  int nx, int ny, int nz,
							  Voxel::BlockID emitterId)
{
	const auto& reg = Voxel::BlockRegistry::get();
	const auto& ep  = reg.propertiesOf(emitterId);

	Voxel::BlockID nb = Voxel::BlockID::Air;

	if (!Chunk::Chunk::inBounds(nx, ny, nz))
	{
		// Coordinate crosses a chunk boundary — look up the neighbour chunk.
		// Y out-of-bounds stays Air (top/bottom of the world).
		if (ny < 0 || ny >= Chunk::CHUNK_SIZE_Y)
		{
			// No block above/below world — never cull.
			return false;
		}

		const Chunk::Chunk* nbChunk = nullptr;
		int localX = nx, localZ = nz;

		if (nx < 0)                    { nbChunk = neighbours.negX.get(); localX = nx + Chunk::CHUNK_SIZE_X; }
		else if (nx >= Chunk::CHUNK_SIZE_X) { nbChunk = neighbours.posX.get(); localX = nx - Chunk::CHUNK_SIZE_X; }
		else if (nz < 0)               { nbChunk = neighbours.negZ.get(); localZ = nz + Chunk::CHUNK_SIZE_Z; }
		else                           { nbChunk = neighbours.posZ.get(); localZ = nz - Chunk::CHUNK_SIZE_Z; }

		if (!nbChunk)
		{
			// Neighbour not loaded yet: cull transparent blocks (avoid seams),
			// show opaque faces (avoid invisible walls).
			return ep.isTransparent;
		}

		nb = nbChunk->getBlock(localX, ny, localZ);
	}
	else
	{
		nb = chunk.getBlock(nx, ny, nz);
	}

	if (nb == Voxel::BlockID::Air) return false;

	const auto& nbp = reg.propertiesOf(nb);

	if (ep.isTransparent)
	{
		// Transparent: cull only against the same block type.
		return nb == emitterId;
	}

	// Opaque solid: cull if neighbour is also opaque solid.
	return nbp.isSolid && !nbp.isTransparent;
}

void ChunkMesher::emitFace(std::vector<ChunkVertex>& out,
						   int x, int y, int z,
						   Voxel::FaceDir face,
						   int texLayer,
						   const float lights[4])
{
	const int fi = static_cast<int>(face);
	const FaceCorner* c = kFaceQuads[fi];

	ChunkVertex quad[4];
	for (int i = 0; i < 4; ++i)
		quad[i] = ChunkVertex::make(
			static_cast<float>(x) + c[i].dx,
			static_cast<float>(y) + c[i].dy,
			static_cast<float>(z) + c[i].dz,
			c[i].u, c[i].v, texLayer, lights[i]);

	// AO anisotropy fix: choose the quad diagonal that keeps the darker
	// crease consistent (avoids ugly "pinching" on AO-darkened corners).
	if (lights[0] + lights[2] < lights[1] + lights[3])
	{
		// Flipped winding: 1,2,3 + 1,3,0
		out.push_back(quad[1]); out.push_back(quad[2]); out.push_back(quad[3]);
		out.push_back(quad[1]); out.push_back(quad[3]); out.push_back(quad[0]);
	}
	else
	{
		for (int i : kTriIdx)
			out.push_back(quad[i]);
	}
}

ChunkMesh ChunkMesher::build(const Chunk::Chunk& chunk,
							  const ChunkNeighbours& neighbours)
{
	ChunkMesh mesh;
	const Voxel::BlockRegistry& reg = Voxel::BlockRegistry::get();

	// Find the highest non-air block to skip empty sky above terrain.
	int maxY = 0;
	for (int y = Chunk::CHUNK_SIZE_Y - 1; y >= 0; --y)
	{
		bool found = false;
		for (int z = 0; z < Chunk::CHUNK_SIZE_Z && !found; ++z)
			for (int x = 0; x < Chunk::CHUNK_SIZE_X && !found; ++x)
				if (chunk.getBlock(x, y, z) != Voxel::BlockID::Air) found = true;
		if (found) { maxY = y; break; }
	}

	// Iterate XZ outer, Y inner — better cache locality on the flat array,
	// and we stop early at maxY+1 to skip the empty sky above terrain.
	for (int z = 0; z < Chunk::CHUNK_SIZE_Z; ++z)
	for (int x = 0; x < Chunk::CHUNK_SIZE_X; ++x)
	for (int y = 0; y <= maxY + 1 && y < Chunk::CHUNK_SIZE_Y; ++y)
	{
		const Voxel::BlockID id = chunk.getBlock(x, y, z);
		if (id == Voxel::BlockID::Air) continue;

		const auto& props = reg.propertiesOf(id);
		if (!props.isSolid && !props.isTransparent) continue;

		const bool isTransp = props.isTransparent;

		for (int fi = 0; fi < static_cast<int>(Voxel::FaceDir::COUNT); ++fi)
		{
			const int nx = x + Voxel::FaceOffsetX[fi];
			const int ny = y + Voxel::FaceOffsetY[fi];
			const int nz = z + Voxel::FaceOffsetZ[fi];

			if (shouldCull(chunk, neighbours, nx, ny, nz, id)) continue;

			const auto fdir  = static_cast<Voxel::FaceDir>(fi);
			const int layer  = reg.texLayer(id, fdir);
			auto& target     = isTransp ? mesh.transparent : mesh.opaque;

			// Compute per-corner AO (baked into the light values).
			float lights[4];
			computeFaceAO(chunk, neighbours, x, y, z, fi, lights);
			emitFace(target, x, y, z, fdir, layer, lights);
		}
	}

	return mesh;
}

// ---------------------------------------------------------------------------
// LOD mesh builder: generates 3 levels of detail
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Helper: Sample a single neighbor LOD region to see if it would create a LOD voxel
// Returns (BlockID, topY, bottomY) for the block found in this region
// Used for sampling cross-chunk neighbors for transparent blocks (water)
// ---------------------------------------------------------------------------
static std::tuple<Voxel::BlockID, int, int> sampleNeighborLODRegion(
	const Chunk::Chunk& chunk,
	const ChunkNeighbours& neighbours,
	int regionMinX,
	int regionMinY,
	int regionMinZ,
	int step)
{
	const Voxel::BlockRegistry& reg = Voxel::BlockRegistry::get();

	// Helper to get block from chunk or neighbor
	auto getBlockAt = [&](int worldX, int worldY, int worldZ) -> Voxel::BlockID
	{
		if (worldY < 0 || worldY >= Chunk::CHUNK_SIZE_Y)
			return Voxel::BlockID::Air;

		if (Chunk::Chunk::inBounds(worldX, worldY, worldZ))
		{
			return chunk.getBlock(worldX, worldY, worldZ);
		}

		// Out of bounds - check neighbor chunk
		const Chunk::Chunk* nbChunk = nullptr;
		int localX = worldX, localZ = worldZ;

		if (worldX < 0)
		{
			nbChunk = neighbours.negX.get();
			localX = worldX + Chunk::CHUNK_SIZE_X;
		}
		else if (worldX >= Chunk::CHUNK_SIZE_X)
		{
			nbChunk = neighbours.posX.get();
			localX = worldX - Chunk::CHUNK_SIZE_X;
		}
		else if (worldZ < 0)
		{
			nbChunk = neighbours.negZ.get();
			localZ = worldZ + Chunk::CHUNK_SIZE_Z;
		}
		else if (worldZ >= Chunk::CHUNK_SIZE_Z)
		{
			nbChunk = neighbours.posZ.get();
			localZ = worldZ - Chunk::CHUNK_SIZE_Z;
		}

		if (!nbChunk)
			return Voxel::BlockID::Air;

		return nbChunk->getBlock(localX, worldY, localZ);
	};

	// Sample the step×step×step region, checking from top downward
	Voxel::BlockID representative = Voxel::BlockID::Air;
	int topY = regionMinY;
	int bottomY = regionMinY;

	bool found = false;
	for (int dy = step - 1; dy >= 0 && !found; --dy)
	{
		const int worldY = regionMinY + dy;
		if (worldY >= Chunk::CHUNK_SIZE_Y) continue;

		for (int dz = 0; dz < step && !found; ++dz)
		{
			const int worldZ = regionMinZ + dz;

			for (int dx = 0; dx < step && !found; ++dx)
			{
				const int worldX = regionMinX + dx;
				const Voxel::BlockID sample = getBlockAt(worldX, worldY, worldZ);

				if (sample != Voxel::BlockID::Air)
				{
					representative = sample;
					topY = worldY;
					found = true;

					// For opaque blocks, scan down to find bottom
					if (reg.propertiesOf(sample).isSolid)
					{
						bottomY = worldY;
						for (int scanY = worldY - 1; scanY >= regionMinY && scanY >= 0; --scanY)
						{
							bool hasSolid = false;
							for (int scanZ = 0; scanZ < step && !hasSolid; ++scanZ)
							{
								const int checkZ = regionMinZ + scanZ;
								for (int scanX = 0; scanX < step && !hasSolid; ++scanX)
								{
									const int checkX = regionMinX + scanX;
									const Voxel::BlockID checkBlock = getBlockAt(checkX, scanY, checkZ);
									if (checkBlock != Voxel::BlockID::Air &&
										reg.propertiesOf(checkBlock).isSolid)
									{
										hasSolid = true;
										bottomY = scanY;
									}
								}
							}
							if (!hasSolid) break;
						}
					}
					else
					{
						bottomY = topY; // Transparent blocks
					}
				}
			}
		}
	}

	return {representative, topY, bottomY};
}

// ---------------------------------------------------------------------------
// Greedy meshing for LOD: merges adjacent faces of same block type into larger quads
// ---------------------------------------------------------------------------
static ChunkMesh buildGreedyLOD(const Chunk::Chunk& chunk,
								const ChunkNeighbours& neighbours,
								int lodLevel)
{
	ChunkMesh mesh;
	const Voxel::BlockRegistry& reg = Voxel::BlockRegistry::get();

	// Step size: LOD1=2, LOD2=4 (represents how many real blocks each LOD voxel covers)
	const int step = (1 << lodLevel);

	// Find the highest non-air block to skip empty sky above terrain.
	int maxY = 0;
	for (int y = Chunk::CHUNK_SIZE_Y - 1; y >= 0; --y)
	{
		bool found = false;
		for (int z = 0; z < Chunk::CHUNK_SIZE_Z && !found; ++z)
			for (int x = 0; x < Chunk::CHUNK_SIZE_X && !found; ++x)
				if (chunk.getBlock(x, y, z) != Voxel::BlockID::Air) found = true;
		if (found) { maxY = y; break; }
	}

	// Build a simplified voxel grid by sampling step×step×step regions
	// Each LOD voxel represents a step×step×step region of the original chunk
	const int lodSizeX = (Chunk::CHUNK_SIZE_X + step - 1) / step;
	const int lodSizeY = (Chunk::CHUNK_SIZE_Y + step - 1) / step;
	const int lodSizeZ = (Chunk::CHUNK_SIZE_Z + step - 1) / step;

	std::vector<Voxel::BlockID> lodVoxels(lodSizeX * lodSizeY * lodSizeZ, Voxel::BlockID::Air);
	// Track the actual Y position of each block within its region (for proper height alignment)
	std::vector<int> lodBlockTopY(lodSizeX * lodSizeY * lodSizeZ, 0);
	std::vector<int> lodBlockBottomY(lodSizeX * lodSizeY * lodSizeZ, 0);

	auto lodIndex = [&](int x, int y, int z) -> int {
		return x + lodSizeX * (y + lodSizeY * z);
	};

	// Sample the chunk at LOD resolution
	// For each XZ column in the LOD grid, find the topmost non-air block
	for (int lz = 0; lz < lodSizeZ; ++lz)
	for (int lx = 0; lx < lodSizeX; ++lx)
	{
		// Process this XZ column from top to bottom
		for (int ly = lodSizeY - 1; ly >= 0; --ly)
		{
			Voxel::BlockID representative = Voxel::BlockID::Air;
			int topY = ly * step;    // Top of visible block
			int bottomY = ly * step; // Bottom of solid column

			// Multi-point sampling strategy: sample multiple interior points
			// within the region, avoiding edges for better representation
			std::vector<std::pair<int, int>> sampleOffsets;

			// Define sample points based on step size (interior-biased)
			if (step == 2) {
				// For 2×2: sample both interior points (avoid 0 and 1 edges)
				sampleOffsets = {{0, 0}, {1, 1}};  // Diagonal
			}
			else if (step == 3) {
				// For 3×3: sample center and mid-points
				sampleOffsets = {{1, 1}, {1, 0}, {0, 1}};  // Center + two mids
			}
			else if (step == 4) {
				// For 4×4: sample interior 2×2 grid
				sampleOffsets = {{1, 1}, {1, 2}, {2, 1}, {2, 2}};
			}
			else if (step == 6) {
				// For 6×6: sample interior points avoiding edges
				sampleOffsets = {{2, 2}, {2, 4}, {4, 2}, {4, 4}, {3, 3}};  // Cross pattern + center
			}
			else {
				// Fallback: just sample center
				sampleOffsets = {{step / 2, step / 2}};
			}

			// Count block types across all sample points
			std::unordered_map<int, int> blockCounts;
			bool found = false;

			for (int dy = step - 1; dy >= 0 && !found; --dy)
			{
				const int worldY = ly * step + dy;
				if (worldY >= Chunk::CHUNK_SIZE_Y) continue;

				// Sample all the defined points at this height
				for (const auto& [ox, oz] : sampleOffsets)
				{
					const int worldX = lx * step + ox;
					const int worldZ = lz * step + oz;

					if (worldX >= Chunk::CHUNK_SIZE_X || worldZ >= Chunk::CHUNK_SIZE_Z)
						continue;

					const Voxel::BlockID sample = chunk.getBlock(worldX, worldY, worldZ);
					if (sample != Voxel::BlockID::Air)
					{
						blockCounts[static_cast<int>(sample)]++;
						if (!found)
						{
							representative = sample;
							topY = worldY;
							found = true;
						}
					}
				}
			}

			// Use the most common block type from our samples
			if (found && !blockCounts.empty())
			{
				int maxCount = 0;
				for (const auto& [blockId, count] : blockCounts)
				{
					if (count > maxCount)
					{
						maxCount = count;
						representative = static_cast<Voxel::BlockID>(blockId);
					}
				}

				// For opaque blocks, find the bottom of the solid column
				// Sample the center column for consistency
				const int centerX = lx * step + step / 2;
				const int centerZ = lz * step + step / 2;

				if (reg.propertiesOf(representative).isSolid)
				{
					// Scan down from the top to find where solid blocks end at the center column
					bottomY = topY;
					for (int scanY = topY - 1; scanY >= ly * step && scanY >= 0; --scanY)
					{
						const Voxel::BlockID checkBlock = chunk.getBlock(centerX, scanY, centerZ);
						if (checkBlock != Voxel::BlockID::Air &&
							reg.propertiesOf(checkBlock).isSolid)
						{
							bottomY = scanY;
						}
						else
						{
							break; // Hit air or transparent, stop scanning
						}
					}
				}
			}

			const int idx = lodIndex(lx, ly, lz);
			lodVoxels[idx] = representative;
			lodBlockTopY[idx] = topY;
			lodBlockBottomY[idx] = bottomY;
		}
	}

	// Now build meshes using standard per-LOD-voxel approach (no greedy merging for now - simpler and more reliable)
	for (int lz = 0; lz < lodSizeZ; ++lz)
	for (int lx = 0; lx < lodSizeX; ++lx)
	for (int ly = 0; ly < lodSizeY && ly * step <= maxY + step; ++ly)
	{
		const int idx = lodIndex(lx, ly, lz);
		const Voxel::BlockID id = lodVoxels[idx];
		if (id == Voxel::BlockID::Air) continue;

		const auto& props = reg.propertiesOf(id);
		if (!props.isSolid && !props.isTransparent) continue;

		const bool isTransp = props.isTransparent;

		// Get the actual Y position of this block
		// For opaque blocks: render from bottom to top of solid column
		// For transparent blocks (water): render just 1 block at top
		const int topY = lodBlockTopY[idx];
		const int bottomY = isTransp ? topY : lodBlockBottomY[idx];
		const int blockHeight = topY - bottomY + 1;

		// Check each face direction
		for (int fi = 0; fi < static_cast<int>(Voxel::FaceDir::COUNT); ++fi)
		{
			// Determine if this is a horizontal face (X or Z direction)
			const auto fdir = static_cast<Voxel::FaceDir>(fi);
			const bool isHorizontalFace = (fdir == Voxel::FaceDir::PosX || fdir == Voxel::FaceDir::NegX ||
										   fdir == Voxel::FaceDir::PosZ || fdir == Voxel::FaceDir::NegZ);

			bool shouldEmit = true;

			// For LOD meshes, be VERY conservative about culling horizontal faces
			// The terrain height variability makes perfect culling nearly impossible
			// Better to have some overdraw than gaps
			if (isHorizontalFace)
			{
				// For horizontal faces: only cull if we're CERTAIN the neighbor occludes us
				// This means: same block type, and neighbor completely encloses our Y range
				const int nlx = lx + Voxel::FaceOffsetX[fi];
				const int nlz = lz + Voxel::FaceOffsetZ[fi];

				// For transparent blocks (water), check cross-chunk neighbors to avoid grid pattern
				// For opaque blocks, only check within chunk (conservative to avoid gaps)
				const bool checkCrossChunk = isTransp;

				if ((nlx >= 0 && nlx < lodSizeX && nlz >= 0 && nlz < lodSizeZ) || checkCrossChunk)
				{
					// Check all Y levels in the neighbor column
					for (int nly = 0; nly < lodSizeY; ++nly)
					{
						Voxel::BlockID neighborId = Voxel::BlockID::Air;
						int neighborTopY = 0;
						int neighborBottomY = 0;

						if (nlx >= 0 && nlx < lodSizeX && nlz >= 0 && nlz < lodSizeZ)
						{
							// Within chunk - use LOD voxel grid
							const int nIdx = lodIndex(nlx, nly, nlz);
							neighborId = lodVoxels[nIdx];
							neighborTopY = lodBlockTopY[nIdx];
							neighborBottomY = lodBlockBottomY[nIdx];
						}
						else if (checkCrossChunk)
						{
							// Cross-chunk transparent block - sample the neighbor region
							// to avoid water grid pattern
							const int neighborMinX = nlx * step;
							const int neighborMinY = nly * step;
							const int neighborMinZ = nlz * step;

							// Sample just this one region (not the full column)
							auto [sampledBlock, sampledTopY, sampledBottomY] = sampleNeighborLODRegion(
								chunk, neighbours, neighborMinX, neighborMinY, neighborMinZ, step);
							neighborId = sampledBlock;
							neighborTopY = sampledTopY;
							neighborBottomY = sampledBottomY;
						}

						if (neighborId == Voxel::BlockID::Air) continue;

						// Check if neighbor completely encloses our Y range
						const bool neighborEnclosesUs = (neighborBottomY <= bottomY && neighborTopY >= topY);

						// For transparent blocks, also cull if they just overlap (to hide water seams)
						const bool overlapsY = !(topY < neighborBottomY || bottomY > neighborTopY);
						const bool shouldCheckOcclusion = isTransp ? overlapsY : neighborEnclosesUs;

						if (!shouldCheckOcclusion) continue;

						// Neighbor encloses/overlaps us - check if it's the same type (for transparent)
						// or opaque (for solid blocks)
						const auto& neighborProps = reg.propertiesOf(neighborId);

						if (isTransp)
						{
							// Transparent: cull if same type and overlaps (hide water grid)
							if (neighborId == id)
							{
								shouldEmit = false;
								break;
							}
						}
						else
						{
							// Opaque: only cull if neighbor is also opaque, solid, and completely encloses us
							if (neighborProps.isSolid && !neighborProps.isTransparent && neighborEnclosesUs)
							{
								shouldEmit = false;
								break;
							}
						}
					}
				}
				// else: opaque block outside chunk - always emit (conservative approach)
			}
			else
			{
				// Vertical face (PosY or NegY) - use simple neighbor check
				const int nlx = lx;
				const int nly = ly + Voxel::FaceOffsetY[fi];
				const int nlz = lz;

				if (nly >= 0 && nly < lodSizeY)
				{
					const int nIdx = lodIndex(nlx, nly, nlz);
					const Voxel::BlockID neighborId = lodVoxels[nIdx];

					if (neighborId != Voxel::BlockID::Air)
					{
						const auto& neighborProps = reg.propertiesOf(neighborId);

						if (isTransp)
						{
							shouldEmit = (neighborId != id);
						}
						else
						{
							shouldEmit = neighborProps.isTransparent || !neighborProps.isSolid;
						}
					}
				}
			}

			if (!shouldEmit) continue;

			// Emit a face for this LOD voxel
			// Use actual Y position for vertical alignment, scale only X and Z
			const int layer = reg.texLayer(id, fdir);
			auto& target = isTransp ? mesh.transparent : mesh.opaque;

			const int baseX = lx * step;
			const int baseZ = lz * step;

			// Get the standard face corners and scale them
			const FaceCorner* corners = kFaceQuads[fi];
			const float baseLight = kFaceLight[fi];

			ChunkVertex quad[4];
			for (int i = 0; i < 4; ++i)
			{
				// Scale X and Z to cover step×step blocks horizontally
				// Y: Use actual positions (bottom to top) to cover the solid column
				const float yOffset = corners[i].dy * blockHeight;
				quad[i].x = baseX + corners[i].dx * step;
				quad[i].y = bottomY + yOffset;
				quad[i].z = baseZ + corners[i].dz * step;
				quad[i].u = corners[i].u * step;              // Scale UVs horizontally
				quad[i].v = corners[i].v * blockHeight;       // Scale V to cover column height
				quad[i].texLayer = static_cast<float>(layer);
				quad[i].light = baseLight;  // No AO for LOD
			}

			// Emit two triangles
			for (int ti = 0; ti < 6; ++ti)
			{
				target.push_back(quad[kTriIdx[ti]]);
			}
		}
	}

	return mesh;
}

ChunkMeshLOD ChunkMesher::buildLOD(const Chunk::Chunk& chunk,
								   const ChunkNeighbours& neighbours)
{
	ChunkMeshLOD lodMesh;

	// LOD0: Full detail using standard per-block meshing with AO
	lodMesh.lod0 = build(chunk, neighbours);

	// LOD1: step=2 (2×2×2 regions)
	lodMesh.lod1 = buildGreedyLOD(chunk, neighbours, 2);

	// LOD2: step=3 (3×3×3 regions) - intermediate detail
	lodMesh.lod2 = buildGreedyLOD(chunk, neighbours, 3);

	// LOD3: step=4 (4×4×4 regions)
	lodMesh.lod3 = buildGreedyLOD(chunk, neighbours, 4);

	// LOD4: step=6 (6×6×6 regions) - very distant terrain
	lodMesh.lod4 = buildGreedyLOD(chunk, neighbours, 6);

	return lodMesh;
}

} // namespace Meshing
