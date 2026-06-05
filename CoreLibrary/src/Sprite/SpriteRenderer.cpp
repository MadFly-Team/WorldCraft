#include <Sprite/SpriteRenderer.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

namespace CoreLib {
namespace Sprite {

// Sprite vertex shader
static const char* spriteVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

uniform mat4 uProjection;

out vec2 vTexCoord;
out vec4 vColor;

void main()
{
	gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
	vTexCoord = aTexCoord;
	vColor = aColor;
}
)";

// Sprite fragment shader
static const char* spriteFragmentShaderSource = R"(
#version 330 core
in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D uTexture;

out vec4 FragColor;

void main()
{
	vec4 texColor = texture(uTexture, vTexCoord);
	FragColor = texColor * vColor;
}
)";

SpriteRenderer::SpriteRenderer()
	: m_initialized(false)
	, m_screenWidth(0)
	, m_screenHeight(0)
	, m_vao(0)
	, m_vbo(0)
	, m_ebo(0)
	, m_shaderProgram(0)
	, m_projectionLoc(-1)
	, m_textureLoc(-1)
	, m_currentTexture(0)
	, m_spriteCount(0)
{
}

SpriteRenderer::~SpriteRenderer()
{
	shutdown();
}

bool SpriteRenderer::initialize(int screenWidth, int screenHeight)
{
	if (m_initialized)
	{
		std::cerr << "SpriteRenderer already initialized" << std::endl;
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

	// Reserve space for batch rendering
	m_vertices.reserve(MAX_SPRITES * 4);
	m_indices.reserve(MAX_SPRITES * 6);

	m_initialized = true;
	std::cout << "SpriteRenderer initialized: " << screenWidth << "x" << screenHeight << std::endl;
	return true;
}

void SpriteRenderer::shutdown()
{
	if (!m_initialized)
		return;

	if (m_ebo) glDeleteBuffers(1, &m_ebo);
	if (m_vbo) glDeleteBuffers(1, &m_vbo);
	if (m_vao) glDeleteVertexArrays(1, &m_vao);
	if (m_shaderProgram) glDeleteProgram(m_shaderProgram);

	m_initialized = false;
}

void SpriteRenderer::begin()
{
	glUseProgram(m_shaderProgram);
	glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, &m_projection[0][0]);
	glUniform1i(m_textureLoc, 0);
}

void SpriteRenderer::end()
{
	flush();
	glUseProgram(0);
}

void SpriteRenderer::flush()
{
	if (m_vertices.empty())
		return;

	renderBatch();

	m_vertices.clear();
	m_indices.clear();
	m_spriteCount = 0;
	m_currentTexture = 0;
}

void SpriteRenderer::setScreenSize(int width, int height)
{
	m_screenWidth = width;
	m_screenHeight = height;
	m_projection = glm::ortho(0.0f, static_cast<float>(width), 
							  static_cast<float>(height), 0.0f, 
							  -1.0f, 1.0f);
}

void SpriteRenderer::beginBatch()
{
	m_vertices.clear();
	m_indices.clear();
	m_spriteCount = 0;
}

void SpriteRenderer::endBatch()
{
	flush();
}

void SpriteRenderer::drawSprite(const CoreLib::Sprite::Sprite& sprite)
{
	if (!sprite.getTexture() || !sprite.getTexture()->isLoaded())
		return;

	// Flush if texture changes or batch is full
	GLuint textureID = sprite.getTexture()->getID();
	if ((m_currentTexture != 0 && m_currentTexture != textureID) || m_spriteCount >= MAX_SPRITES)
	{
		flush();
	}
	m_currentTexture = textureID;

	// Calculate sprite transform
	glm::vec2 size = sprite.getSize() * sprite.getScale();
	glm::vec2 origin = sprite.getOrigin() * size;
	float rotation = sprite.getRotation();

	// Calculate corners
	glm::vec2 corners[4] = {
		glm::vec2(0.0f, 0.0f),
		glm::vec2(size.x, 0.0f),
		glm::vec2(size.x, size.y),
		glm::vec2(0.0f, size.y)
	};

	// Apply rotation
	if (rotation != 0.0f)
	{
		float cosR = std::cos(rotation);
		float sinR = std::sin(rotation);

		for (int i = 0; i < 4; ++i)
		{
			glm::vec2 p = corners[i] - origin;
			corners[i] = glm::vec2(
				p.x * cosR - p.y * sinR,
				p.x * sinR + p.y * cosR
			) + sprite.getPosition();
		}
	}
	else
	{
		for (int i = 0; i < 4; ++i)
		{
			corners[i] = corners[i] - origin + sprite.getPosition();
		}
	}

	// UV coordinates
	glm::vec4 uv = sprite.getUVRect();
	glm::vec2 uvCoords[4] = {
		glm::vec2(uv.x, uv.y),
		glm::vec2(uv.x + uv.z, uv.y),
		glm::vec2(uv.x + uv.z, uv.y + uv.w),
		glm::vec2(uv.x, uv.y + uv.w)
	};

	// Add vertices
	int baseVertex = static_cast<int>(m_vertices.size());
	for (int i = 0; i < 4; ++i)
	{
		SpriteVertex vertex;
		vertex.position = corners[i];
		vertex.texCoord = uvCoords[i];
		vertex.color = sprite.getColor();
		m_vertices.push_back(vertex);
	}

	// Add indices
	m_indices.push_back(baseVertex + 0);
	m_indices.push_back(baseVertex + 1);
	m_indices.push_back(baseVertex + 2);
	m_indices.push_back(baseVertex + 0);
	m_indices.push_back(baseVertex + 2);
	m_indices.push_back(baseVertex + 3);

	m_spriteCount++;
}

void SpriteRenderer::drawSprite(std::shared_ptr<Texture> texture, const glm::vec2& position, 
								 const glm::vec2& size, const Graphics2D::Color& color)
{
	CoreLib::Sprite::Sprite sprite(texture);
	sprite.setPosition(position);
	sprite.setSize(size);
	sprite.setColor(color);
	drawSprite(sprite);
}

void SpriteRenderer::createShaders()
{
	// Compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &spriteVertexShaderSource, nullptr);
	glCompileShader(vertexShader);

	// Check compilation
	GLint success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		std::cerr << "Sprite vertex shader compilation failed: " << infoLog << std::endl;
	}

	// Compile fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &spriteFragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		std::cerr << "Sprite fragment shader compilation failed: " << infoLog << std::endl;
	}

	// Link program
	m_shaderProgram = glCreateProgram();
	glAttachShader(m_shaderProgram, vertexShader);
	glAttachShader(m_shaderProgram, fragmentShader);
	glLinkProgram(m_shaderProgram);

	glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(m_shaderProgram, 512, nullptr, infoLog);
		std::cerr << "Sprite shader program linking failed: " << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Get uniform locations
	m_projectionLoc = glGetUniformLocation(m_shaderProgram, "uProjection");
	m_textureLoc = glGetUniformLocation(m_shaderProgram, "uTexture");
}

void SpriteRenderer::createBuffers()
{
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ebo);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, MAX_SPRITES * 4 * sizeof(SpriteVertex), nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_SPRITES * 6 * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, position));

	// TexCoord attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, texCoord));

	// Color attribute
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, color));

	glBindVertexArray(0);
}

void SpriteRenderer::renderBatch()
{
	if (m_vertices.empty() || m_indices.empty())
		return;

	glBindTexture(GL_TEXTURE_2D, m_currentTexture);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(SpriteVertex), m_vertices.data());

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indices.size() * sizeof(GLuint), m_indices.data());

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace Sprite
} // namespace CoreLib
