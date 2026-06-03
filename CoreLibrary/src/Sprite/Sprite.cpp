#include <Sprite/Sprite.h>
#include <stb_image.h>
#include <iostream>

namespace CoreLib {
namespace Sprite {

// ---------------------------------------------------------------------------
// Texture
// ---------------------------------------------------------------------------

Texture::Texture()
	: m_textureID(0)
	, m_width(0)
	, m_height(0)
	, m_loaded(false)
{
}

Texture::~Texture()
{
	destroy();
}

bool Texture::loadFromFile(const std::string& filepath)
{
	stbi_set_flip_vertically_on_load(false);

	int channels;
	unsigned char* data = stbi_load(filepath.c_str(), &m_width, &m_height, &channels, 0);

	if (!data)
	{
		std::cerr << "Failed to load texture: " << filepath << std::endl;
		return false;
	}

	bool result = createFromData(data, m_width, m_height, channels);
	stbi_image_free(data);

	if (result)
	{
		std::cout << "Loaded texture: " << filepath << " (" << m_width << "x" << m_height << ", " << channels << " channels)" << std::endl;
	}

	return result;
}

bool Texture::createFromData(const unsigned char* data, int width, int height, int channels)
{
	if (!data)
		return false;

	m_width = width;
	m_height = height;

	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Determine format
	GLenum format = GL_RGB;
	if (channels == 4)
		format = GL_RGBA;
	else if (channels == 1)
		format = GL_RED;

	glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	m_loaded = true;
	return true;
}

void Texture::bind(int slot) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void Texture::unbind() const
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::destroy()
{
	if (m_textureID)
	{
		glDeleteTextures(1, &m_textureID);
		m_textureID = 0;
	}
	m_loaded = false;
}

// ---------------------------------------------------------------------------
// Sprite
// ---------------------------------------------------------------------------

Sprite::Sprite()
	: m_position(0.0f, 0.0f)
	, m_scale(1.0f, 1.0f)
	, m_size(0.0f, 0.0f)
	, m_origin(0.0f, 0.0f)
	, m_rotation(0.0f)
	, m_color(Graphics2D::Color::White())
	, m_uvRect(0.0f, 0.0f, 1.0f, 1.0f)
{
}

Sprite::Sprite(std::shared_ptr<Texture> texture)
	: Sprite()
{
	m_texture = texture;
	if (texture && texture->isLoaded())
	{
		m_size = glm::vec2(static_cast<float>(texture->getWidth()), 
						  static_cast<float>(texture->getHeight()));
	}
}

} // namespace Sprite
} // namespace CoreLib
