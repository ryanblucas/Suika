/*
	physics.h ~ RL
*/

#pragma once
#define INCLUDE_DEBUG_PHYSICS

#include <SFML/System/Vector2.hpp>
#include <functional>
#include <optional>
#include <unordered_set>
#include <vector>

namespace physics
{
	class ball;
	using collision_handler = std::function<void(const ball& original, const ball& other)>;

	class world;
	class ball
	{
	public:
		ball(world* world, int id, sf::Vector2f position = sf::Vector2f({ 0.f, 0.f }), float radius = 1.f)
			: m_world(world), m_id(id), m_prev_position(position), m_position(position), m_velocity(0.f, 0.f), m_radius(radius)
		{
			m_collision_handler = [](const ball& original, const ball& other) {};
		}

		sf::Vector2f prev_position() const
		{
			return m_prev_position;
		}
		void set_prev_position()
		{
			m_prev_position = m_position;
		}
		sf::Vector2f position() const
		{
			return m_position;
		}
		sf::Vector2f& position()
		{
			return m_position;
		}
		sf::Vector2f velocity() const
		{
			return m_velocity;
		}
		sf::Vector2f& velocity()
		{
			return m_velocity;
		}
		float radius() const
		{
			return m_radius;
		}
		float& radius()
		{
			return m_radius;
		}
		const world& get_world() const
		{
			return *m_world;
		}
		int get_id() const
		{
			return m_id;
		}
		const collision_handler& get_collision_handler() const
		{
			return m_collision_handler;
		}
		void set_collision_handler(collision_handler func)
		{
			m_collision_handler = func;
		}
	private:
		sf::Vector2f m_prev_position;
		sf::Vector2f m_position;
		sf::Vector2f m_velocity;
		float m_radius;

		collision_handler m_collision_handler;
		world* m_world;
		int m_id;
	};

	class grid
	{
	public:
		grid(int wx, int wy, int cell_size, std::vector<ball>* list) : m_wx(wx), m_wy(wy), m_cell_size(cell_size),
			m_cells(wx, std::vector<std::unordered_set<int>>(wy)), m_balls(list) {}

		void recognize_ball(int id);
		void remove_ball(int id);
		// can add ball if not recognized
		void redo_ball(int id);
		std::vector<int> get_balls_neighbors(int id) const;

		void iterate_balls_regions(int id, std::function<void(int x, int y)> callback) const;

		int get_wx() const
		{
			return m_wx;
		}
		int get_wy() const
		{
			return m_wy;
		}
		int get_cell_size() const
		{
			return m_cell_size;
		}

	private:
		int m_wx, m_wy;
		int m_cell_size;
		std::vector<std::vector<std::unordered_set<int>>> m_cells;
		std::vector<ball>* m_balls;

		void add_in_rect(int id, int l, int r, int t, int b);
		void remove_in_rect(int id, int l, int r, int t, int b);

		ball* try_get_ball(int id) const;
		ball& get_ball(int id) const;
		void iterate_through_neighbors(sf::Vector2f pos, float fradius, std::function<void(std::unordered_set<int>&)> callback);
		void iterate_through_neighbors(sf::Vector2f pos, float fradius, std::function<void(const std::unordered_set<int>&)> callback) const;
	};

	class world
	{
	public:
		world(int wx, int wy, float gravity = 980.f, float impulse = 0.3f, float friction = 1.0f) : m_balls(),
			m_grid((wx + GRID_CELL_SIZE - 1) / GRID_CELL_SIZE, (wy + GRID_CELL_SIZE - 1) / GRID_CELL_SIZE, GRID_CELL_SIZE, &m_balls), 
			m_wx(wx), m_wy(wy), m_gravity(gravity), m_impulse(impulse), m_friction(friction) { }

		void tick(float delta);
		
		int add_ball(sf::Vector2f position = { 0.f,0.f }, float radius = 1.f);
		void remove_ball(int id);
		const ball* get_ball(int id) const;
		void apply_force_on_ball(int id, sf::Vector2f vect);
		void set_ball_radius(int id, float radius);
		void set_ball_collision_handler(int id, collision_handler func)
		{
			auto b = get_ball(id);
			b->set_collision_handler(func);
		}

		const std::vector<ball>& get_balls() const
		{
			return m_balls;
		}
		const grid& get_grid() const
		{
			return m_grid;
		}
		int get_wx() const
		{
			return m_wx;
		}
		int get_wy() const
		{
			return m_wy;
		}
		float get_gravity() const
		{
			return m_gravity;
		}
		void set_gravity(float grav)
		{
			m_gravity = grav;
		}

	private:
		static constexpr int GRID_CELL_SIZE = 24;
		std::vector<ball> m_balls;
		grid m_grid;

		int m_wx, m_wy;
		int m_curr_id = 0;
		float m_gravity;
		float m_impulse;
		float m_friction;

		void apply_bound_check(ball& b);
		void apply_gravity(ball& b, float delta);
		void do_collision(ball& b, ball& c);

		ball* get_ball(int id);
	};
}