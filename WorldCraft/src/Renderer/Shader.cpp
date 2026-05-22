#include <Renderer/Shader.h>

namespace Renderer
{

GLuint compileShader(GLenum type, const char* src)
{
	GLuint id = glCreateShader(type);
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int ok;
	glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		char log[512];
		glGetShaderInfoLog(id, 512, nullptr, log);
		std::cerr << "Shader compile error:\n" << log << "\n";
	}
	return id;
}

GLuint buildProgram(const char* vertSrc, const char* fragSrc)
{
	GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
	GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);

	int ok;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok)
	{
		char log[512];
		glGetProgramInfoLog(prog, 512, nullptr, log);
		std::cerr << "Program link error:\n" << log << "\n";
	}

	glDeleteShader(vert);
	glDeleteShader(frag);
	return prog;
}

} // namespace Renderer
