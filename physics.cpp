/*
	physics.cpp ~ RL
*/

#include "physics.h"
#include "debug.h"
#include <algorithm>
#include <stdexcept>

using namespace physics;

void grid::recognize_ball(int id)
{
	auto& ball = get_ball(id);
	if (ball.position().x < 0 || ball.position().y < 0
		|| ball.position().x >= m_wx * m_cell_size || ball.position().y >= m_wy * m_cell_size)
	{
		throw std::out_of_range("Ball position is out of grid bounds");
	}
	iterate_through_neighbors(ball.position(), ball.radius(), [id](std::unordered_set<int>& s) { s.insert(id); });
}

void grid::remove_ball(int id)
{
	auto ball_opt = try_get_ball(id);
	if (!ball_opt)
	{
		return;
	}
	for (int i = 0; i < m_wx; i++)
	{
		for (int j = 0; j < m_wy; j++)
		{
			m_cells[i][j].erase(id);
		}
	}
}

void grid::add_in_rect(int id, int l, int r, int t, int b)
{
	for (int i = l; i <= r; i++)
	{
		for (int j = t; j <= b; j++)
		{
			m_cells[i][j].insert(id);
		}
	}
}

void grid::remove_in_rect(int id, int l, int r, int t, int b)
{
	for (int i = l; i <= r; i++)
	{
		for (int j = t; j <= b; j++)
		{
			m_cells[i][j].erase(id);
		}
	}
}

void grid::redo_ball(int id)
{
	auto& ball = get_ball(id);
	iterate_through_neighbors(ball.prev_position(), ball.radius(), [&id](std::unordered_set<int>& set) { set.erase(id); });
	iterate_through_neighbors(ball.position(), ball.radius(), [&id](std::unordered_set<int>& set) { set.insert(id); });
}

std::vector<int> grid::get_balls_neighbors(int id) const
{
	auto res = std::unordered_set<int>();
	auto& ball = get_ball(id);
	iterate_through_neighbors(ball.position(), ball.radius(), [&res](const std::unordered_set<int>& s) { res.insert(s.begin(), s.end()); });
	res.erase(id);
	return std::vector<int>(res.begin(), res.end());
}

void grid::iterate_balls_regions(int id, std::function<void(int x, int y)> callback) const
{
	for (int i = 0; i < m_wx; i++)
	{
		for (int j = 0; j < m_wy; j++)
		{
			if (m_cells[i][j].contains(id))
			{
				callback(i, j);
			}
		}
	}
}

ball* grid::try_get_ball(int id) const
{
	for (int i = 0; i < m_balls->size(); i++)
	{
		if ((*m_balls)[i].get_id() == id)
		{
			return &(*m_balls)[i];
		}
	}
	return nullptr;
}

ball& grid::get_ball(int id) const
{
	auto res = try_get_ball(id);
	if (res == nullptr)
	{
		throw std::invalid_argument("Ball ID does not reference a real ball in the given list");
	}
	return *res;
}

void grid::iterate_through_neighbors(sf::Vector2f pos, float fradius, std::function<void(std::unordered_set<int>&)> callback)
{
	auto bx = static_cast<int>(pos.x);
	auto by = static_cast<int>(pos.y);
	auto radius = static_cast<int>(fradius);

	auto j_start = std::max(0, (by - radius) / m_cell_size);
	auto i_end = std::min(m_wx - 1, (bx + radius) / m_cell_size);
	auto j_end = std::min(m_wy - 1, (by + radius) / m_cell_size);
	for (int i = std::max(0, (bx - radius) / m_cell_size); i <= i_end; i++)
	{
		for (int j = j_start; j <= j_end; j++)
		{
			callback(m_cells[i][j]);
		}
	}
}

void grid::iterate_through_neighbors(sf::Vector2f pos, float fradius, std::function<void(const std::unordered_set<int>&)> callback) const
{
	auto bx = static_cast<int>(pos.x);
	auto by = static_cast<int>(pos.y);
	auto radius = static_cast<int>(fradius);

	auto j_start = std::max(0, (by - radius) / m_cell_size);
	auto i_end = std::min(m_wx - 1, (bx + radius) / m_cell_size);
	auto j_end = std::min(m_wy - 1, (by + radius) / m_cell_size);
	for (int i = std::max(0, (bx - radius) / m_cell_size); i <= i_end; i++)
	{
		for (int j = j_start; j <= j_end; j++)
		{
			callback(m_cells[i][j]);
		}
	}
}

void world::apply_bound_check(ball& b)
{
	if (b.position().x + b.radius() > m_wx)
	{
		b.position().x = m_wx - b.radius();
		b.velocity().x *= -m_impulse;
	}
	if (b.position().x - b.radius() < 0.f)
	{
		b.position().x = b.radius();
		b.velocity().x *= -m_impulse;
	}
	if (b.position().y + b.radius() > m_wy)
	{
		b.position().y = m_wy - b.radius();
		b.velocity().y *= -m_impulse;
		b.velocity().x *= m_friction;
	}
	if (b.position().y - b.radius() < 0.f)
	{
		b.position().y = b.radius();
		b.velocity().y *= -m_impulse;
	}
}

void world::apply_gravity(ball& b, float delta)
{
	b.velocity().y += delta * m_gravity;
	b.velocity().y = std::min(b.velocity().y, 1000.f);
}

void world::do_collision(ball& b, ball& c)
{
	auto diff = c.position() - b.position();
	auto dist = diff.lengthSquared();
	auto min_dist = b.radius() + c.radius();
	if (dist < min_dist * min_dist)
	{
		dist = std::sqrt(dist);
		if (dist == 0.0)
		{
			return;
		}
		b.get_collision_handler()(b, c);
		c.get_collision_handler()(c, b);

		diff /= dist;
		auto overlap = min_dist - dist;
		b.position() -= diff * overlap * 0.5f;
		c.position() += diff * overlap * 0.5f;
		
		auto rel = (c.velocity() - b.velocity()).dot(diff);
		if (rel > 0)
		{
			return;
		}
		rel *= -m_impulse;
		b.velocity() -= rel * diff;
		c.velocity() += rel * diff;
	}
}

ball* world::get_ball(int id)
{
	for (ball& b : m_balls)
	{
		if (b.get_id() == id)
		{
			return &b;
		}
	}
	return nullptr;
}

void world::tick(float delta)
{
	for (ball& b : m_balls)
	{
		b.set_prev_position();
		apply_gravity(b, delta);
		b.position() += b.velocity() * delta;
	}
	for (int i = 0; i < 4; i++)
	{
		for (ball& b : m_balls)
		{
			apply_bound_check(b);
			auto neighbors = m_grid.get_balls_neighbors(b.get_id());
			for (int id : neighbors)
			{
				ball* c = get_ball(id);
				assert(c != nullptr);
				do_collision(b, *c);
			}

			m_grid.redo_ball(b.get_id());
		}
	}
}

int world::add_ball(sf::Vector2f position, float radius)
{
	if (position.x < 0 || position.y < 0 || position.x >= m_wx || position.y >= m_wy)
	{
		return -1;
	}
	m_balls.push_back(ball(this, m_curr_id, position, radius));
	m_grid.recognize_ball(m_curr_id);
	return m_curr_id++;
}

void world::remove_ball(int id)
{
	// if needed, m_balls and m_rects is always sorted by id because the m_curr_id only increases and add_ball/rectangle
	// always adds to the end, so you could use binary search if it's slow
	auto it = std::find_if(m_balls.begin(), m_balls.end(), [id](const ball& b) { return b.get_id() == id; });
	if (it == m_balls.end() || it->get_id() != id)
	{
		return;
	}
	m_grid.remove_ball(id);
	m_balls.erase(it);
}

const ball* world::get_ball(int id) const
{
	for (const ball& b : m_balls)
	{
		if (b.get_id() == id)
		{
			return &b;
		}
	}
	return nullptr;
}

void world::apply_force_on_ball(int id, sf::Vector2f vect)
{
	auto b = get_ball(id);
	b->velocity() += vect;
}

void world::set_ball_radius(int id, float radius)
{
	radius = std::max(5.f, radius);
	auto b = get_ball(id);
	b->radius() = radius;
	m_grid.remove_ball(id);
	m_grid.recognize_ball(id);
}