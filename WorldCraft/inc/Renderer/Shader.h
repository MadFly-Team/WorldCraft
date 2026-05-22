#pragma once

#include <WorldCraft.h>

namespace Renderer
{
	// Compile a single GLSL shader stage and return its handle.
	// Logs any compile errors to stderr.
	GLuint compileShader(GLenum type, const char* src);

	// Link a vertex and fragment shader into a program and return it.
	// Logs any link errors to stderr.
	GLuint buildProgram(const char* vertSrc, const char* fragSrc);

} // namespace Renderer
