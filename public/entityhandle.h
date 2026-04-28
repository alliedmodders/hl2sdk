//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
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

class CEntityInstance;

class CEntityHandle
{
public:
	friend class CEntityIdentity;

	CEntityHandle();
	CEntityHandle(const CEntityHandle& other);
	CEntityHandle(uint32 value);
	CEntityHandle(int iEntry, int iSerialNumber);

	void Init(int iEntry, int iSerialNumber);
	void Term();

	bool IsValid() const;

	int GetEntryIndex() const;
	int GetSerialNumber() const;

	int ToInt() const;

	// AMNOTE: Packs handle to an int used in net messages for example, replicates Server_EHandleToInt logic
	int ToPackedInt() const;
	// AMNOTE: Unpacks previously packed handle, mostly used in net messages.
	// Note: this is implemented in game code (ehandle.h)
	static CEntityHandle FromPackedInt( int packed_handle );

	bool operator !=(const CEntityHandle& other) const;
	bool operator ==(const CEntityHandle& other) const;
	bool operator ==(const CEntityInstance* pEnt) const;
	bool operator !=(const CEntityInstance* pEnt) const;
	bool operator <(const CEntityHandle& other) const;
	bool operator <(const CEntityInstance* pEnt) const;

	// Assign a value to the handle.
	const CEntityHandle& operator=(const CEntityInstance* pEntity);
	const CEntityHandle& Set(const CEntityInstance* pEntity);

	// Use this to dereference the handle.
	// Note: this is implemented in game code (ehandle.h)
	CEntityInstance* Get() const;

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

inline int CEntityHandle::ToInt() const
{
	return m_Index;
}

inline int CEntityHandle::ToPackedInt() const
{
	if(!IsValid())
		return 0xFFFFFF;

	return GetEntryIndex() | ((GetSerialNumber() & 0x3FF) << 14);
}

inline bool CEntityHandle::operator !=(const CEntityHandle& other) const
{
	return m_Index != other.m_Index;
}

inline bool CEntityHandle::operator ==(const CEntityHandle& other) const
{
	return m_Index == other.m_Index;
}

inline bool CEntityHandle::operator ==(const CEntityInstance* pEnt) const
{
	return Get() == pEnt;
}

inline bool CEntityHandle::operator !=(const CEntityInstance* pEnt) const
{
	return Get() != pEnt;
}

inline bool CEntityHandle::operator <(const CEntityHandle& other) const
{
	return m_Index < other.m_Index;
}

inline const CEntityHandle &CEntityHandle::operator=( const CEntityInstance *pEntity )
{
	return Set( pEntity );
}

typedef CEntityHandle CBaseHandle;

#endif // ENTITYHANDLE_H
