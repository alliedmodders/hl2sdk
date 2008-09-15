//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SHADER_VCS_VERSION_H
#define SHADER_VCS_VERSION_H
#ifdef _WIN32
#pragma once
#endif

// 1=hl2 shipped. 2=compressed with diffs version (lostcoast)
#define SHADER_VCS_VERSION_NUMBER 2

#pragma pack(1)
struct ShaderHeader_t_V1
{
	int				m_nVersion;
	int				m_nTotalCombos;
	int				m_nDynamicCombos;
	unsigned int	m_nFlags;
	unsigned int	m_nCentroidMask;
};

struct ShaderHeader_t : ShaderHeader_t_V1
{
	int m_ReferenceComboSizeForDiffs;
};

struct ShaderDictionaryEntry_t
{
	int m_Offset;
	int m_Size;
};
#pragma pack()

// 2=xbox shipped.
#define SHADER_XCS_VERSION_NUMBER	2

// xcs file format
#pragma pack(1)
struct XShaderDictionaryEntry_t
{
	int				m_Offset;
	unsigned short	m_PackedSize;
	unsigned short	m_Size;
};
struct XShaderHeader_t
{
	int				m_nVersion;
	int				m_nTotalCombos;
	int				m_nDynamicCombos;
	unsigned int	m_nFlags;
	unsigned int	m_nCentroidMask; 
	int				m_nReferenceShader;	

	bool			IsValid() { return m_nVersion == SHADER_XCS_VERSION_NUMBER; }
	unsigned int	BytesToPreload() { return sizeof( XShaderHeader_t ) + sizeof( XShaderDictionaryEntry_t ) * m_nTotalCombos; }  
};
#pragma pack()

#endif // SHADER_VCS_VERSION_H
	