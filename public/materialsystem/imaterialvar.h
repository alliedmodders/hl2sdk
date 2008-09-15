//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef IMATERIALVAR_H
#define IMATERIALVAR_H

#ifdef _WIN32
#pragma once
#endif

class ITexture;
class IMaterial;
class VMatrix;

#define MAKE_MATERIALVAR_FOURCC(ch0, ch1, ch2, ch3)                              \
		((unsigned long)(ch0) | ((unsigned long)(ch1) << 8) |   \
		((unsigned long)(ch2) << 16) | ((unsigned long)(ch3) << 24 ))

// This fourcc is reserved.
#define FOURCC_UNKNOWN	MAKE_MATERIALVAR_FOURCC('U','N','K','N')


//-----------------------------------------------------------------------------
// Various material var types
//-----------------------------------------------------------------------------
enum MaterialVarType_t 
{ 
	MATERIAL_VAR_TYPE_FLOAT = 0,
	MATERIAL_VAR_TYPE_STRING,
	MATERIAL_VAR_TYPE_VECTOR,
	MATERIAL_VAR_TYPE_TEXTURE,
	MATERIAL_VAR_TYPE_INT,
	MATERIAL_VAR_TYPE_FOURCC,
	MATERIAL_VAR_TYPE_UNDEFINED,
	MATERIAL_VAR_TYPE_MATRIX,
	MATERIAL_VAR_TYPE_MATERIAL,
};

typedef unsigned short MaterialVarSym_t;

class IMaterialVar
{
public:
	typedef unsigned long FourCC;
	
	// class factory methods
	static IMaterialVar* Create( IMaterial* pMaterial, char const* pKey, VMatrix const& matrix );
	static IMaterialVar* Create( IMaterial* pMaterial, char const* pKey, char const* pVal );
	static IMaterialVar* Create( IMaterial* pMaterial, char const* pKey, float* pVal, int numcomps );
	static IMaterialVar* Create( IMaterial* pMaterial, char const* pKey, float val );
	static IMaterialVar* Create( IMaterial* pMaterial, char const* pKey, int val );
	static IMaterialVar* Create( IMaterial* pMaterial, char const* pKey );
	static void Destroy( IMaterialVar* pVar );
	static MaterialVarSym_t	GetSymbol( char const* pName );
	static MaterialVarSym_t	FindSymbol( char const* pName );
	static bool SymbolMatches( char const* pName, MaterialVarSym_t symbol );
	static void DeleteUnreferencedTextures( bool enable );

	virtual char const *	GetName( void ) const = 0;
	virtual MaterialVarSym_t	GetNameAsSymbol() const = 0;

	virtual void			SetFloatValue( float val ) = 0;
	virtual float			GetFloatValue( void ) = 0;
	
	virtual void			SetIntValue( int val ) = 0;
	virtual int				GetIntValue( void ) const = 0;
	
	virtual void			SetStringValue( char const *val ) = 0;
	virtual char const *	GetStringValue( void ) const = 0;

	// Use FourCC values to pass app-defined data structures between
	// the proxy and the shader. The shader should ignore the data if
	// its FourCC type not correct.	
	virtual void			SetFourCCValue( FourCC type, void *pData ) = 0;
	virtual void			GetFourCCValue( FourCC *type, void **ppData ) = 0;

	// Vec (dim 2-4)
	virtual void			SetVecValue( float const* val, int numcomps ) = 0;
	virtual void			SetVecValue( float x, float y ) = 0;
	virtual void			SetVecValue( float x, float y, float z ) = 0;
	virtual void			SetVecValue( float x, float y, float z, float w ) = 0;
	virtual void			GetLinearVecValue( float *val, int numcomps ) const = 0;
	virtual void			GetVecValue( float *val, int numcomps ) const = 0;
	virtual float const*	GetVecValue( ) const = 0;
	virtual int				VectorSize() const = 0;

	// revisit: is this a good interface for textures?
	virtual ITexture *		GetTextureValue( void ) = 0;
	virtual void			SetTextureValue( ITexture * ) = 0;
	virtual					operator ITexture*() = 0;

	virtual IMaterial *		GetMaterialValue( void ) = 0;
	virtual void			SetMaterialValue( IMaterial * ) = 0;

	virtual MaterialVarType_t	GetType() const = 0;
	virtual bool			IsDefined() const = 0;
	virtual void			SetUndefined() = 0;
	inline bool IsTexture() const
	{
		return GetType() == MATERIAL_VAR_TYPE_TEXTURE;
	}

	// Matrix
	virtual void			SetMatrixValue( VMatrix const& matrix ) = 0;
	virtual const VMatrix  &GetMatrixValue( ) = 0;
	virtual bool			MatrixIsIdentity() const = 0;

	// Copy....
	virtual void			CopyFrom( IMaterialVar *pMaterialVar ) = 0;

	virtual void			SetValueAutodetectType( char const *val ) = 0;
};

#endif // IMATERIALVAR_H
