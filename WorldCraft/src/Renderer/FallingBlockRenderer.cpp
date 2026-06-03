#include <Renderer/FallingBlockRenderer.h>
#include <Renderer/Shader.h>
#include <Voxel/BlockRegistry.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Renderer
{

// Simple vertex shader for falling block cubes
static const char* kFallingBlockVertexShader = R"(
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in float aTexLayer;

uniform mat4 uMVP;

out vec2 vTexCoord;
out float vTexLayer;

void main()
{
	gl_Position = uMVP * vec4(aPos, 1.0);
	vTexCoord = aTexCoord;
	vTexLayer = aTexLayer;
}
)";

// Simple fragment shader for falling block cubes
static const char* kFallingBlockFragmentShader = R"(
#version 330 core

in vec2 vTexCoord;
in float vTexLayer;

uniform sampler2DArray uTexArray;
uniform vec4 uTintColor;

out vec4 FragColor;

void main()
{
	vec4 texColor = texture(uTexArray, vec3(vTexCoord, vTexLayer));
	if (texColor.a < 0.1)
		discard;
	FragColor = texColor * uTintColor;
}
)";

FallingBlockRenderer::FallingBlockRenderer()
	: m_vao(0)
	, m_vbo(0)
	, m_shaderProgram(0)
	, m_mvpLoc(-1)
	, m_texLayerLoc(-1)
	, m_tintColorLoc(-1)
	, m_batchActive(false)
{
}

FallingBlockRenderer::~FallingBlockRenderer()
{
	destroy();
}

void FallingBlockRenderer::init()
{
	buildShader();
	buildCubeMesh();
}

void FallingBlockRenderer::destroy()
{
	if (m_vao)
	{
		glDeleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}
	if (m_vbo)
	{
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
	}
	if (m_shaderProgram)
	{
		glDeleteProgram(m_shaderProgram);
		m_shaderProgram = 0;
	}
}

void FallingBlockRenderer::buildShader()
{
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShader, 1, &kFallingBlockVertexShader, nullptr);
	glCompileShader(vertShader);

	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &kFallingBlockFragmentShader, nullptr);
	glCompileShader(fragShader);

	m_shaderProgram = glCreateProgram();
	glAttachShader(m_shaderProgram, vertShader);
	glAttachShader(m_shaderProgram, fragShader);
	glLinkProgram(m_shaderProgram);

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	// Get uniform locations
	m_mvpLoc = glGetUniformLocation(m_shaderProgram, "uMVP");
	m_texLayerLoc = glGetUniformLocation(m_shaderProgram, "uTexArray");
	m_tintColorLoc = glGetUniformLocation(m_shaderProgram, "uTintColor");
}

void FallingBlockRenderer::buildCubeMesh()
{
	// Cube mesh data (6 floats per vertex: pos x3, uv x2, texLayer x1)
	// We'll update texLayer per-block in the render call
	float vertices[] = {
		// Positions          // TexCoords
		// Front face (+Z)
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,

		// Back face (-Z)
		 0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,

		// Top face (+Y)
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,

		// Bottom face (-Y)
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,

		// Right face (+X)
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,

		// Left face (-X)
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
	};

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// TexLayer attribute
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

void FallingBlockRenderer::renderBlock(const glm::vec3& pos, Voxel::BlockID blockType,
									   float rotation, const glm::mat4& view, const glm::mat4& proj,
									   float scale)
{
	beginBatch(view, proj);
	renderBlockBatch(pos, blockType, rotation, scale);
	endBatch();
}

void FallingBlockRenderer::beginBatch(const glm::mat4& view, const glm::mat4& proj)
{
	m_batchActive = true;
	m_batchView = view;
	m_batchProj = proj;

	glUseProgram(m_shaderProgram);
	glBindVertexArray(m_vao);

	// Texture array is bound externally (same as chunk rendering)
	glUniform1i(m_texLayerLoc, 0);

	// Enable depth testing for solid blocks
	glEnable(GL_DEPTH_TEST);

	// Enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void FallingBlockRenderer::renderBlockBatch(const glm::vec3& pos, Voxel::BlockID blockType,
											float rotation, float scale, bool underwater)
{
	if (!m_batchActive)
		return;

	// Get texture layers for each face of this block type
	const Voxel::BlockRegistry& registry = Voxel::BlockRegistry::get();
	float texTop    = static_cast<float>(registry.texLayer(blockType, Voxel::FaceDir::PosY));
	float texBottom = static_cast<float>(registry.texLayer(blockType, Voxel::FaceDir::NegY));
	float texNorth  = static_cast<float>(registry.texLayer(blockType, Voxel::FaceDir::PosZ));
	float texSouth  = static_cast<float>(registry.texLayer(blockType, Voxel::FaceDir::NegZ));
	float texEast   = static_cast<float>(registry.texLayer(blockType, Voxel::FaceDir::PosX));
	float texWest   = static_cast<float>(registry.texLayer(blockType, Voxel::FaceDir::NegX));

	// Set tint color (blue tint for underwater, white for normal)
	if (underwater)
	{
		// Blue-ish tint for underwater blocks
		glUniform4f(m_tintColorLoc, 0.5f, 0.7f, 1.0f, 1.0f);
	}
	else
	{
		// No tint (white = multiply by 1)
		glUniform4f(m_tintColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	// Build model matrix: translate, rotate, scale
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, pos);
	model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(scale));

	glm::mat4 mvp = m_batchProj * m_batchView * model;
	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

	// Build cube with correct texture on each face
	float cubeData[] = {
		// Front face (PosZ / North) - flip V to fix grass orientation
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, texNorth,
		 0.5f, -0.5f,  0.5f,  1.0f, 1.0f, texNorth,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, texNorth,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, texNorth,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, texNorth,
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, texNorth,
		// Back face (NegZ / South) - flip V to fix grass orientation
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f, texSouth,
		-0.5f, -0.5f, -0.5f,  1.0f, 1.0f, texSouth,
		-0.5f,  0.5f, -0.5f,  1.0f, 0.0f, texSouth,
		-0.5f,  0.5f, -0.5f,  1.0f, 0.0f, texSouth,
		 0.5f,  0.5f, -0.5f,  0.0f, 0.0f, texSouth,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f, texSouth,
		// Top face (PosY)
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, texTop,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, texTop,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, texTop,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, texTop,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, texTop,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, texTop,
		// Bottom face (NegY)
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, texBottom,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, texBottom,
		 0.5f, -0.5f,  0.5f,  1.0f, 1.0f, texBottom,
		 0.5f, -0.5f,  0.5f,  1.0f, 1.0f, texBottom,
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, texBottom,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, texBottom,
		// Right face (PosX / East) - flip V to fix grass orientation
		 0.5f, -0.5f,  0.5f,  0.0f, 1.0f, texEast,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f, texEast,
		 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, texEast,
		 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, texEast,
		 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, texEast,
		 0.5f, -0.5f,  0.5f,  0.0f, 1.0f, texEast,
		// Left face (NegX / West) - flip V to fix grass orientation
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, texWest,
		-0.5f, -0.5f,  0.5f,  1.0f, 1.0f, texWest,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f, texWest,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f, texWest,
		-0.5f,  0.5f, -0.5f,  0.0f, 0.0f, texWest,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, texWest,
	};

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cubeData), cubeData);

	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void FallingBlockRenderer::endBatch()
{
	glBindVertexArray(0);
	glUseProgram(0);
	glDisable(GL_BLEND);
	m_batchActive = false;
}

} // namespace Renderer
