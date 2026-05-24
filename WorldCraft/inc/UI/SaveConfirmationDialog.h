#pragma once

#include <string>

namespace UI
{

class SaveConfirmationDialog
{
public:
	SaveConfirmationDialog() = default;

	void show(int chunksCount);
	void render();
	bool isOpen() const { return m_isOpen; }

private:
	bool m_isOpen = false;
	int m_chunksSaved = 0;
	float m_displayTimer = 0.0f;
	static constexpr float DISPLAY_DURATION = 2.0f;  // Show for 2 seconds
};

} // namespace UI
