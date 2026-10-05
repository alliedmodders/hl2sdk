#ifndef GAME_TIME_H
#define GAME_TIME_H

#ifdef _WIN32
#pragma once
#endif

struct GameTime_t
{
public:
	GameTime_t( float value = 0.0f ) : m_Value( value ) {}

	float GetTime() const { return m_Value; }
	void SetTime( float value ) { m_Value = value; }

	bool operator==( const GameTime_t &other ) const { return m_Value == other.m_Value; }

private:
	float m_Value;
};

#endif // GAME_TIME_H
