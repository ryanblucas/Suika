/*
	game.h ~ RL
*/

#pragma once

#include "physics.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace game
{
	constexpr int WIDTH = 400;
	constexpr int HEIGHT = 600;

	constexpr float THRESHOLD = 80.f;
	
	enum class object_type
	{
		CHERRY,
		STRAWBERRY,
		GRAPE,
		GRAPEFRUIT,
		ORANGE,
		APPLE,
		CANTALOUPE,
		PEACH,
		PINEAPPLE,
		MELON,
		WATERMELON
	};

	enum class state
	{
		PLAYING,
		END_GAME,
	};

	enum class hovering_state
	{
		HOVERING,
		CREATING,
		WAITING,
	};

	class object
	{
	public:
		virtual bool shown() const { return true; }
		virtual sf::Vector2f position() const { return {}; }
		virtual sf::Vector2f prev_position() const { return {}; }
		virtual object_type type() const { return object_type::CHERRY; }
		virtual void tick(float delta) {}
		virtual void handle_event(sf::Window& window, const std::optional<sf::Event>& event) {}
		virtual bool marked_for_deletion() const { return false; }
		virtual void cleanup() {}
	};

	class fallen_object;
	class hovering_object : public object
	{
	public:
		bool shown() const override
		{
			return m_state != hovering_state::WAITING;
		}
		sf::Vector2f position() const override
		{
			return { m_x, Y_POSITION };
		}
		sf::Vector2f prev_position() const override
		{
			return { m_prev_x, Y_POSITION };
		}
		object_type type() const override
		{
			return m_type;
		}

		hovering_object(float start_x);
		void tick(float delta) override;
		void handle_event(sf::Window& window, const std::optional<sf::Event>& event) override;
	private:
		fallen_object* m_waiting_for;
		int m_ticks_waited;

		hovering_state m_state;
		float m_move_x;
		float m_prev_x;
		float m_x;
		static constexpr float Y_POSITION = 50.f;

		object_type m_type;
		object_type get_random_type();
		void adjust_position_in_bounds();
	};

	struct collision_pair
	{
		std::pair<int, int> objects;
		int tick;

		collision_pair() : objects(), tick(-1) { }

		collision_pair(int id1, int id2, int tick)
		{
			objects.first = std::min(id1, id2);
			objects.second = std::max(id1, id2);
			this->tick = tick;
		}

		bool operator==(collision_pair& other)
		{
			return objects == other.objects && tick == other.tick;
		}
	};

	class fallen_object : public object
	{
	public:
		int phys_id() const
		{
			return m_phys_id;
		}
		object_type type() const override
		{
			return m_type;
		}
		bool marked_for_deletion() const override
		{
			return m_marked_for_deletion;
		}

		fallen_object(object_type type, sf::Vector2f start_pos);
		sf::Vector2f position() const override;
		sf::Vector2f prev_position() const override;
		void handle_collision(fallen_object& other);
		void cleanup() override;
	private:
		collision_pair m_last_collision;
		bool m_marked_for_deletion;
		int m_phys_id;
		object_type m_type;
	};

	void initialize();
	void destroy();
	void tick(float delta);
	void render(sf::RenderTarget& target, float tick_percentage);
	void handle_event(sf::Window& window, const std::optional<sf::Event>& event);
}