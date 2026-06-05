#include <Sprite/SpriteSheet.h>
#include <algorithm>

namespace CoreLib {
namespace Sprite {

// ---------------------------------------------------------------------------
// SpriteSheet
// ---------------------------------------------------------------------------

SpriteSheet::SpriteSheet()
{
}

SpriteSheet::SpriteSheet(std::shared_ptr<Texture> texture)
	: m_texture(texture)
{
}

void SpriteSheet::addFrame(const std::string& name, int x, int y, int width, int height)
{
	if (!m_texture || !m_texture->isLoaded())
		return;

	float texWidth = static_cast<float>(m_texture->getWidth());
	float texHeight = static_cast<float>(m_texture->getHeight());

	Frame frame;
	frame.name = name;
	frame.uvRect = glm::vec4(
		x / texWidth,
		y / texHeight,
		width / texWidth,
		height / texHeight
	);
	frame.size = glm::vec2(static_cast<float>(width), static_cast<float>(height));

	m_frames.push_back(frame);
}

void SpriteSheet::addFrame(const std::string& name, const glm::vec4& uvRect, const glm::vec2& size)
{
	Frame frame;
	frame.name = name;
	frame.uvRect = uvRect;
	frame.size = size;
	m_frames.push_back(frame);
}

const SpriteSheet::Frame* SpriteSheet::getFrame(const std::string& name) const
{
	auto it = std::find_if(m_frames.begin(), m_frames.end(),
		[&name](const Frame& f) { return f.name == name; });

	if (it != m_frames.end())
		return &(*it);

	return nullptr;
}

const SpriteSheet::Frame* SpriteSheet::getFrame(int index) const
{
	if (index >= 0 && index < static_cast<int>(m_frames.size()))
		return &m_frames[index];

	return nullptr;
}

void SpriteSheet::generateFramesFromGrid(int rows, int cols, int frameWidth, int frameHeight, int spacing)
{
	m_frames.clear();

	for (int row = 0; row < rows; ++row)
	{
		for (int col = 0; col < cols; ++col)
		{
			int x = col * (frameWidth + spacing);
			int y = row * (frameHeight + spacing);

			std::string frameName = "frame_" + std::to_string(row) + "_" + std::to_string(col);
			addFrame(frameName, x, y, frameWidth, frameHeight);
		}
	}
}

CoreLib::Sprite::Sprite SpriteSheet::createSprite(const std::string& frameName) const
{
	const Frame* frame = getFrame(frameName);
	if (!frame)
		return CoreLib::Sprite::Sprite(m_texture);

	CoreLib::Sprite::Sprite sprite(m_texture);
	sprite.setUVRect(frame->uvRect);
	sprite.setSize(frame->size);
	return sprite;
}

CoreLib::Sprite::Sprite SpriteSheet::createSprite(int frameIndex) const
{
	const Frame* frame = getFrame(frameIndex);
	if (!frame)
		return CoreLib::Sprite::Sprite(m_texture);

	CoreLib::Sprite::Sprite sprite(m_texture);
	sprite.setUVRect(frame->uvRect);
	sprite.setSize(frame->size);
	return sprite;
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

Animation::Animation()
	: m_frameDuration(0.1f)
	, m_loop(true)
	, m_playing(false)
	, m_currentFrame(0)
	, m_elapsed(0.0f)
{
}

void Animation::addFrame(int frameIndex)
{
	m_frames.push_back(frameIndex);
}

void Animation::setFrames(const std::vector<int>& frames)
{
	m_frames = frames;
}

void Animation::update(float deltaTime)
{
	if (!m_playing || m_frames.empty())
		return;

	m_elapsed += deltaTime;

	while (m_elapsed >= m_frameDuration)
	{
		m_elapsed -= m_frameDuration;
		m_currentFrame++;

		if (m_currentFrame >= static_cast<int>(m_frames.size()))
		{
			if (m_loop)
			{
				m_currentFrame = 0;
			}
			else
			{
				m_currentFrame = static_cast<int>(m_frames.size()) - 1;
				m_playing = false;
			}
		}
	}
}

int Animation::getCurrentFrameIndex() const
{
	if (m_currentFrame >= 0 && m_currentFrame < static_cast<int>(m_frames.size()))
		return m_frames[m_currentFrame];

	return 0;
}

// ---------------------------------------------------------------------------
// AnimatedSprite
// ---------------------------------------------------------------------------

AnimatedSprite::AnimatedSprite()
	: m_position(0.0f, 0.0f)
	, m_scale(1.0f, 1.0f)
	, m_rotation(0.0f)
	, m_color(Graphics2D::Color::White())
{
}

AnimatedSprite::AnimatedSprite(std::shared_ptr<SpriteSheet> spriteSheet)
	: AnimatedSprite()
{
	m_spriteSheet = spriteSheet;
}

void AnimatedSprite::update(float deltaTime)
{
	m_animation.update(deltaTime);
}

CoreLib::Sprite::Sprite AnimatedSprite::getCurrentSprite() const
{
	if (!m_spriteSheet)
		return CoreLib::Sprite::Sprite();

	int frameIndex = m_animation.getCurrentFrameIndex();
	CoreLib::Sprite::Sprite sprite = m_spriteSheet->createSprite(frameIndex);

	sprite.setPosition(m_position);
	sprite.setScale(m_scale);
	sprite.setRotation(m_rotation);
	sprite.setColor(m_color);

	return sprite;
}

} // namespace Sprite
} // namespace CoreLib
