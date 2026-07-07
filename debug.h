/*
	debug.h ~ RL
*/

#pragma once

#include <cassert>
#include <format>
#include <string_view>
#include <SFML/Graphics.hpp>

namespace debug
{
	void print(std::string_view str);
	void println(std::string_view str);

	void profiler_push();
	void profiler_pop(std::string_view desc);

	void render_initialize();
	void render_line(sf::RenderTarget& target, sf::Vector2f start, sf::Vector2f end, sf::Color color = sf::Color::White);

#ifdef INCLUDE_DEBUG_PHYSICS
	void render_grid(sf::RenderTarget& target, const physics::grid& grid);
	void render_balls_cells(sf::RenderTarget& target, const physics::grid& grid, const physics::ball& ball);
	void render_ball_details(sf::RenderTarget& target, const physics::ball& ball);
#endif
}