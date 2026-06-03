#pragma once

#include <Graphics2D/Renderer2D.h>
#include <glm/glm.hpp>

namespace CoreLib {
namespace Graphics2D {

// ---------------------------------------------------------------------------
// Shape - Base class for 2D shapes
// ---------------------------------------------------------------------------
class Shape
{
public:
	virtual ~Shape() = default;

	virtual void draw(Renderer2D& renderer) const = 0;

	void setPosition(const glm::vec2& pos) { m_position = pos; }
	const glm::vec2& getPosition() const { return m_position; }

	void setColor(const Color& color) { m_color = color; }
	const Color& getColor() const { return m_color; }

	void setFilled(bool filled) { m_filled = filled; }
	bool isFilled() const { return m_filled; }

protected:
	glm::vec2 m_position;
	Color m_color;
	bool m_filled;

	Shape() : m_position(0.0f, 0.0f), m_color(Color::White()), m_filled(true) {}
};

// ---------------------------------------------------------------------------
// Rectangle
// ---------------------------------------------------------------------------
class Rectangle : public Shape
{
public:
	Rectangle(const glm::vec2& position, const glm::vec2& size);

	void setSize(const glm::vec2& size) { m_size = size; }
	const glm::vec2& getSize() const { return m_size; }

	void draw(Renderer2D& renderer) const override;

private:
	glm::vec2 m_size;
};

// ---------------------------------------------------------------------------
// Circle
// ---------------------------------------------------------------------------
class Circle : public Shape
{
public:
	Circle(const glm::vec2& position, float radius);

	void setRadius(float radius) { m_radius = radius; }
	float getRadius() const { return m_radius; }

	void setSegments(int segments) { m_segments = segments; }
	int getSegments() const { return m_segments; }

	void draw(Renderer2D& renderer) const override;

private:
	float m_radius;
	int m_segments;
};

// ---------------------------------------------------------------------------
// Line
// ---------------------------------------------------------------------------
class Line : public Shape
{
public:
	Line(const glm::vec2& start, const glm::vec2& end);

	void setStart(const glm::vec2& start) { m_start = start; }
	const glm::vec2& getStart() const { return m_start; }

	void setEnd(const glm::vec2& end) { m_end = end; }
	const glm::vec2& getEnd() const { return m_end; }

	void setThickness(float thickness) { m_thickness = thickness; }
	float getThickness() const { return m_thickness; }

	void draw(Renderer2D& renderer) const override;

private:
	glm::vec2 m_start;
	glm::vec2 m_end;
	float m_thickness;
};

// ---------------------------------------------------------------------------
// Triangle
// ---------------------------------------------------------------------------
class Triangle : public Shape
{
public:
	Triangle(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3);

	void setPoint1(const glm::vec2& p) { m_p1 = p; }
	void setPoint2(const glm::vec2& p) { m_p2 = p; }
	void setPoint3(const glm::vec2& p) { m_p3 = p; }

	const glm::vec2& getPoint1() const { return m_p1; }
	const glm::vec2& getPoint2() const { return m_p2; }
	const glm::vec2& getPoint3() const { return m_p3; }

	void draw(Renderer2D& renderer) const override;

private:
	glm::vec2 m_p1, m_p2, m_p3;
};

// ---------------------------------------------------------------------------
// Polygon
// ---------------------------------------------------------------------------
class Polygon : public Shape
{
public:
	Polygon(const std::vector<glm::vec2>& points);

	void setPoints(const std::vector<glm::vec2>& points) { m_points = points; }
	const std::vector<glm::vec2>& getPoints() const { return m_points; }

	void addPoint(const glm::vec2& point) { m_points.push_back(point); }
	void clearPoints() { m_points.clear(); }

	void draw(Renderer2D& renderer) const override;

private:
	std::vector<glm::vec2> m_points;
};

} // namespace Graphics2D
} // namespace CoreLib
