#pragma once

#include <cstdint>

namespace Meshing
{

	// ---------------------------------------------------------------------------
	// ChunkVertex — one vertex of a voxel face quad.
	//
	// Layout (28 bytes):
	//   float  x, y, z     — world-space position
	//   float  u, v        — texture coordinates [0,1]
	//   float  texLayer    — GL_TEXTURE_2D_ARRAY layer index (cast to float for GLSL)
	//   float  light       — simple face-based directional light factor [0,1]
	//   uint32_t _pad      — keeps the struct a multiple of 4 bytes (future use)
	//
	// Attribute locations (must match the chunk vertex shader):
	//   0 = position (vec3)
	//   1 = uv       (vec2)
	//   2 = texLayer (float)
	//   3 = light    (float)
	// ---------------------------------------------------------------------------
	struct ChunkVertex
	{
		float x, y, z;       // world position
		float u, v;          // UV [0,1]
		float texLayer;      // array layer
		float light;         // directional factor

		static ChunkVertex make(float px, float py, float pz,
								float pu, float pv,
								int   layer,
								float lum)
		{
			return { px, py, pz, pu, pv,
					 static_cast<float>(layer),
					 lum };
		}
	};

	// Keep old name as an alias so any legacy code still compiles.
	using VoxelVertex = ChunkVertex;

} // namespace Meshing
