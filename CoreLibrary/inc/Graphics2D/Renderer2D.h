#pragma once

#include <glm/glm.hpp>
#include <glad/gl.h>
#include <vector>

namespace CoreLib {
namespace Graphics2D {

// ---------------------------------------------------------------------------
// Color - RGBA color representation
// ---------------------------------------------------------------------------
struct Color
{
	float r, g, b, a;

	Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
	Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

	static Color White() { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
	static Color Black() { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
	static Color Red() { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
	static Color Green() { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
	static Color Blue() { return Color(0.0f, 0.0f, 1.0f, 1.0f); }
	static Color Yellow() { return Color(1.0f, 1.0f, 0.0f, 1.0f); }
	static Color Cyan() { return Color(0.0f, 1.0f, 1.0f, 1.0f); }
	static Color Magenta() { return Color(1.0f, 0.0f, 1.0f, 1.0f); }
	static Color Transparent() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
};

// ---------------------------------------------------------------------------
// Renderer2D - 2D shape rendering system
// ---------------------------------------------------------------------------
class Renderer2D
{
public:
	Renderer2D();
	~Renderer2D();

	// Initialize the renderer
	bool initialize(int screenWidth, int screenHeight);

	// Shutdown and cleanup
	void shutdown();

	// Frame lifecycle
	void begin();
	void end();

	// Viewport
	void setViewport(int x, int y, int width, int height);
	void setScreenSize(int width, int height);

	// Drawing primitives
	void drawLine(const glm::vec2& start, const glm::vec2& end, const Color& color, float thickness = 1.0f);
	void drawRectangle(const glm::vec2& position, const glm::vec2& size, const Color& color, bool filled = true);
	void drawRectangleOutline(const glm::vec2& position, const glm::vec2& size, const Color& color, float thickness = 1.0f);
	void drawCircle(const glm::vec2& center, float radius, const Color& color, bool filled = true, int segments = 32);
	void drawCircleOutline(const glm::vec2& center, float radius, const Color& color, float thickness = 1.0f, int segments = 32);
	void drawTriangle(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const Color& color, bool filled = true);
	void drawPolygon(const std::vector<glm::vec2>& points, const Color& color, bool filled = true);
	void drawPolygonOutline(const std::vector<glm::vec2>& points, const Color& color, float thickness = 1.0f);

	// Advanced shapes
	void drawRoundedRectangle(const glm::vec2& position, const glm::vec2& size, float radius, const Color& color, bool filled = true);
	void drawEllipse(const glm::vec2& center, float radiusX, float radiusY, const Color& color, bool filled = true, int segments = 32);
	void drawArc(const glm::vec2& center, float radius, float startAngle, float endAngle, const Color& color, float thickness = 1.0f, int segments = 32);

	// State
	bool isInitialized() const { return m_initialized; }

private:
	struct Vertex
	{
		glm::vec2 position;
		Color color;
	};

	void createShaders();
	void createBuffers();
	void renderBatch(const std::vector<Vertex>& vertices, GLenum mode);

	bool m_initialized;
	int m_screenWidth;
	int m_screenHeight;
	glm::mat4 m_projection;

	// OpenGL resources
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_shaderProgram;
	GLint m_projectionLoc;
};

} // namespace Graphics2D
} // namespace CoreLib
