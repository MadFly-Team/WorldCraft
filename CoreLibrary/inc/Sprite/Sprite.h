#pragma once

#include <Graphics2D/Renderer2D.h>
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <string>
#include <memory>

namespace CoreLib {
namespace Sprite {

// ---------------------------------------------------------------------------
// Texture - 2D texture wrapper
// ---------------------------------------------------------------------------
class Texture
{
public:
	Texture();
	~Texture();

	// Load from file
	bool loadFromFile(const std::string& filepath);

	// Create from data
	bool createFromData(const unsigned char* data, int width, int height, int channels);

	// Bind/unbind
	void bind(int slot = 0) const;
	void unbind() const;

	// Getters
	GLuint getID() const { return m_textureID; }
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	bool isLoaded() const { return m_loaded; }

	// Cleanup
	void destroy();

private:
	GLuint m_textureID;
	int m_width;
	int m_height;
	bool m_loaded;
};

// ---------------------------------------------------------------------------
// Sprite - Single sprite with texture and transform
// ---------------------------------------------------------------------------
class Sprite
{
public:
	Sprite();
	Sprite(std::shared_ptr<Texture> texture);

	// Texture
	void setTexture(std::shared_ptr<Texture> texture) { m_texture = texture; }
	std::shared_ptr<Texture> getTexture() const { return m_texture; }

	// Transform
	void setPosition(const glm::vec2& position) { m_position = position; }
	const glm::vec2& getPosition() const { return m_position; }

	void setScale(const glm::vec2& scale) { m_scale = scale; }
	const glm::vec2& getScale() const { return m_scale; }

	void setRotation(float angle) { m_rotation = angle; }
	float getRotation() const { return m_rotation; }

	void setSize(const glm::vec2& size) { m_size = size; }
	const glm::vec2& getSize() const { return m_size; }

	// Color tint
	void setColor(const Graphics2D::Color& color) { m_color = color; }
	const Graphics2D::Color& getColor() const { return m_color; }

	// UV coordinates (for sprite sheets)
	void setUVRect(const glm::vec4& uvRect) { m_uvRect = uvRect; }
	const glm::vec4& getUVRect() const { return m_uvRect; }

	// Origin/pivot point (0,0 = top-left, 0.5,0.5 = center)
	void setOrigin(const glm::vec2& origin) { m_origin = origin; }
	const glm::vec2& getOrigin() const { return m_origin; }

private:
	std::shared_ptr<Texture> m_texture;
	glm::vec2 m_position;
	glm::vec2 m_scale;
	glm::vec2 m_size;
	glm::vec2 m_origin;
	float m_rotation;
	Graphics2D::Color m_color;
	glm::vec4 m_uvRect; // x, y, width, height in texture space (0-1)

	friend class SpriteRenderer;
};

} // namespace Sprite
} // namespace CoreLib
