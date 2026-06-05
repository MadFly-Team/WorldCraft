#include <Effects/ParticleSystem.h>
#include <glad/gl.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>

namespace Effects {

// Simple particle shader sources
static const char* PARTICLE_VERTEX_SHADER = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aSize;

uniform mat4 uViewProjection;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;

out vec4 vColor;

void main()
{
	gl_Position = uViewProjection * vec4(aPosition, 1.0);
	vColor = aColor;

	// Convert world-space size to screen-space point size
	// Project size based on distance from camera (perspective scaling)
	float distance = length(gl_Position.xyz);
	float pointSize = (aSize * 500.0) / max(distance, 1.0); // Scale factor 500 for visibility
	gl_PointSize = clamp(pointSize, 5.0, 200.0); // Clamp between 5 and 200 pixels
}
)";

static const char* PARTICLE_FRAGMENT_SHADER = R"(
#version 330 core

in vec4 vColor;
out vec4 FragColor;

void main()
{
	// Simple circular particle
	vec2 coord = gl_PointCoord - vec2(0.5);
	float dist = length(coord);
	if (dist > 0.5)
		discard;

	float alpha = vColor.a * (1.0 - dist * 2.0);
	FragColor = vec4(vColor.rgb, alpha);
}
)";

// Random number generator for particle variations
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

ParticleSystem::ParticleSystem()
	: m_vao(0)
	, m_vbo(0)
	, m_shader(0)
	, m_uViewProjection(-1)
	, m_uCameraRight(-1)
	, m_uCameraUp(-1)
	, m_initialized(false)
{
	m_particles.reserve(MAX_PARTICLES);
}

ParticleSystem::~ParticleSystem()
{
	destroy();
}

void ParticleSystem::init()
{
	if (m_initialized)
		return;

	// Compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &PARTICLE_VERTEX_SHADER, nullptr);
	glCompileShader(vertexShader);

	GLint success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		std::cerr << "[ParticleSystem] Vertex shader compilation failed: " << infoLog << std::endl;
	}

	// Compile fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &PARTICLE_FRAGMENT_SHADER, nullptr);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		std::cerr << "[ParticleSystem] Fragment shader compilation failed: " << infoLog << std::endl;
	}

	// Link shader program
	m_shader = glCreateProgram();
	glAttachShader(m_shader, vertexShader);
	glAttachShader(m_shader, fragmentShader);
	glLinkProgram(m_shader);

	glGetProgramiv(m_shader, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(m_shader, 512, nullptr, infoLog);
		std::cerr << "[ParticleSystem] Shader program linking failed: " << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Get uniform locations
	m_uViewProjection = glGetUniformLocation(m_shader, "uViewProjection");
	m_uCameraRight = glGetUniformLocation(m_shader, "uCameraRight");
	m_uCameraUp = glGetUniformLocation(m_shader, "uCameraUp");

	// Create VAO and VBO
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	// Allocate buffer for max particles (position, color, size per particle)
	size_t bufferSize = MAX_PARTICLES * (sizeof(glm::vec3) + sizeof(glm::vec4) + sizeof(float));
	glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) + sizeof(glm::vec4) + sizeof(float), (void*)0);

	// Color attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) + sizeof(glm::vec4) + sizeof(float), (void*)sizeof(glm::vec3));

	// Size attribute
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) + sizeof(glm::vec4) + sizeof(float), (void*)(sizeof(glm::vec3) + sizeof(glm::vec4)));

	glBindVertexArray(0);

	m_initialized = true;
	std::cout << "[ParticleSystem] Initialized successfully" << std::endl;
}

void ParticleSystem::destroy()
{
	if (!m_initialized)
		return;

	if (m_vao)
		glDeleteVertexArrays(1, &m_vao);
	if (m_vbo)
		glDeleteBuffers(1, &m_vbo);
	if (m_shader)
		glDeleteProgram(m_shader);

	m_vao = 0;
	m_vbo = 0;
	m_shader = 0;
	m_initialized = false;
}

void ParticleSystem::update(float deltaTime)
{
	// Update all particles
	for (auto& particle : m_particles)
	{
		// Age the particle
		particle.age += deltaTime;

		// Apply gravity
		particle.velocity.y += GRAVITY * deltaTime;

		// Apply air resistance
		particle.velocity *= AIR_RESISTANCE;

		// Update position
		particle.position += particle.velocity * deltaTime;

		// Update color alpha based on lifetime
		particle.color.a = particle.getAlpha();
	}

	// Remove dead particles
	removeDeadParticles();
}

void ParticleSystem::render(const glm::mat4& viewProjection)
{
	if (!m_initialized)
		return;

	if (m_particles.empty())
		return;

	// Debug: log particle count periodically
	static float debugTimer = 0.0f;
	static auto lastTime = std::chrono::steady_clock::now();
	auto currentTime = std::chrono::steady_clock::now();
	float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
	lastTime = currentTime;
	debugTimer += deltaTime;

	if (debugTimer >= 2.0f)
	{
		std::cout << "[ParticleSystem] Rendering " << m_particles.size() << " particles" << std::endl;
		debugTimer = 0.0f;
	}

	// Save depth state
	GLboolean depthWriteEnabled;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);

	// Enable blending for transparent particles
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_PROGRAM_POINT_SIZE);

	// Keep depth test enabled but disable depth writes (particles shouldn't occlude)
	glDepthMask(GL_FALSE);

	glUseProgram(m_shader);
	glUniformMatrix4fv(m_uViewProjection, 1, GL_FALSE, &viewProjection[0][0]);

	// Extract camera right and up vectors from view matrix (inverse of view)
	glm::vec3 cameraRight(1.0f, 0.0f, 0.0f);
	glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
	glUniform3fv(m_uCameraRight, 1, &cameraRight[0]);
	glUniform3fv(m_uCameraUp, 1, &cameraUp[0]);

	// Prepare particle data for upload
	std::vector<float> vertexData;
	vertexData.reserve(m_particles.size() * 8); // 3 pos + 4 color + 1 size

	for (const auto& particle : m_particles)
	{
		// Position
		vertexData.push_back(particle.position.x);
		vertexData.push_back(particle.position.y);
		vertexData.push_back(particle.position.z);

		// Color
		vertexData.push_back(particle.color.r);
		vertexData.push_back(particle.color.g);
		vertexData.push_back(particle.color.b);
		vertexData.push_back(particle.color.a);

		// Size
		vertexData.push_back(particle.size);
	}

	// Upload particle data
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexData.size() * sizeof(float), vertexData.data());

	// Draw particles as points
	glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_particles.size()));

	glBindVertexArray(0);
	glUseProgram(0);

	// Restore depth state
	glDepthMask(depthWriteEnabled);
	glDisable(GL_BLEND);
	glDisable(GL_PROGRAM_POINT_SIZE);
}

void ParticleSystem::createSplash(const glm::vec3& position, float intensity, const glm::vec3& direction)
{
	// Create splash particles
	int particleCount = static_cast<int>(intensity * 20.0f);
	particleCount = std::clamp(particleCount, 5, 50);

	glm::vec4 waterColor(0.2f, 0.5f, 0.9f, 0.95f); // Bright blue water color with high alpha

	for (int i = 0; i < particleCount; ++i)
	{
		// Random velocity with upward bias
		glm::vec3 velocity(
			dis(gen) * intensity * 2.0f + direction.x,
			std::abs(dis(gen)) * intensity * 3.0f + direction.y * 2.0f,
			dis(gen) * intensity * 2.0f + direction.z
		);

		// MUCH LARGER size for water droplets (10x larger than original!)
		float size = 0.4f + std::abs(dis(gen)) * 0.6f;  // 0.4 to 1.0 block size!

		// Random lifetime
		float lifetime = 0.5f + std::abs(dis(gen)) * 0.5f;

		spawnParticle(position, velocity, waterColor, size, lifetime);
	}

	std::cout << "[ParticleSystem] Created splash with " << particleCount << " particles" << std::endl;
}

void ParticleSystem::createRippleEffect(const glm::vec3& position, float radius)
{
	// Create multiple expanding rings at different radii for cascading ripple effect
	int ringsCount = 3; // Create 3 waves
	int particlesPerRing = 24; // Particles in each ring

	for (int ring = 0; ring < ringsCount; ++ring)
	{
		float ringDelay = ring * 0.15f; // Delay between rings
		float ringRadius = radius * (1.0f + ring * 0.5f); // Each ring starts further out

		// Color fades with each ring
		float alpha = 0.9f - (ring * 0.2f);
		glm::vec4 rippleColor(0.6f, 0.8f, 1.0f, alpha);

		for (int i = 0; i < particlesPerRing; ++i)
		{
			float angle = (static_cast<float>(i) / particlesPerRing) * 2.0f * 3.14159f;

			// Start at a small radius and expand outward
			float startRadius = 0.2f + (ring * 0.3f);
			glm::vec3 startPos = position + glm::vec3(
				std::cos(angle) * startRadius,
				0.05f, // Slightly above water surface
				std::sin(angle) * startRadius
			);

			// Velocity moves particles outward horizontally
			float expandSpeed = 2.5f + (ring * 0.5f); // Faster rings for later waves
			glm::vec3 velocity(
				std::cos(angle) * expandSpeed,
				-0.05f, // Very slight downward to settle on water
				std::sin(angle) * expandSpeed
			);

			// Size grows slightly as ripple expands (larger outer rings)
			float size = 0.25f + (ring * 0.1f) + std::abs(dis(gen)) * 0.15f;

			// Lifetime adjusted so rings fade as they expand
			float lifetime = 1.0f + (ring * 0.2f) + std::abs(dis(gen)) * 0.3f;

			spawnParticle(startPos, velocity, rippleColor, size, lifetime);
		}
	}

	std::cout << "[ParticleSystem] Created ripple with " << (ringsCount * particlesPerRing) << " particles in " << ringsCount << " rings" << std::endl;
}

void ParticleSystem::createDebris(const glm::vec3& position, const glm::vec4& color, int count)
{
	// Create debris particles for block destruction
	count = std::clamp(count, 5, 30);

	for (int i = 0; i < count; ++i)
	{
		// Random velocity in all directions with upward bias
		glm::vec3 velocity(
			dis(gen) * 3.0f,
			std::abs(dis(gen)) * 2.0f + 1.0f, // Always go up
			dis(gen) * 3.0f
		);

		// MUCH LARGER debris particles (0.2 to 0.4 block size!)
		float size = 0.2f + std::abs(dis(gen)) * 0.2f;

		// Shorter lifetime for debris
		float lifetime = 0.4f + std::abs(dis(gen)) * 0.4f;

		// Slightly vary the color
		glm::vec4 debrisColor = color;
		debrisColor.r += dis(gen) * 0.1f;
		debrisColor.g += dis(gen) * 0.1f;
		debrisColor.b += dis(gen) * 0.1f;
		debrisColor = glm::clamp(debrisColor, 0.0f, 1.0f);

		spawnParticle(position, velocity, debrisColor, size, lifetime);
	}

	std::cout << "[ParticleSystem] Created debris with " << count << " particles" << std::endl;
}

void ParticleSystem::clear()
{
	m_particles.clear();
}

void ParticleSystem::spawnParticle(const glm::vec3& position, const glm::vec3& velocity, 
	const glm::vec4& color, float size, float lifetime)
{
	// Don't exceed maximum particle count
	if (m_particles.size() >= MAX_PARTICLES)
		return;

	Particle particle;
	particle.position = position;
	particle.velocity = velocity;
	particle.color = color;
	particle.size = size;
	particle.lifetime = lifetime;
	particle.age = 0.0f;

	m_particles.push_back(particle);
}

void ParticleSystem::removeDeadParticles()
{
	m_particles.erase(
		std::remove_if(m_particles.begin(), m_particles.end(),
			[](const Particle& p) { return !p.isAlive(); }),
		m_particles.end()
	);
}

} // namespace Effects
