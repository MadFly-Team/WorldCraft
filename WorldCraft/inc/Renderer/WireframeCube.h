#pragma once

#include <glm/glm.hpp>
#include <glad/gl.h>

namespace Renderer
{

	// ---------------------------------------------------------------------------
	// WireframeCube — renders a wireframe outline cube at a specified position.
	//
	// Used for highlighting the block the player is looking at.
	// ---------------------------------------------------------------------------
	class WireframeCube
	{
	public:
		WireframeCube();
		~WireframeCube();

		// Initialize OpenGL resources (call after GL context is created)
		void init();

		// Clean up OpenGL resources (call before GL context destruction)
		void cleanup();

		// Render the wireframe cube at the specified block position
		// blockPos: integer block coordinates
		// mvp: combined model-view-projection matrix
		// color: RGBA color for the wireframe (default: white with transparency)
		void render(const glm::ivec3& blockPos,
					const glm::mat4& mvp,
					const glm::vec4& color = glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));

	private:
		GLuint m_vao = 0;
		GLuint m_vbo = 0;
		GLuint m_shader = 0;
		GLint  m_mvpLoc = -1;
		GLint  m_colorLoc = -1;
		GLint  m_offsetLoc = -1;

		void createShader();
		void createGeometry();
	};

} // namespace Renderer
