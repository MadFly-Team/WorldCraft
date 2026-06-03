# CoreLibrary

A reusable application framework library providing essential components for building desktop applications with SDL2, OpenGL, and Dear ImGui.

## Features

### Core Module
- **Screen Management**: Window creation, OpenGL context initialization, VSync control, fullscreen toggle
- **Application Framework**: Main loop, event handling, delta time calculation, FPS tracking

### UI Module (ImGui Wrappers)
- **Window**: ImGui window wrapper with style configuration and content callbacks
- **Menu**: Menu bar and menu item management with keyboard shortcuts
- **Dropdown**: Combo box and list box wrappers
- **TextRenderer**: Text rendering utilities (headings, colors, tooltips, input fields)
- **ImGuiManager**: ImGui context and backend lifecycle management

### Graphics2D Module
- **Renderer2D**: Batch 2D primitive rendering
  - Lines, rectangles, circles, triangles, polygons
  - Rounded rectangles, ellipses, arcs
  - Filled and outline modes
- **Shapes**: Object-oriented shape classes with draw methods

### Sprite Module
- **Texture**: Image loading and GPU texture management (via stb_image)
- **Sprite**: Single sprite with transform, color tint, UV coordinates
- **SpriteSheet**: Texture atlas with frame definitions
- **Animation**: Frame-based sprite animation with timing and looping
- **AnimatedSprite**: Combines sprite sheet with animation playback
- **SpriteRenderer**: Batched sprite rendering with automatic texture switching

## Building

CoreLibrary is built as a static library using CMake:

```bash
cmake -S . -B build
cmake --build build --target CoreLibrary
```

## Usage

### Basic Application

```cpp
#include <Core/Application.h>
#include <Core/Screen.h>

class MyApp : public CoreLib::Core::Application
{
protected:
	void onInitialize() override {
		// Initialize your resources
	}

	void onUpdate(float deltaTime) override {
		// Update game logic
	}

	void onRender() override {
		// Render your scene
	}

	void onEvent(const SDL_Event& event) override {
		// Handle input events
	}
};

int main(int argc, char* argv[])
{
	MyApp app;

	CoreLib::Core::Screen::Config config;
	config.title = "My Application";
	config.width = 1280;
	config.height = 720;
	config.vsync = true;

	if (!app.initialize(config)) {
		return -1;
	}

	app.run();
	app.shutdown();

	return 0;
}
```

### 2D Rendering

```cpp
#include <Graphics2D/Renderer2D.h>

CoreLib::Graphics2D::Renderer2D renderer;
renderer.initialize(screenWidth, screenHeight);

renderer.begin();
renderer.drawRectangle(glm::vec2(100, 100), glm::vec2(200, 150), 
					   CoreLib::Graphics2D::Color::Red(), true);
renderer.drawCircle(glm::vec2(400, 200), 50, 
					CoreLib::Graphics2D::Color::Blue(), true);
renderer.end();
```

### Sprite Rendering

```cpp
#include <Sprite/Sprite.h>
#include <Sprite/SpriteRenderer.h>

// Load texture
auto texture = std::make_shared<CoreLib::Sprite::Texture>();
texture->loadFromFile("player.png");

// Create sprite
CoreLib::Sprite::Sprite sprite(texture);
sprite.setPosition(glm::vec2(100, 100));
sprite.setSize(glm::vec2(64, 64));

// Render
CoreLib::Sprite::SpriteRenderer renderer;
renderer.initialize(screenWidth, screenHeight);

renderer.begin();
renderer.drawSprite(sprite);
renderer.end();
```

### UI Components

```cpp
#include <UI/ImGuiManager.h>
#include <UI/Window.h>
#include <UI/Menu.h>
#include <UI/TextRenderer.h>

CoreLib::UI::ImGuiManager imgui;
imgui.initialize(window, glContext);

// Create a window
CoreLib::UI::Window myWindow("Settings");
myWindow.setSize(400, 300);

// Create a menu
CoreLib::UI::Menu menu;
std::vector<CoreLib::UI::Menu::MenuItem> fileMenu;
fileMenu.push_back(CoreLib::UI::Menu::MenuItem("New", []() { /* action */ }, "Ctrl+N"));
fileMenu.push_back(CoreLib::UI::Menu::MenuItem("Open", []() { /* action */ }, "Ctrl+O"));
menu.addMenu("File", fileMenu);

imgui.newFrame();

// Render menu bar (option 1: use render() which wraps in BeginMenuBar/EndMenuBar)
menu.render();

// OR render menu bar (option 2: manually control menu bar for more flexibility)
if (ImGui::BeginMainMenuBar())
{
    menu.renderMenus();  // Use this when already inside a menu bar
    ImGui::EndMainMenuBar();
}

// Render window
myWindow.begin();
CoreLib::UI::TextRenderer::heading("Game Settings");
CoreLib::UI::TextRenderer::text("Configure your game");
myWindow.end();

imgui.render();
```

## Demo Application

See `CoreLibraryDemo/main.cpp` for a complete example showcasing all modules.

## Dependencies

- SDL2 (window/event management)
- OpenGL 3.3+ (rendering)
- GLAD (OpenGL loader)
- Dear ImGui (UI)
- GLM (mathematics)
- stb_image (image loading)

## License

Same as parent project.
