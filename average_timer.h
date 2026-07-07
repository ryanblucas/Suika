/*
	average_timer.h ~ RL
*/

#pragma once

#include <format>
#include <string>

class average_timer
{
public:
	average_timer() : m_description(""), m_target(1.0), m_accumlator(0.0), m_sample_count(0), m_recent_average(0.0) { }
	average_timer(std::string_view descripton, double target) : m_description(descripton), m_target(target), m_accumlator(0.0), m_sample_count(0), m_recent_average(0.0) { }

	bool update(double delta)
	{
		m_accumlator += delta;
		m_sample_count++;
		if (m_accumlator > m_target)
		{
			m_recent_average = m_accumlator / m_sample_count;
			m_accumlator = 0.0;
			m_sample_count = 0;
			return true;
		}
		return false;
	}
	std::string caption() const
	{
		return std::format("{:.0f}{}", 1 / m_recent_average, m_description);
	}
	double recent_average() const
	{
		return m_recent_average;
	}
private:
	std::string m_description;
	double m_accumlator;
	int m_sample_count;
	double m_recent_average;
	double m_target;
};