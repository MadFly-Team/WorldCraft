#include <Renderer/ItemEntityRenderer.h>
#include <Renderer/Shader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Renderer
{

// Simple vertex shader for item cubes
static const char* kItemVertexShader = R"(
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

// Simple fragment shader for item cubes
static const char* kItemFragmentShader = R"(
#version 330 core

in vec2 vTexCoord;
in float vTexLayer;

uniform sampler2DArray uTexArray;

out vec4 FragColor;

void main()
{
	vec4 texColor = texture(uTexArray, vec3(vTexCoord, vTexLayer));
	if (texColor.a < 0.1)
		discard;
	FragColor = texColor;
}
)";

ItemEntityRenderer::ItemEntityRenderer()
	: m_vao(0)
	, m_vbo(0)
	, m_shaderProgram(0)
	, m_mvpLoc(-1)
	, m_texLayerLoc(-1)
	, m_batchActive(false)
{
}

ItemEntityRenderer::~ItemEntityRenderer()
{
	destroy();
}

void ItemEntityRenderer::init()
{
	buildShader();
	buildCubeMesh();
}

void ItemEntityRenderer::destroy()
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

void ItemEntityRenderer::buildShader()
{
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShader, 1, &kItemVertexShader, nullptr);
	glCompileShader(vertShader);

	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &kItemFragmentShader, nullptr);
	glCompileShader(fragShader);

	m_shaderProgram = glCreateProgram();
	glAttachShader(m_shaderProgram, vertShader);
	glAttachShader(m_shaderProgram, fragShader);
	glLinkProgram(m_shaderProgram);

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	m_mvpLoc = glGetUniformLocation(m_shaderProgram, "uMVP");
	m_texLayerLoc = glGetUniformLocation(m_shaderProgram, "uTexArray");
}

void ItemEntityRenderer::buildCubeMesh()
{
	// Simple cube vertices: position (3) + texCoord (2) + texLayer (1) = 6 floats per vertex
	// 36 vertices for a cube (6 faces * 2 triangles * 3 vertices)

	// We'll generate this at render time for now with texture layers based on block type
	// For simplicity, we'll use a single texture layer for all faces of the item cube

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
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// TexLayer attribute (will be set per-instance, but we include it in the layout)
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

void ItemEntityRenderer::renderItem(const glm::vec3& pos, Voxel::BlockID blockType,
									 float rotation, const glm::mat4& view, const glm::mat4& proj,
									 float scale)
{
	beginBatch(view, proj);
	renderItemBatch(pos, blockType, rotation, scale);
	endBatch();
}

void ItemEntityRenderer::beginBatch(const glm::mat4& view, const glm::mat4& proj)
{
	m_batchActive = true;
	m_batchView = view;
	m_batchProj = proj;

	glUseProgram(m_shaderProgram);
	glBindVertexArray(m_vao);

	// Texture array is bound externally (same as chunk rendering)
	glUniform1i(m_texLayerLoc, 0);

	// Enable blending for semi-transparent items (if any)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void ItemEntityRenderer::renderItemBatch(const glm::vec3& pos, Voxel::BlockID blockType,
										  float rotation, float scale)
{
	if (!m_batchActive)
		return;

	// Get texture layer for this block type (use top face as representative)
	const Voxel::BlockRegistry& registry = Voxel::BlockRegistry::get();
	int texLayer = registry.texLayer(blockType, Voxel::FaceDir::PosY);

	// Build model matrix: translate, rotate, scale
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, pos);
	model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(scale));

	glm::mat4 mvp = m_batchProj * m_batchView * model;
	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

	// Update texture layer for all vertices (hacky but simple)
	// For proper instancing, we'd use a uniform or instance attribute
	// For now, we'll just update the VBO data
	std::vector<float> vertices(36 * 6);

	// Simple cube with the texture layer set
	float cubeData[] = {
		// Front face
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, (float)texLayer,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, (float)texLayer,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, (float)texLayer,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, (float)texLayer,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, (float)texLayer,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, (float)texLayer,
		// (repeat for all 6 faces...)
		// Back
		 0.5f, -0.5f, -0.5f,  0.0f, 0.0f, (float)texLayer,
		-0.5f, -0.5f, -0.5f,  1.0f, 0.0f, (float)texLayer,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f, (float)texLayer,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f, (float)texLayer,
		 0.5f,  0.5f, -0.5f,  0.0f, 1.0f, (float)texLayer,
		 0.5f, -0.5f, -0.5f,  0.0f, 0.0f, (float)texLayer,
		// Top
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, (float)texLayer,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, (float)texLayer,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, (float)texLayer,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, (float)texLayer,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, (float)texLayer,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, (float)texLayer,
		// Bottom
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, (float)texLayer,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, (float)texLayer,
		 0.5f, -0.5f,  0.5f,  1.0f, 1.0f, (float)texLayer,
		 0.5f, -0.5f,  0.5f,  1.0f, 1.0f, (float)texLayer,
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, (float)texLayer,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, (float)texLayer,
		// Right
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, (float)texLayer,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, (float)texLayer,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, (float)texLayer,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, (float)texLayer,
		 0.5f,  0.5f,  0.5f,  0.0f, 1.0f, (float)texLayer,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, (float)texLayer,
		// Left
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, (float)texLayer,
		-0.5f, -0.5f,  0.5f,  1.0f, 0.0f, (float)texLayer,
		-0.5f,  0.5f,  0.5f,  1.0f, 1.0f, (float)texLayer,
		-0.5f,  0.5f,  0.5f,  1.0f, 1.0f, (float)texLayer,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, (float)texLayer,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, (float)texLayer,
	};

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cubeData), cubeData);

	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void ItemEntityRenderer::endBatch()
{
	glBindVertexArray(0);
	glUseProgram(0);
	glDisable(GL_BLEND);
	m_batchActive = false;
}

} // namespace Renderer
