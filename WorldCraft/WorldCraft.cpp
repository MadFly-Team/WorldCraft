// WorldCraft.cpp : Defines the entry point for the application.
//

#include "WorldCraft.h"

// ---------------------------------------------------------------------------
// Window dimensions
// ---------------------------------------------------------------------------
static constexpr int SCREEN_W = 1024;
static constexpr int SCREEN_H = 768;

// ---------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------
static const char* vertSrc = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 uMVP;

out vec3 vColor;

void main()
{
	gl_Position = uMVP * vec4(aPos, 1.0);
	vColor = aColor;
}
)glsl";

static const char* fragSrc = R"glsl(
#version 330 core
in  vec3 vColor;
out vec4 FragColor;

void main()
{
	FragColor = vec4(vColor, 1.0);
}
)glsl";

// ---------------------------------------------------------------------------
// Cube geometry  (6 faces * 2 tris * 3 verts = 36 vertices)
// Each vertex: position(xyz) + color(rgb)
// ---------------------------------------------------------------------------
static const float cubeVerts[] = {
	// Back face  (blue)
	-0.5f, -0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	 0.5f,  0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	 0.5f, -0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	 0.5f,  0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	-0.5f, -0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	-0.5f,  0.5f, -0.5f,  0.20f, 0.35f, 0.80f,
	// Front face (cyan)
	-0.5f, -0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	 0.5f, -0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	 0.5f,  0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	 0.5f,  0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	-0.5f,  0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	-0.5f, -0.5f,  0.5f,  0.20f, 0.80f, 0.80f,
	// Left face  (green)
	-0.5f,  0.5f,  0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f,  0.5f, -0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f, -0.5f, -0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f, -0.5f, -0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f, -0.5f,  0.5f,  0.20f, 0.75f, 0.30f,
	-0.5f,  0.5f,  0.5f,  0.20f, 0.75f, 0.30f,
	// Right face (orange)
	 0.5f,  0.5f,  0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f, -0.5f, -0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f,  0.5f, -0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f, -0.5f, -0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f,  0.5f,  0.5f,  0.90f, 0.55f, 0.10f,
	 0.5f, -0.5f,  0.5f,  0.90f, 0.55f, 0.10f,
	// Bottom face (red)
	-0.5f, -0.5f, -0.5f,  0.85f, 0.20f, 0.20f,
	 0.5f, -0.5f, -0.5f,  0.85f, 0.20f, 0.20f,
	 0.5f, -0.5f,  0.5f,  0.85f, 0.20f, 0.20f,
	 0.5f, -0.5f,  0.5f,  0.85f, 0.20f, 0.20f,
	-0.5f, -0.5f,  0.5f,  0.85f, 0.20f, 0.20f,
	-0.5f, -0.5f, -0.5f,  0.85f, 0.20f, 0.20f,
	// Top face   (yellow)
	-0.5f,  0.5f, -0.5f,  0.90f, 0.85f, 0.10f,
	 0.5f,  0.5f,  0.5f,  0.90f, 0.85f, 0.10f,
	 0.5f,  0.5f, -0.5f,  0.90f, 0.85f, 0.10f,
	 0.5f,  0.5f,  0.5f,  0.90f, 0.85f, 0.10f,
	-0.5f,  0.5f, -0.5f,  0.90f, 0.85f, 0.10f,
	-0.5f,  0.5f,  0.5f,  0.90f, 0.85f, 0.10f,
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static GLuint compileShader(GLenum type, const char* src)
{
	GLuint id = glCreateShader(type);
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int ok;
	glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(id, 512, nullptr, log);
		std::cerr << "Shader compile error:\n" << log << "\n";
	}
	return id;
}

static GLuint buildProgram(const char* vs, const char* fs)
{
	GLuint vert = compileShader(GL_VERTEX_SHADER,   vs);
	GLuint frag = compileShader(GL_FRAGMENT_SHADER, fs);

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);

	int ok;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetProgramInfoLog(prog, 512, nullptr, log);
		std::cerr << "Program link error:\n" << log << "\n";
	}

	glDeleteShader(vert);
	glDeleteShader(frag);
	return prog;
}

static void framebufferSizeCB(GLFWwindow*, int w, int h)
{
	glViewport(0, 0, w, h);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main()
{
	// --- GLFW init ---
	if (!glfwInit()) {
		std::cerr << "Failed to initialise GLFW\n";
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(SCREEN_W, SCREEN_H, "WorldCraft", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCB);

	// --- GLAD init ---
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		std::cerr << "Failed to initialise GLAD\n";
		return -1;
	}

	glViewport(0, 0, SCREEN_W, SCREEN_H);
	glEnable(GL_DEPTH_TEST);

	// --- Shader program ---
	GLuint shaderProg = buildProgram(vertSrc, fragSrc);
	GLint  mvpLoc     = glGetUniformLocation(shaderProg, "uMVP");

	// --- VAO / VBO ---
	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

	// position attribute  (location 0, 3 floats, stride 6 floats, offset 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);

	// colour attribute    (location 1, 3 floats, stride 6 floats, offset 3 floats)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	// --- Projection matrix (fixed for this demo) ---
	glm::mat4 projection = glm::perspective(
		glm::radians(45.0f),
		static_cast<float>(SCREEN_W) / static_cast<float>(SCREEN_H),
		0.1f, 100.0f
	);

	glm::mat4 view = glm::lookAt(
		glm::vec3(1.8f, 1.4f, 2.5f),  // camera position
		glm::vec3(0.0f, 0.0f, 0.0f),  // look at origin
		glm::vec3(0.0f, 1.0f, 0.0f)   // up vector
	);

	// --- Render loop ---
	while (!glfwWindowShouldClose(window))
	{
		// ESC to quit
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		// Dark grey background  (#222222)
		glClearColor(0.133f, 0.133f, 0.133f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Rotate around a diagonal axis over time
		float     angle = static_cast<float>(glfwGetTime());
		glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.5f, 1.0f, 0.3f));
		glm::mat4 mvp   = projection * view * model;

		glUseProgram(shaderProg);
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// --- Cleanup ---
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(shaderProg);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
