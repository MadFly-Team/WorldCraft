#include <Renderer/WireframeCube.h>
#include <cstdio>

namespace Renderer
{

// Wireframe cube vertex shader
static const char* wireframeVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uMVP;
uniform vec3 uOffset;  // Block position offset

void main()
{
	vec3 worldPos = aPos + uOffset;
	gl_Position = uMVP * vec4(worldPos, 1.0);
}
)";

// Wireframe cube fragment shader
static const char* wireframeFragmentShader = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 uColor;

void main()
{
	FragColor = uColor;
}
)";

WireframeCube::WireframeCube()
{
}

WireframeCube::~WireframeCube()
{
	cleanup();
}

void WireframeCube::init()
{
	createShader();
	createGeometry();
}

void WireframeCube::cleanup()
{
	if (m_vao) glDeleteVertexArrays(1, &m_vao);
	if (m_vbo) glDeleteBuffers(1, &m_vbo);
	if (m_shader) glDeleteProgram(m_shader);
	m_vao = m_vbo = m_shader = 0;
	m_mvpLoc = m_colorLoc = m_offsetLoc = -1;
}

void WireframeCube::createShader()
{
	// Compile vertex shader
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &wireframeVertexShader, nullptr);
	glCompileShader(vs);

	GLint success;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char log[512];
		glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
		printf("[WireframeCube] Vertex shader compile error: %s\n", log);
	}

	// Compile fragment shader
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &wireframeFragmentShader, nullptr);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char log[512];
		glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
		printf("[WireframeCube] Fragment shader compile error: %s\n", log);
	}

	// Link shader program
	m_shader = glCreateProgram();
	glAttachShader(m_shader, vs);
	glAttachShader(m_shader, fs);
	glLinkProgram(m_shader);

	glGetProgramiv(m_shader, GL_LINK_STATUS, &success);
	if (!success)
	{
		char log[512];
		glGetProgramInfoLog(m_shader, sizeof(log), nullptr, log);
		printf("[WireframeCube] Shader link error: %s\n", log);
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	// Get uniform locations
	m_mvpLoc    = glGetUniformLocation(m_shader, "uMVP");
	m_colorLoc  = glGetUniformLocation(m_shader, "uColor");
	m_offsetLoc = glGetUniformLocation(m_shader, "uOffset");
}

void WireframeCube::createGeometry()
{
	// Wireframe cube: 12 edges, 2 vertices per edge = 24 vertices
	// Cube spans from (0,0,0) to (1,1,1) — will be offset by block position
	// Slightly expand by 0.001 units to prevent z-fighting with block faces

	const float eps = 0.001f; // Expand slightly to sit outside block faces
	const float min = -eps;
	const float max = 1.0f + eps;

	float vertices[] = {
		// Bottom face (Y = 0) - 4 edges
		min, min, min,  max, min, min,  // Edge 0
		max, min, min,  max, min, max,  // Edge 1
		max, min, max,  min, min, max,  // Edge 2
		min, min, max,  min, min, min,  // Edge 3

		// Top face (Y = 1) - 4 edges
		min, max, min,  max, max, min,  // Edge 4
		max, max, min,  max, max, max,  // Edge 5
		max, max, max,  min, max, max,  // Edge 6
		min, max, max,  min, max, min,  // Edge 7

		// Vertical edges - 4 edges
		min, min, min,  min, max, min,  // Edge 8
		max, min, min,  max, max, min,  // Edge 9
		max, min, max,  max, max, max,  // Edge 10
		min, min, max,  min, max, max,  // Edge 11
	};

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void WireframeCube::render(const glm::ivec3& blockPos, const glm::mat4& mvp, const glm::vec4& color)
{
	if (m_shader == 0 || m_vao == 0)
		return;

	glUseProgram(m_shader);
	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, &mvp[0][0]);
	glUniform4fv(m_colorLoc, 1, &color[0]);
	glUniform3f(m_offsetLoc, static_cast<float>(blockPos.x),
								static_cast<float>(blockPos.y),
								static_cast<float>(blockPos.z));

	// Enable alpha blending for semi-transparent wireframe
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Disable depth write but keep depth test so wireframe appears over geometry
	glDepthMask(GL_FALSE);

	// Make the line thicker for better visibility
	glLineWidth(3.0f);

	// Render as lines
	glBindVertexArray(m_vao);
	glDrawArrays(GL_LINES, 0, 24);
	glBindVertexArray(0);

	// Restore defaults
	glLineWidth(1.0f);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

} // namespace Renderer
