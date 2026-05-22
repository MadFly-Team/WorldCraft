#pragma once

#include <cstdint>

namespace Meshing
{

	// ---------------------------------------------------------------------------
	// ChunkVertex — one vertex of a voxel face quad.
	//
	// Layout (36 bytes):
	//   float  x, y, z     — world-space position
	//   float  u, v        — texture coordinates [0,1]
	//   float  texLayer    — GL_TEXTURE_2D_ARRAY layer index (cast to float for GLSL)
	//   float  light       — simple face-based directional light factor [0,1]
	//   float  faceType    — 0=bottom/side bottom, 1=top face, 0.5=side top edge
	//   float  skyLight    — sky light level 0-15 from light propagation system
	//
	// Attribute locations (must match the chunk vertex shader):
	//   0 = position (vec3)
	//   1 = uv       (vec2)
	//   2 = texLayer (float)
	//   3 = light    (float)
	//   4 = faceType (float)
	//   5 = skyLight (float)
	// ---------------------------------------------------------------------------
	struct ChunkVertex
	{
		float x, y, z;       // world position
		float u, v;          // UV [0,1]
		float texLayer;      // array layer
		float light;         // directional factor
		float faceType;      // 0=bottom, 0.5=side top edge, 1=top
		float skyLight;      // sky light level 0-15

		static ChunkVertex make(float px, float py, float pz,
								float pu, float pv,
								int   layer,
								float lum,
								float ftype = 0.0f,
								float skyLt = 15.0f)
		{
			return { px, py, pz, pu, pv,
					 static_cast<float>(layer),
					 lum,
					 ftype,
					 skyLt };
		}
	};

	// Keep old name as an alias so any legacy code still compiles.
	using VoxelVertex = ChunkVertex;

} // namespace Meshing
