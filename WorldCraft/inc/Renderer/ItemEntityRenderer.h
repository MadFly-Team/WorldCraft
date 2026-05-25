#pragma once

#include <WorldCraft.h>
#include <Voxel/BlockTypes.h>
#include <Voxel/BlockRegistry.h>
#include <glm/glm.hpp>

namespace Renderer
{

// ---------------------------------------------------------------------------
// ItemEntityRenderer - Renders dropped item entities as small 3D cubes
// Uses the same block texture array as the world chunks
// ---------------------------------------------------------------------------
class ItemEntityRenderer
{
public:
	ItemEntityRenderer();
	~ItemEntityRenderer();

	// Initialize OpenGL resources (VAO, VBO, shader)
	// Must be called after OpenGL context is created
	void init();

	// Clean up OpenGL resources
	void destroy();

	// Render a single item entity cube
	// pos: world position of the item
	// blockType: which block texture to use
	// rotation: rotation angle in radians around Y axis
	// scale: size multiplier (default 0.25 for small item)
	void renderItem(const glm::vec3& pos, Voxel::BlockID blockType, 
					float rotation, const glm::mat4& view, const glm::mat4& proj,
					float scale = 0.25f);

	// Batch render multiple items (more efficient)
	void beginBatch(const glm::mat4& view, const glm::mat4& proj);
	void renderItemBatch(const glm::vec3& pos, Voxel::BlockID blockType, 
						 float rotation, float scale = 0.25f);
	void endBatch();

private:
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_shaderProgram;

	// Uniform locations
	GLint m_mvpLoc;
	GLint m_texLayerLoc;

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
