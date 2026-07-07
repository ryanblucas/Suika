/*
	debug.cpp ~ RL
*/

#include "physics.h"
#include "debug.h"
#include "path.h"
#include <iostream>

using namespace debug;

static std::chrono::steady_clock::time_point profiler_starts[16];
static int profiler_curr;

static sf::Font debug_font;
static bool initialized;

void debug::print(std::string_view str)
{
	std::cout << str;
}

void debug::println(std::string_view str)
{
	std::cout << str << std::endl;
}

void debug::profiler_push()
{
	profiler_starts[profiler_curr] = std::chrono::steady_clock::now();
	profiler_curr++;
}

void debug::profiler_pop(std::string_view desc)
{
	profiler_curr--;
	auto duration = std::chrono::steady_clock::now() - profiler_starts[profiler_curr];
	println(std::format("{} took {}s.", desc, static_cast<double>(duration.count()) / 1'000'000'000.0));
}

void debug::render_initialize()
{
	if (initialized)
	{
		return;
	}
	if (!debug_font.openFromFile(path::find_system_font("arial.ttf")))
	{
		throw std::runtime_error("Failed to find debug font.");
	}
	initialized = true;
}

void debug::render_line(sf::RenderTarget& target, sf::Vector2f start, sf::Vector2f end, sf::Color color)
{
	sf::VertexArray arr(sf::PrimitiveType::Lines, 2);
	arr[0].position = start;
	arr[0].color = color;
	arr[1].position = end;
	arr[1].color = color;
	target.draw(arr);
}

void debug::render_grid(sf::RenderTarget& target, const physics::grid& grid)
{
	auto color = sf::Color(128, 128, 128);
	float width = static_cast<float>(grid.get_wx() * grid.get_cell_size());
	float height = static_cast<float>(grid.get_wy() * grid.get_cell_size());
	sf::VertexArray arr(sf::PrimitiveType::Lines, 2 * (grid.get_wx() + grid.get_wy()));
	for (int i = 0; i < grid.get_wx(); i++)
	{
		arr[i * 2].position = { static_cast<float>(i * grid.get_cell_size()), 0.f };
		arr[i * 2 + 1].position = { static_cast<float>(i * grid.get_cell_size()), height };
		arr[i * 2].color = color;
		arr[i * 2 + 1].color = color;
	}
	int offset = grid.get_wx() * 2;
	for (int i = 0; i < grid.get_wy(); i++)
	{
		arr[offset + i * 2].position = { 0.f, static_cast<float>(i * grid.get_cell_size()) };
		arr[offset + i * 2 + 1].position = { width, static_cast<float>(i * grid.get_cell_size()) };
		arr[offset + i * 2].color = color;
		arr[offset + i * 2 + 1].color = color;
	}
	target.draw(arr);
}

void debug::render_balls_cells(sf::RenderTarget& target, const physics::grid& grid, const physics::ball& ball)
{
	sf::RectangleShape rect({ static_cast<float>(grid.get_cell_size()), static_cast<float>(grid.get_cell_size()) });
	auto color = sf::Color::Red;
	color.a = 128;
	rect.setFillColor(color);
	grid.iterate_balls_regions(ball.get_id(), [&target, &rect, &grid](int x, int y)
	{
		rect.setPosition({static_cast<float>(x * grid.get_cell_size()), static_cast<float>(y * grid.get_cell_size())});
		target.draw(rect);
	});
}

void debug::render_ball_details(sf::RenderTarget& target, const physics::ball& ball)
{
	render_initialize();
	
	sf::Text str(debug_font);
	str.setString(std::format("pos: ({}, {})\nvel: ({}, {})", ball.position().x, ball.position().y, ball.velocity().x, ball.velocity().y));
	str.setCharacterSize(12);
	str.setFillColor(sf::Color::White);
	str.setPosition(ball.position() + sf::Vector2f({ ball.radius(), -ball.radius() }));

	target.draw(str);
}