#ifndef PLAYERSLOT_H
#define PLAYERSLOT_H

#if _WIN32
#pragma once
#endif

class CPlayerSlot
{
public:
	CPlayerSlot(int slot) : m_Data(slot)
	{
	}

	int Get() const
	{
		return m_Data;
	}

	bool operator==(const CPlayerSlot &other) const {
		return other.m_Data == m_Data;
	}
	bool operator!=(const CPlayerSlot &other) const {
		return other.m_Data != m_Data;
	}

private:
	int m_Data;
};

#endif // PLAYERSLOT_H