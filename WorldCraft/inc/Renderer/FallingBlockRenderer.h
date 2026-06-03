#pragma once

#include <WorldCraft.h>
#include <World/FallingBlock.h>
#include <glm/glm.hpp>
#include <vector>

namespace Renderer
{

// ---------------------------------------------------------------------------
// FallingBlockRenderer - Renders falling block physics entities as full-size 3D cubes
// Uses the same block texture array as the world chunks
// Similar to ItemEntityRenderer but renders at full block size
// ---------------------------------------------------------------------------
class FallingBlockRenderer
{
public:
	FallingBlockRenderer();
	~FallingBlockRenderer();

	// Initialize OpenGL resources (VAO, VBO, shader)
	// Must be called after OpenGL context is created
	void init();

	// Clean up OpenGL resources
	void destroy();

	// Render a single falling block cube
	// pos: world position of the block
	// blockType: which block texture to use
	// rotation: rotation angle in radians around Y axis
	// scale: size multiplier (default 1.0 for full block size)
	void renderBlock(const glm::vec3& pos, Voxel::BlockID blockType, 
					float rotation, const glm::mat4& view, const glm::mat4& proj,
					float scale = 1.0f);

	// Batch render multiple falling blocks (more efficient)
	void beginBatch(const glm::mat4& view, const glm::mat4& proj);
	void renderBlockBatch(const glm::vec3& pos, Voxel::BlockID blockType, 
						 float rotation, float scale = 1.0f, bool underwater = false);
	void endBatch();

private:
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_shaderProgram;

	// Uniform locations
	GLint m_mvpLoc;
	GLint m_texLayerLoc;
	GLint m_tintColorLoc;

	// Batch rendering state
	bool m_batchActive;
	glm::mat4 m_batchView;
	glm::mat4 m_batchProj;

	// Build shader program
	void buildShader();

	// Generate cube mesh data
	void buildCubeMesh();
};

} // namespace Renderer
