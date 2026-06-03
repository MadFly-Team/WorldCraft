#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Effects {

// Single particle data
struct Particle
{
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec4 color;
	float size;
	float lifetime;
	float age;

	Particle()
		: position(0.0f)
		, velocity(0.0f)
		, color(1.0f)
		, size(0.1f)
		, lifetime(1.0f)
		, age(0.0f)
	{}

	bool isAlive() const { return age < lifetime; }
	float getAlpha() const { return 1.0f - (age / lifetime); }
};

// Particle system for water splashes and effects
class ParticleSystem
{
public:
	ParticleSystem();
	~ParticleSystem();

	// Initialize OpenGL resources
	void init();

	// Clean up resources
	void destroy();

	// Update all particles
	void update(float deltaTime);

	// Render all particles
	void render(const glm::mat4& viewProjection);

	// Create a water splash at a position
	void createSplash(const glm::vec3& position, float intensity, const glm::vec3& direction = glm::vec3(0, 1, 0));

	// Create a water ripple particle effect (visual only, complements WaterPhysics ripples)
	void createRippleEffect(const glm::vec3& position, float radius);

	// Create debris particles for block destruction
	void createDebris(const glm::vec3& position, const glm::vec4& color, int count = 12);

	// Clear all particles
	void clear();

	// Get number of active particles
	size_t getParticleCount() const { return m_particles.size(); }

private:
	// Spawn a single particle
	void spawnParticle(const glm::vec3& position, const glm::vec3& velocity, const glm::vec4& color, float size, float lifetime);

	// Remove dead particles
	void removeDeadParticles();

	std::vector<Particle> m_particles;

	// OpenGL resources
	unsigned int m_vao;
	unsigned int m_vbo;
	unsigned int m_shader;

	// Shader locations
	int m_uViewProjection;
	int m_uCameraRight;
	int m_uCameraUp;

	// Constants
	static constexpr size_t MAX_PARTICLES = 2000;
	static constexpr float GRAVITY = -9.81f;
	static constexpr float AIR_RESISTANCE = 0.98f;

	bool m_initialized;
};

} // namespace Effects
