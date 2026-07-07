/*
	game.cpp ~ RL
*/

#include "game.h"
#include "physics.h"
#include "debug.h"
#include "path.h"
#include <vector>

using namespace game;
using object_variant = std::variant<hovering_object, fallen_object>;

static bool debug_show;
static int tick_count;

static int score;
static physics::world* world;
static std::vector<object_variant> objects;
static float top;
static int top_above_threshold_tick_count;

static state current_state;

static sf::RenderTexture background;
static int background_tick_start;
static sf::Font font;

static object& game_get_base(object_variant& obj)
{
	return std::visit([](auto& derived) -> object& { return derived; }, obj);
}

static const object& game_get_base(const object_variant& obj)
{
	return std::visit([](const auto& derived) -> const object& { return derived; }, obj);
}

static fallen_object* game_get_fallen(object_variant& obj)
{
	return std::visit([](auto&& arg) -> fallen_object*
	{
		if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, fallen_object>)
		{
			return &arg;
		}
		return nullptr;
	}, obj);
}

static hovering_object* game_get_hovering(object_variant& obj)
{
	return std::visit([](auto&& arg) -> hovering_object*
	{
		if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, hovering_object>)
		{
			return &arg;
		}
		return nullptr;
	}, obj);
}

static fallen_object* game_find_from_id(int id)
{
	auto it = std::find_if(objects.begin(), objects.end(), [id](object_variant& obj_variant)
	{
		fallen_object* obj = game_get_fallen(obj_variant);
		return obj && obj->phys_id() == id;
	});
	if (it == objects.end() || game_get_fallen(*it)->phys_id() != id)
	{
		return nullptr;
	}
	return game_get_fallen(*it);
}

void game::initialize()
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	world = new physics::world(WIDTH, HEIGHT);
	objects.reserve(64);
	objects.push_back(hovering_object(WIDTH / 2));
	background = sf::RenderTexture({WIDTH, HEIGHT});
	if (!font.openFromFile(path::find_system_font("cour.ttf")))
	{
		throw std::runtime_error("Failed to find game font.");
	}
}

void game::destroy()
{
	delete world;
	objects.clear();
	score = 0;
}

static void game_check_lose_condition()
{
	top = static_cast<float>(HEIGHT);
	for (auto& obj : objects)
	{
		auto fallen = game_get_fallen(obj);
		if (!fallen)
		{
			continue;
		}
		auto ball = std::as_const(*world).get_ball(fallen->phys_id());
		top = std::min(top, (ball->position() - sf::Vector2f({ 0.f, ball->radius() })).y);
	}
	if (top < THRESHOLD)
	{
		top_above_threshold_tick_count++;
		if (top_above_threshold_tick_count > 40)
		{
			current_state = state::END_GAME;
			background_tick_start = tick_count;
		}
	}
	else
	{
		top_above_threshold_tick_count = 0;
	}
}

static void game_playing_tick(float delta)
{
	world->tick(delta);
	for (auto i = objects.size(); i > 0; i--)
	{
		auto& obj = game_get_base(objects[i - 1]);
		if (obj.marked_for_deletion())
		{
			obj.cleanup();
			objects.erase(objects.begin() + i - 1);
			continue;
		}
		obj.tick(delta);
	}
	game_check_lose_condition();
}

void game::tick(float delta)
{
	if (current_state == state::PLAYING)
	{
		game_playing_tick(delta);
	}
	tick_count++;
}

static float game_radius_from_type(object_type type)
{
	switch (type)
	{
	case object_type::CHERRY: return 25.f;
	case object_type::STRAWBERRY: return 30.f;
	case object_type::GRAPE: return 40.f;
	case object_type::GRAPEFRUIT: return 50.f;
	case object_type::ORANGE: return 60.f;
	case object_type::APPLE: return 65.f;
	case object_type::CANTALOUPE: return 80.f;
	case object_type::PEACH: return 90.f;
	case object_type::PINEAPPLE: return 100.f;
	case object_type::MELON: return 115.f;
	case object_type::WATERMELON: return 120.f;
	}
	return 5.f;
}

static sf::Color game_color_from_type(object_type type)
{
	switch (type)
	{
	case object_type::CHERRY: return sf::Color::Red;
	case object_type::STRAWBERRY: return sf::Color(255, 0, 128);
	case object_type::GRAPE: return sf::Color::Magenta;
	case object_type::GRAPEFRUIT: return sf::Color(128, 0, 255);
	case object_type::ORANGE: return sf::Color(255, 128, 0);
	case object_type::APPLE: return sf::Color(255, 64, 64);
	case object_type::CANTALOUPE: return sf::Color(128, 128, 128);
	case object_type::PEACH: return sf::Color(255, 229, 180);
	case object_type::PINEAPPLE: return sf::Color(254, 234, 99);
	case object_type::MELON: return sf::Color::Green;
	case object_type::WATERMELON: return sf::Color(64, 255, 64);
	}
	return sf::Color::White;
}

static void game_playing_render(sf::RenderTarget& target, float tick_percentage)
{
	if (debug_show)
	{
		debug::render_grid(target, world->get_grid());
		debug::render_line(target, sf::Vector2f({ 0.f, top }), sf::Vector2f({ static_cast<float>(WIDTH), top }));
		debug::render_line(target, sf::Vector2f({ 0.f, THRESHOLD }), sf::Vector2f({ static_cast<float>(WIDTH), THRESHOLD }));
	}
	for (const auto& obj_variant : objects)
	{
		const auto& obj = game_get_base(obj_variant);
		if (!obj.shown())
		{
			continue;
		}
		auto pos = obj.position();
		auto prev_pos = obj.prev_position();
		auto radius = game_radius_from_type(obj.type());

		sf::CircleShape circle(radius);
		circle.setOrigin({ radius, radius });
		circle.setFillColor(game_color_from_type(obj.type()));
		circle.setPosition((pos - prev_pos) * tick_percentage + prev_pos);
		target.draw(circle);
	}
}

void game::render(sf::RenderTarget& target, float tick_percentage)
{
	if (current_state == state::END_GAME)
	{
		background.clear(sf::Color::Black);
		game_playing_render(background, 1.f);
		background.display();

		auto spr = sf::Sprite(background.getTexture());
		auto index = std::max(128, 255 - (tick_count - background_tick_start) * 6);
		spr.setColor(sf::Color(index, index, index));
		target.draw(spr);
		
		sf::Text txt(font, "Game over");
		txt.setPosition(sf::Vector2f({ static_cast<float>(WIDTH) / 2.f - txt.getLocalBounds().size.x / 2.f, 200.f }));
		target.draw(txt);

		txt.setString(std::format("Score - {}", score));
		txt.setCharacterSize(20);
		txt.setPosition(sf::Vector2f({ static_cast<float>(WIDTH) / 2.f - txt.getLocalBounds().size.x / 2.f, 240.f }));
		target.draw(txt);
		return;
	}
	game_playing_render(target, tick_percentage);
	sf::Text txt(font, std::format("Score - {}", score));
	txt.setCharacterSize(20);
	txt.setPosition(sf::Vector2f({ 4, 4 }));
	target.draw(txt);
}

void game::handle_event(sf::Window& window, const std::optional<sf::Event>& event)
{
	auto key = event->getIf<sf::Event::KeyPressed>();
	if (current_state == state::PLAYING)
	{
		for (auto& obj : objects)
		{
			game_get_base(obj).handle_event(window, event);
		}
	}
	else if (key)
	{
		current_state = state::PLAYING;
		destroy();
		initialize();
	}
	if (key && key->shift && key->code == sf::Keyboard::Key::D)
	{
		debug_show = !debug_show;
	}
	else if (key && key->shift && key->code == sf::Keyboard::Key::Q)
	{
		current_state = state::END_GAME;
		background_tick_start = tick_count;
	}
}

hovering_object::hovering_object(float start_x)
{
	m_waiting_for = nullptr;
	m_state = hovering_state::HOVERING;
	m_move_x = 0.f;
	m_x = start_x;
	m_prev_x = 0.f;
	m_type = get_random_type();
}

void hovering_object::tick(float delta)
{
	switch (m_state)
	{
	case hovering_state::HOVERING:
	{
		m_prev_x = m_x;
		m_x += m_move_x * delta * 400.f;
		adjust_position_in_bounds();
		break;
	}
	case hovering_state::CREATING:
	{
		m_prev_x = m_x;
		objects.push_back(fallen_object(m_type, position()));
		m_waiting_for = game_get_fallen(objects.back());
		assert(m_waiting_for);
		m_state = hovering_state::WAITING;
		m_ticks_waited = 0;
		m_type = get_random_type();
		adjust_position_in_bounds();
		
		world->set_ball_collision_handler(m_waiting_for->phys_id(), [](const physics::ball& original, const physics::ball& other)
		{
			auto original_fallen = game_find_from_id(original.get_id());
			auto arg = game_find_from_id(other.get_id());
			if (original_fallen && arg)
			{
				original_fallen->handle_collision(*arg);
			}
		});
		break;
	}
	case hovering_state::WAITING:
	{
		m_ticks_waited++;
		auto ball = m_waiting_for ? std::as_const(*world).get_ball(m_waiting_for->phys_id()) : nullptr;
		if (m_ticks_waited > 20 && (!ball || (ball->position() - ball->prev_position()).lengthSquared() < 200.f))
		{
			m_state = hovering_state::HOVERING;
			m_waiting_for = nullptr;
		}
		break;
	}
	}
}

void hovering_object::handle_event(sf::Window& window, const std::optional<sf::Event>& event)
{
	bool is_down = event->is<sf::Event::KeyPressed>();
	sf::Keyboard::Key code;
	if (is_down)
	{
		code = event->getIf<sf::Event::KeyPressed>()->code;
	}
	else if (event->is<sf::Event::KeyReleased>())
	{
		code = event->getIf<sf::Event::KeyReleased>()->code;
	}
	else
	{
		return;
	}

	if (code == sf::Keyboard::Key::Left)
	{
		m_move_x = is_down ? -1.f : 0.f;
	}
	else if (code == sf::Keyboard::Key::Right)
	{
		m_move_x = is_down ? 1.f : 0.f;
	}
	else if (is_down && code == sf::Keyboard::Key::Space && m_state == hovering_state::HOVERING)
	{
		m_state = hovering_state::CREATING;
	}
}

object_type hovering_object::get_random_type()
{
	auto rand = std::rand() % 3;
	if (rand == 0)
	{
		return object_type::CHERRY;
	}
	else if (rand == 1)
	{
		return object_type::STRAWBERRY;
	}
	return object_type::GRAPE;
}

void hovering_object::adjust_position_in_bounds()
{
	auto rad = game_radius_from_type(m_type);
	if (m_x - rad < 0)
	{
		m_x = rad;
	}
	else if (m_x + rad > world->get_wx())
	{
		m_x = world->get_wx() - rad;
	}
}

fallen_object::fallen_object(object_type type, sf::Vector2f start_pos)
{
	m_type = type;
	m_phys_id = world->add_ball(start_pos, game_radius_from_type(type));
	world->apply_force_on_ball(m_phys_id, sf::Vector2f({ static_cast<float>(std::rand() % 8 - 4), 0.f }));
	m_marked_for_deletion = false;
	m_last_collision = collision_pair();
	m_last_collision.objects.first = m_phys_id;
}

sf::Vector2f fallen_object::position() const
{
	return std::as_const(*world).get_ball(m_phys_id)->position();
}

sf::Vector2f fallen_object::prev_position() const
{
	return std::as_const(*world).get_ball(m_phys_id)->prev_position();
}

void fallen_object::handle_collision(fallen_object& other)
{
	if (m_last_collision == other.m_last_collision || m_type == object_type::WATERMELON || m_type != other.m_type)
	{
		return;
	}
	m_last_collision = collision_pair(m_phys_id, other.m_phys_id, tick_count);
	other.handle_collision(*this);
	if (other.m_phys_id > m_phys_id)
	{
		m_marked_for_deletion = true;
		return;
	}
	m_type = static_cast<object_type>(static_cast<int>(m_type) + 1);
	world->set_ball_radius(m_phys_id, game_radius_from_type(m_type));
	score += static_cast<int>(m_type) * 100;
}

void fallen_object::cleanup()
{
	if (m_marked_for_deletion)
	{
		world->remove_ball(m_phys_id);
	}
}