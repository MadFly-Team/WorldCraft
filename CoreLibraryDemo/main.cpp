#include <Core/Application.h>
#include <Core/Screen.h>
#include <UI/ImGuiManager.h>
#include <UI/Window.h>
#include <UI/Menu.h>
#include <UI/Dropdown.h>
#include <UI/TextRenderer.h>
#include <Graphics2D/Renderer2D.h>
#include <Graphics2D/Shapes.h>
#include <Sprite/Sprite.h>
#include <Sprite/SpriteRenderer.h>
#include <SDL.h>
#include <iostream>
#include <memory>

using namespace CoreLib;

class DemoApplication : public Core::Application
{
public:
	DemoApplication() 
		: m_showDemoWindow(true)
		, m_show2DShapes(true)
		, m_showUIDemo(true)
		, m_animationTime(0.0f)
	{}

protected:
	void onInitialize() override
	{
		std::cout << "Demo Application Initializing..." << std::endl;

		// Initialize ImGui
		m_imguiManager = std::make_unique<UI::ImGuiManager>();
		m_imguiManager->initialize(getScreen().getWindow(), getScreen().getContext());

		// Initialize 2D Renderer
		m_renderer2D = std::make_unique<Graphics2D::Renderer2D>();
		m_renderer2D->initialize(getScreen().getWidth(), getScreen().getHeight());

		// Initialize Sprite Renderer
		m_spriteRenderer = std::make_unique<Sprite::SpriteRenderer>();
		m_spriteRenderer->initialize(getScreen().getWidth(), getScreen().getHeight());

		// Create demo UI
		m_demoWindow = std::make_unique<UI::Window>("CoreLibrary Demo");
		m_demoWindow->setSize(400, 500);
		m_demoWindow->setPosition(20, 20);

		// Create dropdown
		m_dropdown = std::make_unique<UI::Dropdown>("Select Shape");
		m_dropdown->addItem("Rectangle");
		m_dropdown->addItem("Circle");
		m_dropdown->addItem("Triangle");
		m_dropdown->addItem("Line");
		m_dropdown->addItem("Polygon");
		m_dropdown->setSelectedIndex(0);

		// Create menu
		m_menu = std::make_unique<UI::Menu>();
		setupMenu();

		// Create some shapes
		createShapes();

		std::cout << "Demo Application Initialized!" << std::endl;
	}

	void onShutdown() override
	{
		m_spriteRenderer->shutdown();
		m_renderer2D->shutdown();
		m_imguiManager->shutdown();
	}

	void onUpdate(float deltaTime) override
	{
		m_animationTime += deltaTime;

		// Update shape positions for animation
		updateShapes(deltaTime);
	}

	void onRender() override
	{
		// Render 2D shapes
		if (m_show2DShapes)
		{
			render2DShapes();
		}

		// Render UI
		renderUI();
	}

	void onEvent(const SDL_Event& event) override
	{
		m_imguiManager->processEvent(&event);

		if (event.type == SDL_KEYDOWN)
		{
			if (event.key.keysym.sym == SDLK_F1)
			{
				m_showDemoWindow = !m_showDemoWindow;
			}
			else if (event.key.keysym.sym == SDLK_F2)
			{
				m_show2DShapes = !m_show2DShapes;
			}
			else if (event.key.keysym.sym == SDLK_F3)
			{
				m_showUIDemo = !m_showUIDemo;
			}
		}
	}

private:
	void setupMenu()
	{
		std::vector<UI::Menu::MenuItem> fileMenu;
		fileMenu.push_back(UI::Menu::MenuItem("New", []() { std::cout << "New clicked\n"; }, "Ctrl+N"));
		fileMenu.push_back(UI::Menu::MenuItem("Open", []() { std::cout << "Open clicked\n"; }, "Ctrl+O"));
		fileMenu.push_back(UI::Menu::MenuItem::Separator());
		fileMenu.push_back(UI::Menu::MenuItem("Exit", [this]() { quit(); }, "Alt+F4"));
		m_menu->addMenu("File", fileMenu);

		std::vector<UI::Menu::MenuItem> viewMenu;
		viewMenu.push_back(UI::Menu::MenuItem("Demo Window", [this]() { m_showDemoWindow = !m_showDemoWindow; }, "F1"));
		viewMenu.push_back(UI::Menu::MenuItem("2D Shapes", [this]() { m_show2DShapes = !m_show2DShapes; }, "F2"));
		viewMenu.push_back(UI::Menu::MenuItem("UI Demo", [this]() { m_showUIDemo = !m_showUIDemo; }, "F3"));
		m_menu->addMenu("View", viewMenu);

		std::vector<UI::Menu::MenuItem> helpMenu;
		helpMenu.push_back(UI::Menu::MenuItem("About", []() { std::cout << "CoreLibrary Demo v1.0\n"; }));
		m_menu->addMenu("Help", helpMenu);
	}

	void createShapes()
	{
		// Create animated shapes
		m_rect = std::make_unique<Graphics2D::Rectangle>(glm::vec2(100, 100), glm::vec2(100, 80));
		m_rect->setColor(Graphics2D::Color::Red());
		m_rect->setFilled(true);

		m_circle = std::make_unique<Graphics2D::Circle>(glm::vec2(250, 150), 50.0f);
		m_circle->setColor(Graphics2D::Color::Blue());
		m_circle->setFilled(true);

		m_triangle = std::make_unique<Graphics2D::Triangle>(
			glm::vec2(400, 100),
			glm::vec2(450, 200),
			glm::vec2(350, 200)
		);
		m_triangle->setColor(Graphics2D::Color::Green());
		m_triangle->setFilled(true);

		m_line = std::make_unique<Graphics2D::Line>(glm::vec2(500, 100), glm::vec2(600, 200));
		m_line->setColor(Graphics2D::Color::Yellow());
		m_line->setThickness(3.0f);

		// Create polygon (pentagon)
		std::vector<glm::vec2> points;
		for (int i = 0; i < 5; ++i)
		{
			float angle = (2.0f * 3.14159f * i) / 5.0f - 3.14159f / 2.0f;
			points.push_back(glm::vec2(
				700.0f + std::cos(angle) * 50.0f,
				150.0f + std::sin(angle) * 50.0f
			));
		}
		m_polygon = std::make_unique<Graphics2D::Polygon>(points);
		m_polygon->setColor(Graphics2D::Color::Magenta());
		m_polygon->setFilled(true);
	}

	void updateShapes(float deltaTime)
	{
		// Animate shapes - move them in a circular pattern
		float speed = 0.5f;
		float radius = 20.0f;

		if (m_rect)
		{
			glm::vec2 basePos(100, 100);
			m_rect->setPosition(basePos + glm::vec2(
				std::cos(m_animationTime * speed) * radius,
				std::sin(m_animationTime * speed) * radius
			));
		}

		if (m_circle)
		{
			glm::vec2 basePos(250, 150);
			m_circle->setPosition(basePos + glm::vec2(
				std::cos(m_animationTime * speed + 1.0f) * radius,
				std::sin(m_animationTime * speed + 1.0f) * radius
			));
		}
	}

	void render2DShapes()
	{
		m_renderer2D->begin();

		// Draw shapes
		if (m_rect) m_rect->draw(*m_renderer2D);
		if (m_circle) m_circle->draw(*m_renderer2D);
		if (m_triangle) m_triangle->draw(*m_renderer2D);
		if (m_line) m_line->draw(*m_renderer2D);
		if (m_polygon) m_polygon->draw(*m_renderer2D);

		// Draw some additional shapes
		m_renderer2D->drawRectangleOutline(glm::vec2(50, 300), glm::vec2(150, 100), Graphics2D::Color::Cyan(), 2.0f);
		m_renderer2D->drawCircleOutline(glm::vec2(300, 350), 60.0f, Graphics2D::Color::White(), 2.0f);
		m_renderer2D->drawRoundedRectangle(glm::vec2(400, 300), glm::vec2(180, 120), 15.0f,
										   Graphics2D::Color(0.8f, 0.4f, 0.2f, 1.0f), true);
		m_renderer2D->drawEllipse(glm::vec2(700, 350), 80.0f, 50.0f, Graphics2D::Color(0.5f, 0.8f, 0.3f, 1.0f));

		m_renderer2D->end();
	}

	void renderUI()
	{
		m_imguiManager->newFrame();

		// Menu bar
		if (ImGui::BeginMainMenuBar())
		{
			m_menu->renderMenus();

			ImGui::Separator();
			UI::TextRenderer::textColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
				"FPS: " + std::to_string(static_cast<int>(getFPS())));

			ImGui::EndMainMenuBar();
		}

		// Demo window
		if (m_showDemoWindow)
		{
			renderDemoWindow();
		}

		// UI Demo window
		if (m_showUIDemo)
		{
			renderUIDemo();
		}

		m_imguiManager->render();
	}

	void renderDemoWindow()
	{
		m_demoWindow->begin();

		UI::TextRenderer::heading("CoreLibrary Demo");
		UI::TextRenderer::separator();
		UI::TextRenderer::spacing();

		UI::TextRenderer::subheading("Application Info");
		UI::TextRenderer::labelText("FPS", std::to_string(static_cast<int>(getFPS())));
		UI::TextRenderer::labelText("Frame", std::to_string(getFrameCount()));
		UI::TextRenderer::labelText("Delta Time", std::to_string(getDeltaTime() * 1000.0f) + " ms");

		UI::TextRenderer::spacing(2);
		UI::TextRenderer::subheading("Modules Loaded");
		UI::TextRenderer::bulletText("Core (Application & Screen)");
		UI::TextRenderer::bulletText("UI (ImGui Wrappers)");
		UI::TextRenderer::bulletText("Graphics2D (Shape Rendering)");
		UI::TextRenderer::bulletText("Sprite (Texture & Sprites)");

		UI::TextRenderer::spacing(2);
		UI::TextRenderer::separatorText("Shape Selection");
		m_dropdown->render();

		UI::TextRenderer::spacing(2);
		UI::TextRenderer::subheading("Keyboard Shortcuts");
		UI::TextRenderer::text("F1 - Toggle this window");
		UI::TextRenderer::text("F2 - Toggle 2D shapes");
		UI::TextRenderer::text("F3 - Toggle UI demo");
		UI::TextRenderer::text("ESC - Exit application");

		m_demoWindow->end();
	}

	void renderUIDemo()
	{
		ImGui::SetNextWindowPos(ImVec2(450, 50), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("UI Components Demo", &m_showUIDemo))
		{
			UI::TextRenderer::heading("Text Rendering");
			UI::TextRenderer::text("Regular text");
			UI::TextRenderer::textColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Colored text");
			UI::TextRenderer::textDisabled("Disabled text");
			UI::TextRenderer::bulletText("Bullet point text");

			UI::TextRenderer::spacing(2);
			UI::TextRenderer::separator();
			UI::TextRenderer::spacing();

			static std::string inputText = "Type here...";
			if (UI::TextRenderer::inputText("Text Input", inputText, 128))
			{
				std::cout << "Text changed: " << inputText << std::endl;
			}

			UI::TextRenderer::spacing();

			if (ImGui::Button("Click Me!"))
			{
				std::cout << "Button clicked!" << std::endl;
			}
			UI::TextRenderer::tooltip("This is a button tooltip");

			UI::TextRenderer::spacing(2);
			ImGui::Checkbox("Show 2D Shapes", &m_show2DShapes);
			ImGui::Checkbox("Show Demo Window", &m_showDemoWindow);
		}
		ImGui::End();
	}

	// Members
	std::unique_ptr<UI::ImGuiManager> m_imguiManager;
	std::unique_ptr<Graphics2D::Renderer2D> m_renderer2D;
	std::unique_ptr<Sprite::SpriteRenderer> m_spriteRenderer;
	std::unique_ptr<UI::Window> m_demoWindow;
	std::unique_ptr<UI::Menu> m_menu;
	std::unique_ptr<UI::Dropdown> m_dropdown;

	// Shapes
	std::unique_ptr<Graphics2D::Rectangle> m_rect;
	std::unique_ptr<Graphics2D::Circle> m_circle;
	std::unique_ptr<Graphics2D::Triangle> m_triangle;
	std::unique_ptr<Graphics2D::Line> m_line;
	std::unique_ptr<Graphics2D::Polygon> m_polygon;

	// State
	bool m_showDemoWindow;
	bool m_show2DShapes;
	bool m_showUIDemo;
	float m_animationTime;
};

int main(int argc, char* argv[])
{
	std::cout << "==================================" << std::endl;
	std::cout << "CoreLibrary Demo Application" << std::endl;
	std::cout << "==================================" << std::endl;

	DemoApplication app;

	Core::Screen::Config config;
	config.title = "CoreLibrary Demo - Application Framework";
	config.width = 1280;
	config.height = 720;
	config.vsync = true;
	config.resizable = true;

	if (!app.initialize(config))
	{
		std::cerr << "Failed to initialize application" << std::endl;
		return -1;
	}

	app.run();
	app.shutdown();

	std::cout << "Application exited successfully" << std::endl;
	return 0;
}
