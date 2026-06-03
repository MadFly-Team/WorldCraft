#pragma once

#include <Sprite/Sprite.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

namespace CoreLib {
namespace Sprite {

// ---------------------------------------------------------------------------
// SpriteSheet - Texture atlas with multiple sprites
// ---------------------------------------------------------------------------
class SpriteSheet
{
public:
	struct Frame
	{
		std::string name;
		glm::vec4 uvRect; // x, y, width, height in texture space (0-1)
		glm::vec2 size;   // pixel size
	};

	SpriteSheet();
	SpriteSheet(std::shared_ptr<Texture> texture);

	// Texture
	void setTexture(std::shared_ptr<Texture> texture) { m_texture = texture; }
	std::shared_ptr<Texture> getTexture() const { return m_texture; }

	// Frame management
	void addFrame(const std::string& name, int x, int y, int width, int height);
	void addFrame(const std::string& name, const glm::vec4& uvRect, const glm::vec2& size);
	const Frame* getFrame(const std::string& name) const;
	const Frame* getFrame(int index) const;
	int getFrameCount() const { return static_cast<int>(m_frames.size()); }

	// Grid-based frame generation (for uniform sprite sheets)
	void generateFramesFromGrid(int rows, int cols, int frameWidth, int frameHeight, int spacing = 0);

	// Create sprite from frame
	CoreLib::Sprite::Sprite createSprite(const std::string& frameName) const;
	CoreLib::Sprite::Sprite createSprite(int frameIndex) const;

private:
	std::shared_ptr<Texture> m_texture;
	std::vector<Frame> m_frames;
};

// ---------------------------------------------------------------------------
// Animation - Sprite animation from frames
// ---------------------------------------------------------------------------
class Animation
{
public:
	Animation();

	// Frame sequence
	void addFrame(int frameIndex);
	void setFrames(const std::vector<int>& frames);
	const std::vector<int>& getFrames() const { return m_frames; }
	void clearFrames() { m_frames.clear(); }

	// Timing
	void setFrameDuration(float duration) { m_frameDuration = duration; }
	float getFrameDuration() const { return m_frameDuration; }

	void setLoop(bool loop) { m_loop = loop; }
	bool isLooping() const { return m_loop; }

	// Playback
	void play() { m_playing = true; }
	void pause() { m_playing = false; }
	void stop() { m_playing = false; m_currentFrame = 0; m_elapsed = 0.0f; }
	bool isPlaying() const { return m_playing; }

	void update(float deltaTime);
	int getCurrentFrameIndex() const;

private:
	std::vector<int> m_frames;
	float m_frameDuration;
	bool m_loop;
	bool m_playing;
	int m_currentFrame;
	float m_elapsed;
};

// ---------------------------------------------------------------------------
// AnimatedSprite - Sprite with animation support
// ---------------------------------------------------------------------------
class AnimatedSprite
{
public:
	AnimatedSprite();
	AnimatedSprite(std::shared_ptr<SpriteSheet> spriteSheet);

	// Sprite sheet
	void setSpriteSheet(std::shared_ptr<SpriteSheet> spriteSheet) { m_spriteSheet = spriteSheet; }
	std::shared_ptr<SpriteSheet> getSpriteSheet() const { return m_spriteSheet; }

	// Animation
	void setAnimation(const Animation& animation) { m_animation = animation; }
	Animation& getAnimation() { return m_animation; }
	const Animation& getAnimation() const { return m_animation; }

	// Update
	void update(float deltaTime);

	// Get current sprite
	CoreLib::Sprite::Sprite getCurrentSprite() const;

	// Transform (forwarded to sprite)
	void setPosition(const glm::vec2& position) { m_position = position; }
	const glm::vec2& getPosition() const { return m_position; }

	void setScale(const glm::vec2& scale) { m_scale = scale; }
	const glm::vec2& getScale() const { return m_scale; }

	void setRotation(float angle) { m_rotation = angle; }
	float getRotation() const { return m_rotation; }

	void setColor(const Graphics2D::Color& color) { m_color = color; }
	const Graphics2D::Color& getColor() const { return m_color; }

private:
	std::shared_ptr<SpriteSheet> m_spriteSheet;
	Animation m_animation;
	glm::vec2 m_position;
	glm::vec2 m_scale;
	float m_rotation;
	Graphics2D::Color m_color;
};

} // namespace Sprite
} // namespace CoreLib
