#include <Meshing/CubePrimitive.h>

namespace Meshing
{

// ---------------------------------------------------------------------------
// Unit cube geometry  (6 faces * 2 tris * 3 verts = 36 vertices)
// Each vertex: position (xyz) + colour (rgb)
// ---------------------------------------------------------------------------
const float cubeVerts[CUBE_VERTEX_COUNT * CUBE_FLOATS_PER_VERTEX] =
{
	// Back face  (blue)
	-0.5f, -0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	 0.5f,  0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	 0.5f, -0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	 0.5f,  0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	-0.5f, -0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	-0.5f,  0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	// Front face (cyan)
	-0.5f, -0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	 0.5f, -0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	 0.5f,  0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	 0.5f,  0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	-0.5f,  0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	-0.5f, -0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	// Left face  (green)
	-0.5f,  0.5f,  0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f,  0.5f, -0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f, -0.5f, -0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f, -0.5f, -0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f, -0.5f,  0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f,  0.5f,  0.5f,  0.20f, 0.75f, 0.30f,
	// Right face (orange)
	 0.5f,  0.5f,  0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f, -0.5f, -0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f,  0.5f, -0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f, -0.5f, -0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f,  0.5f,  0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f, -0.5f,  0.5f,  0.90f, 0.55f, 0.10f,
	// Bottom face (red)
	-0.5f, -0.5f, -0.5f,  0.85f, 0.20f, 0.20f,
	 0.5f, -0.5f, -0.5f,  0.85f, 0.20f, 0.20f,
	 0.5f, -0.5f,  0.5f,  0.85f, 0.20f, 0.20f,
	 0.5f, -0.5f,  0.5f,  0.85f, 0.20f, 0.20f,
	-0.5f, -0.5f,  0.5f,  0.85f, 0.20f, 0.20f,
	-0.5f, -0.5f, -0.5f,  0.85f, 0.20f, 0.20f,
	// Top face   (yellow)
	-0.5f,  0.5f, -0.5f,  0.90f, 0.85f, 0.10f,
	 0.5f,  0.5f,  0.5f,  0.90f, 0.85f, 0.10f,
	 0.5f,  0.5f, -0.5f,  0.90f, 0.85f, 0.10f,
	 0.5f,  0.5f,  0.5f,  0.90f, 0.85f, 0.10f,
	-0.5f,  0.5f, -0.5f,  0.90f, 0.85f, 0.10f,
	-0.5f,  0.5f,  0.5f,  0.90f, 0.85f, 0.10f,
};

GLuint buildCubeVAO(GLuint& vboOut)
{
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vboOut);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vboOut);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

	const int stride = CUBE_FLOATS_PER_VERTEX * static_cast<int>(sizeof(float));

	// Position attribute  (location 0, 3 floats, offset 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
						  reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);

	// Colour attribute    (location 1, 3 floats, offset 3 floats)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
						  reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	return vao;
}

} // namespace Meshing
