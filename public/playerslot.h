#ifndef PLAYERSLOT_H
#define PLAYERSLOT_H

#if _WIN32
#pragma once
#endif

#include "const.h"

class CPlayerSlot
{
public:
	CPlayerSlot( int slot ) : m_Data( slot ) {}

	void Invalidate() { m_Data = -1; }
	bool IsValid() const { return m_Data >= 0 && m_Data < ABSOLUTE_PLAYER_LIMIT; }
	int Get() const { return m_Data; }

	bool operator==( const CPlayerSlot &other ) const { return other.m_Data == m_Data; }
	bool operator!=( const CPlayerSlot &other ) const { return other.m_Data != m_Data; }

private:
	int m_Data;
};

#endif // PLAYERSLOT_H