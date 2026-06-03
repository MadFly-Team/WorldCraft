#include <Graphics2D/Renderer2D.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

namespace CoreLib {
namespace Graphics2D {

// Simple 2D vertex shader
static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 uProjection;

out vec4 vColor;

void main()
{
	gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
	vColor = aColor;
}
)";

// Simple 2D fragment shader
static const char* fragmentShaderSource = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main()
{
	FragColor = vColor;
}
)";

Renderer2D::Renderer2D()
	: m_initialized(false)
	, m_screenWidth(0)
	, m_screenHeight(0)
	, m_vao(0)
	, m_vbo(0)
	, m_shaderProgram(0)
	, m_projectionLoc(-1)
{
}

Renderer2D::~Renderer2D()
{
	shutdown();
}

bool Renderer2D::initialize(int screenWidth, int screenHeight)
{
	if (m_initialized)
	{
		std::cerr << "Renderer2D already initialized" << std::endl;
		return false;
	}

	m_screenWidth = screenWidth;
	m_screenHeight = screenHeight;

	createShaders();
	createBuffers();

	// Create orthographic projection matrix
	m_projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 
							  static_cast<float>(screenHeight), 0.0f, 
							  -1.0f, 1.0f);

	m_initialized = true;
	std::cout << "Renderer2D initialized: " << screenWidth << "x" << screenHeight << std::endl;
	return true;
}

void Renderer2D::shutdown()
{
	if (!m_initialized)
		return;

	if (m_vbo) glDeleteBuffers(1, &m_vbo);
	if (m_vao) glDeleteVertexArrays(1, &m_vao);
	if (m_shaderProgram) glDeleteProgram(m_shaderProgram);

	m_initialized = false;
}

void Renderer2D::begin()
{
	glUseProgram(m_shaderProgram);
	glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, &m_projection[0][0]);
}

void Renderer2D::end()
{
	glUseProgram(0);
}

void Renderer2D::setViewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

void Renderer2D::setScreenSize(int width, int height)
{
	m_screenWidth = width;
	m_screenHeight = height;
	m_projection = glm::ortho(0.0f, static_cast<float>(width), 
							  static_cast<float>(height), 0.0f, 
							  -1.0f, 1.0f);
}

void Renderer2D::drawLine(const glm::vec2& start, const glm::vec2& end, const Color& color, float thickness)
{
	glLineWidth(thickness);

	std::vector<Vertex> vertices = {
		{ start, color },
		{ end, color }
	};

	renderBatch(vertices, GL_LINES);
	glLineWidth(1.0f);
}

void Renderer2D::drawRectangle(const glm::vec2& position, const glm::vec2& size, const Color& color, bool filled)
{
	if (filled)
	{
		std::vector<Vertex> vertices = {
			{ position, color },
			{ position + glm::vec2(size.x, 0), color },
			{ position + size, color },
			{ position + glm::vec2(0, size.y), color }
		};
		renderBatch(vertices, GL_TRIANGLE_FAN);
	}
	else
	{
		drawRectangleOutline(position, size, color, 1.0f);
	}
}

void Renderer2D::drawRectangleOutline(const glm::vec2& position, const glm::vec2& size, const Color& color, float thickness)
{
	glLineWidth(thickness);

	std::vector<Vertex> vertices = {
		{ position, color },
		{ position + glm::vec2(size.x, 0), color },
		{ position + size, color },
		{ position + glm::vec2(0, size.y), color },
		{ position, color }
	};

	renderBatch(vertices, GL_LINE_STRIP);
	glLineWidth(1.0f);
}

void Renderer2D::drawCircle(const glm::vec2& center, float radius, const Color& color, bool filled, int segments)
{
	std::vector<Vertex> vertices;
	vertices.reserve(segments + 1);

	if (filled)
	{
		vertices.push_back({ center, color });
	}

	for (int i = 0; i <= segments; ++i)
	{
		float angle = (2.0f * glm::pi<float>() * i) / segments;
		glm::vec2 point = center + glm::vec2(std::cos(angle), std::sin(angle)) * radius;
		vertices.push_back({ point, color });
	}

	renderBatch(vertices, filled ? GL_TRIANGLE_FAN : GL_LINE_STRIP);
}

void Renderer2D::drawCircleOutline(const glm::vec2& center, float radius, const Color& color, float thickness, int segments)
{
	glLineWidth(thickness);
	drawCircle(center, radius, color, false, segments);
	glLineWidth(1.0f);
}

void Renderer2D::drawTriangle(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const Color& color, bool filled)
{
	std::vector<Vertex> vertices = {
		{ p1, color },
		{ p2, color },
		{ p3, color }
	};

	if (!filled)
	{
		vertices.push_back({ p1, color });
		renderBatch(vertices, GL_LINE_STRIP);
	}
	else
	{
		renderBatch(vertices, GL_TRIANGLES);
	}
}

void Renderer2D::drawPolygon(const std::vector<glm::vec2>& points, const Color& color, bool filled)
{
	if (points.size() < 3)
		return;

	std::vector<Vertex> vertices;
	vertices.reserve(points.size());

	for (const auto& point : points)
	{
		vertices.push_back({ point, color });
	}

	renderBatch(vertices, filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
}

void Renderer2D::drawPolygonOutline(const std::vector<glm::vec2>& points, const Color& color, float thickness)
{
	glLineWidth(thickness);
	drawPolygon(points, color, false);
	glLineWidth(1.0f);
}

void Renderer2D::drawRoundedRectangle(const glm::vec2& position, const glm::vec2& size, float radius, const Color& color, bool filled)
{
	// Simplified rounded rectangle using circles at corners
	radius = std::min(radius, std::min(size.x, size.y) * 0.5f);

	std::vector<Vertex> vertices;
	const int segments = 8;

	glm::vec2 corners[4] = {
		position + glm::vec2(radius, radius),
		position + glm::vec2(size.x - radius, radius),
		position + glm::vec2(size.x - radius, size.y - radius),
		position + glm::vec2(radius, size.y - radius)
	};

	float startAngles[4] = { glm::pi<float>(), 1.5f * glm::pi<float>(), 0.0f, 0.5f * glm::pi<float>() };

	for (int c = 0; c < 4; ++c)
	{
		for (int i = 0; i <= segments; ++i)
		{
			float angle = startAngles[c] + (glm::pi<float>() * 0.5f * i) / segments;
			glm::vec2 point = corners[c] + glm::vec2(std::cos(angle), std::sin(angle)) * radius;
			vertices.push_back({ point, color });
		}
	}

	renderBatch(vertices, filled ? GL_TRIANGLE_FAN : GL_LINE_STRIP);
}

void Renderer2D::drawEllipse(const glm::vec2& center, float radiusX, float radiusY, const Color& color, bool filled, int segments)
{
	std::vector<Vertex> vertices;
	vertices.reserve(segments + 1);

	if (filled)
	{
		vertices.push_back({ center, color });
	}

	for (int i = 0; i <= segments; ++i)
	{
		float angle = (2.0f * glm::pi<float>() * i) / segments;
		glm::vec2 point = center + glm::vec2(std::cos(angle) * radiusX, std::sin(angle) * radiusY);
		vertices.push_back({ point, color });
	}

	renderBatch(vertices, filled ? GL_TRIANGLE_FAN : GL_LINE_STRIP);
}

void Renderer2D::drawArc(const glm::vec2& center, float radius, float startAngle, float endAngle, const Color& color, float thickness, int segments)
{
	glLineWidth(thickness);

	std::vector<Vertex> vertices;
	vertices.reserve(segments + 1);

	for (int i = 0; i <= segments; ++i)
	{
		float t = static_cast<float>(i) / segments;
		float angle = startAngle + (endAngle - startAngle) * t;
		glm::vec2 point = center + glm::vec2(std::cos(angle), std::sin(angle)) * radius;
		vertices.push_back({ point, color });
	}

	renderBatch(vertices, GL_LINE_STRIP);
	glLineWidth(1.0f);
}

void Renderer2D::createShaders()
{
	// Compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);

	// Check vertex shader compilation
	GLint success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
	}

	// Compile fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);

	// Check fragment shader compilation
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
	}

	// Link shaders
	m_shaderProgram = glCreateProgram();
	glAttachShader(m_shaderProgram, vertexShader);
	glAttachShader(m_shaderProgram, fragmentShader);
	glLinkProgram(m_shaderProgram);

	// Check linking
	glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(m_shaderProgram, 512, nullptr, infoLog);
		std::cerr << "Shader program linking failed: " << infoLog << std::endl;
	}

	// Clean up shaders
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Get uniform location
	m_projectionLoc = glGetUniformLocation(m_shaderProgram, "uProjection");
}

void Renderer2D::createBuffers()
{
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

	// Color attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

	glBindVertexArray(0);
}

void Renderer2D::renderBatch(const std::vector<Vertex>& vertices, GLenum mode)
{
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size()));
	glBindVertexArray(0);
}

} // namespace Graphics2D
} // namespace CoreLib
