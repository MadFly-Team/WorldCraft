#include <Graphics2D/Shapes.h>

namespace CoreLib {
namespace Graphics2D {

// ---------------------------------------------------------------------------
// Rectangle
// ---------------------------------------------------------------------------

Rectangle::Rectangle(const glm::vec2& position, const glm::vec2& size)
	: m_size(size)
{
	m_position = position;
}

void Rectangle::draw(Renderer2D& renderer) const
{
	renderer.drawRectangle(m_position, m_size, m_color, m_filled);
}

// ---------------------------------------------------------------------------
// Circle
// ---------------------------------------------------------------------------

Circle::Circle(const glm::vec2& position, float radius)
	: m_radius(radius)
	, m_segments(32)
{
	m_position = position;
}

void Circle::draw(Renderer2D& renderer) const
{
	renderer.drawCircle(m_position, m_radius, m_color, m_filled, m_segments);
}

// ---------------------------------------------------------------------------
// Line
// ---------------------------------------------------------------------------

Line::Line(const glm::vec2& start, const glm::vec2& end)
	: m_start(start)
	, m_end(end)
	, m_thickness(1.0f)
{
}

void Line::draw(Renderer2D& renderer) const
{
	renderer.drawLine(m_start, m_end, m_color, m_thickness);
}

// ---------------------------------------------------------------------------
// Triangle
// ---------------------------------------------------------------------------

Triangle::Triangle(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3)
	: m_p1(p1)
	, m_p2(p2)
	, m_p3(p3)
{
}

void Triangle::draw(Renderer2D& renderer) const
{
	renderer.drawTriangle(m_p1, m_p2, m_p3, m_color, m_filled);
}

// ---------------------------------------------------------------------------
// Polygon
// ---------------------------------------------------------------------------

Polygon::Polygon(const std::vector<glm::vec2>& points)
	: m_points(points)
{
}

void Polygon::draw(Renderer2D& renderer) const
{
	renderer.drawPolygon(m_points, m_color, m_filled);
}

} // namespace Graphics2D
} // namespace CoreLib
