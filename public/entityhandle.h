//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYHANDLE_H
#define ENTITYHANDLE_H
#ifdef _WIN32
#pragma once
#endif

#include "const.h"
#include "ihandleentity.h"


class CEntityHandle
{
public:
	CEntityHandle();
	CEntityHandle(const CEntityHandle& other);
	CEntityHandle(uint32 value);
	CEntityHandle(int iEntry, int iSerialNumber);

	void Init(int iEntry, int iSerialNumber);
	void Term();

	bool IsValid() const;

	int GetEntryIndex() const;
	int GetSerialNumber() const;

	bool operator !=(const CEntityHandle& other) const;
	bool operator ==(const CEntityHandle& other) const;
	bool operator ==(const IHandleEntity* pEnt) const;
	bool operator !=(const IHandleEntity* pEnt) const;
	bool operator <(const CEntityHandle& other) const;
	bool operator <(const IHandleEntity* pEnt) const;

	// Assign a value to the handle.
	const CEntityHandle& operator=(const IHandleEntity* pEntity);
	const CEntityHandle& Set(const IHandleEntity* pEntity);

	// Use this to dereference the handle.
	// Note: this is implemented in game code (ehandle.h)
	IHandleEntity* Get() const;

protected:
	union
	{
		uint32 m_Index;
		struct
		{
			uint32 m_EntityIndex : 15;
			uint32 m_Serial : 17;
		} m_Parts;
	};
};


inline CEntityHandle::CEntityHandle()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline CEntityHandle::CEntityHandle(const CEntityHandle& other)
{
	m_Index = other.m_Index;
}

inline CEntityHandle::CEntityHandle(uint32 value)
{
	m_Index = value;
}

inline CEntityHandle::CEntityHandle(int iEntry, int iSerialNumber)
{
	Init(iEntry, iSerialNumber);
}

inline void CEntityHandle::Init(int iEntry, int iSerialNumber)
{
	m_Parts.m_EntityIndex = iEntry;
	m_Parts.m_Serial = iSerialNumber;
}

inline void CEntityHandle::Term()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline bool CEntityHandle::IsValid() const
{
	return m_Index != INVALID_EHANDLE_INDEX;
}

inline int CEntityHandle::GetEntryIndex() const
{
	if (IsValid())
	{
		return m_Parts.m_EntityIndex;
	}

	return -1;
}

inline int CEntityHandle::GetSerialNumber() const
{
	return m_Parts.m_Serial;
}

inline bool CEntityHandle::operator !=(const CEntityHandle& other) const
{
	return m_Index != other.m_Index;
}

inline bool CEntityHandle::operator ==(const CEntityHandle& other) const
{
	return m_Index == other.m_Index;
}

inline bool CEntityHandle::operator ==(const IHandleEntity* pEnt) const
{
	return Get() == pEnt;
}

inline bool CEntityHandle::operator !=(const IHandleEntity* pEnt) const
{
	return Get() != pEnt;
}

inline bool CEntityHandle::operator <(const CEntityHandle& other) const
{
	return m_Index < other.m_Index;
}

inline bool CEntityHandle::operator <(const IHandleEntity* pEntity) const
{
	unsigned long otherIndex = (pEntity) ? pEntity->GetRefEHandle().m_Index : INVALID_EHANDLE_INDEX;
	return m_Index < otherIndex;
}

inline const CEntityHandle& CEntityHandle::operator=(const IHandleEntity* pEntity)
{
	return Set(pEntity);
}

inline const CEntityHandle& CEntityHandle::Set(const IHandleEntity* pEntity)
{
	if (pEntity)
	{
		*this = pEntity->GetRefEHandle();
	}
	else
	{
		m_Index = INVALID_EHANDLE_INDEX;
	}

	return *this;
}

#endif // ENTITYHANDLE_H

typedef CEntityHandle CBaseHandle;