#pragma once

#include <WorldCraft.h>
#include <Voxel/BlockTypes.h>

namespace Meshing
{

	// Number of floats per vertex in the debug cube (position xyz + colour rgb).
	inline constexpr int CUBE_FLOATS_PER_VERTEX = 6;

	// Total number of vertices in the debug cube (6 faces * 2 tris * 3 verts).
	inline constexpr int CUBE_VERTEX_COUNT = 36;

	// Interleaved position+colour vertex data for a unit cube centred at the origin.
	// Each face has a distinct colour to aid visual debugging.
	extern const float cubeVerts[CUBE_VERTEX_COUNT * CUBE_FLOATS_PER_VERTEX];

	// Upload cubeVerts to the GPU and configure vertex attributes.
	// Returns the VAO handle. vboOut is filled with the created VBO handle.
	GLuint buildCubeVAO(GLuint& vboOut);

} // namespace Meshing
