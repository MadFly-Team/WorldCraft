#pragma once

#include <Sprite/Sprite.h>
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <vector>

namespace CoreLib {
namespace Sprite {

// ---------------------------------------------------------------------------
// SpriteRenderer - Batched sprite rendering system
// ---------------------------------------------------------------------------
class SpriteRenderer
{
public:
	SpriteRenderer();
	~SpriteRenderer();

	// Initialize the renderer
	bool initialize(int screenWidth, int screenHeight);

	// Shutdown and cleanup
	void shutdown();

	// Frame lifecycle
	void begin();
	void end();
	void flush();

	// Screen size
	void setScreenSize(int width, int height);

	// Drawing
	void drawSprite(const CoreLib::Sprite::Sprite& sprite);
	void drawSprite(std::shared_ptr<Texture> texture, const glm::vec2& position, const glm::vec2& size, 
					const Graphics2D::Color& color = Graphics2D::Color::White());

	// Batch rendering
	void beginBatch();
	void endBatch();

	// State
	bool isInitialized() const { return m_initialized; }

private:
	struct SpriteVertex
	{
		glm::vec2 position;
		glm::vec2 texCoord;
		Graphics2D::Color color;
	};

	void createShaders();
	void createBuffers();
	void renderBatch();

	bool m_initialized;
	int m_screenWidth;
	int m_screenHeight;
	glm::mat4 m_projection;

	// OpenGL resources
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ebo;
	GLuint m_shaderProgram;
	GLint m_projectionLoc;
	GLint m_textureLoc;

	// Batch rendering
	std::vector<SpriteVertex> m_vertices;
	std::vector<GLuint> m_indices;
	GLuint m_currentTexture;
	int m_spriteCount;
	static constexpr int MAX_SPRITES = 1000;
};

} // namespace Sprite
} // namespace CoreLib
