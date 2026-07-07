/*
	main.cpp ~ RL
*/

#include <chrono>
#include <thread>
#include <sfml/Graphics.hpp>
#include "average_timer.h"
#include "game.h"
#include "physics.h"
#include "debug.h"
#ifdef _WIN32
#include <Windows.h>
#endif

static void accurate_sleep(std::chrono::microseconds length)
{
	auto start = std::chrono::steady_clock::now();
	auto target = start + length;

	auto bulk = length - std::chrono::milliseconds(2);
	if (bulk > std::chrono::microseconds::zero())
	{
		std::this_thread::sleep_for(bulk);
	}

	while (std::chrono::steady_clock::now() < target);
}

static void update_title(sf::Window& window, const average_timer& fps, const average_timer& tps)
{
	window.setTitle(std::format("Suika - {}, {}", fps.caption(), tps.caption()));
}

int main()
{
#ifdef _WIN32
	bool used_period = timeBeginPeriod(1) == TIMERR_NOERROR;
	if (!used_period)
	{
		debug::println("Failed to specify an accurate timer... fps may be inconsistent");
	}
#endif

	using namespace std::chrono_literals;

	constexpr std::chrono::microseconds rate = std::chrono::microseconds(1'000'000 / 60);
	constexpr double tick_rate = std::chrono::microseconds(1'000'000 / 24) / 1.0s;

	sf::RenderWindow window(sf::VideoMode({ game::WIDTH, game::HEIGHT }), "Suika", sf::Style::Titlebar | sf::Style::Close);

	game::initialize();

	auto time_since_last_tick = 0.0;

	average_timer fps("fps", 1.0), tps("tps", 1.0);
	auto start = std::chrono::steady_clock::now();
	auto prev = start - rate;
	while (window.isOpen())
	{
		double delta = (start - prev) / 1.0s;
		time_since_last_tick += delta;
		if (fps.update(delta))
		{
			update_title(window, fps, tps);
		}

		while (std::optional<sf::Event> event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
			{
				window.close();
				continue;
			}
			game::handle_event(window, event);
		}
		
		for (int i = 0; i < 5 && time_since_last_tick > tick_rate; i++)
		{
			if (tps.update(time_since_last_tick))
			{
				update_title(window, fps, tps);
			}
			game::tick(static_cast<float>(time_since_last_tick));
			time_since_last_tick -= tick_rate;
		}
		window.clear(sf::Color::Black);
		game::render(window, static_cast<float>(time_since_last_tick / tick_rate));
		window.display();

		accurate_sleep(std::chrono::duration_cast<std::chrono::microseconds>(start + rate - std::chrono::steady_clock::now()));
		prev = start;
		start = std::chrono::steady_clock::now();
	}
	game::destroy();
#ifdef _WIN32
	if (used_period)
	{
		timeEndPeriod(1);
	}
#endif
	return 0;
}