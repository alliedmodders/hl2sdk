//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======
//
// Purpose:
//
//=============================================================================

#ifndef VALVEMAYA_H
#define VALVEMAYA_H

#if defined( _WIN32 )
#pragma once
#endif

// std includes
#include <iostream>
#include <streambuf>
#include <ostream>

// Maya Includes

#include <maya/MDagModifier.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MSyntax.h>
#include <maya/MString.h>
#include <maya/MDagPath.h>

// Valve Includes

#include "tier1/stringpool.h"
#include "tier1/utlstring.h"
#include "tier1/utlstringmap.h"
#include "tier1/utlvector.h"
#include "tier1/interface.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMayaVGui;


//-----------------------------------------------------------------------------
// Maya-specific library singletons
//-----------------------------------------------------------------------------
extern IMayaVGui *g_pMayaVGui;


//-----------------------------------------------------------------------------
//
// Purpose: Group a bunch of functions into the Valve Maya Namespace
//
//-----------------------------------------------------------------------------


namespace ValveMaya
{

//-----------------------------------------------------------------------------
// Connect, disconnect
//-----------------------------------------------------------------------------
bool ConnectLibraries( CreateInterfaceFn factory );
void DisconnectLibraries();

MStatus CreateDagNode(
	const char *const i_nodeType,
	const char *const i_transformName = NULL,
	const MObject &i_parentObj = MObject::kNullObj,
	MObject *o_pTransformObj = NULL,
	MObject *o_pShapeObj = NULL,
	MDagModifier *i_mDagModifier = NULL);

bool IsNodeVisible(	const MDagPath &mDagPath, bool bTemplateAsInvisible = true );
bool IsPathVisible( MDagPath mDagPath, bool bTemplateAsInvisible = true );

class CMSyntaxHelp 
{
public:
	CMSyntaxHelp()
	: m_groupedHelp( false )	// Make case sensitive
	, m_helpCount( 0 )
	, m_shortNameLength( 0 )
	{}

	void Clear();

	MStatus AddFlag(
		MSyntax &i_mSyntax,
		const char *i_shortName,
		const char *i_longName,
		const char *i_group,
		const char *i_help,
		const MSyntax::MArgType i_argType1 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType2 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType3 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType4 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType5 = MSyntax::kNoArg,
		const MSyntax::MArgType i_argType6 = MSyntax::kNoArg);

	void PrintHelp(
		const char *const i_cmdName,
		const char *const i_cmdDesc,
		int i_lineLength = 0U);

	void PrintHelp(
		const MString &i_cmdName,
		const MString &i_cmdDesc,
		int i_lineLength = 0U)
	{
		PrintHelp( i_cmdName.asChar(), i_cmdDesc.asChar(), i_lineLength );
	}

protected:
public:
protected:
	CCountedStringPool m_stringPool;

	struct HelpData_t
	{
		const char *m_shortName;
		const char *m_longName;
		const char *m_help;
		CUtlVector<MSyntax::MArgType> m_argTypes;
	};

	CUtlVector<const char *> m_groupOrder;
	CUtlStringMap<CUtlVector<HelpData_t> > m_groupedHelp;
	int m_helpCount;
	int m_shortNameLength;
};

MString RemoveNameSpace( const MString &mString );

MString &BackslashToSlash( MString &mString );

template < class T_t > bool IsDefault( T_t &, const MPlug & );

bool IsDefault(	const MPlug &aPlug );

uint NextAvailable( MPlug &mPlug );

MObject AddColorSetToMesh(
	const MString &colorSetName,
	const MDagPath &mDagPath,
	MDagModifier &mDagModifier );

MString GetMaterialPath(
	const MObject &shadingGroupObj,
	MObject *pFileObj,
	MObject *pPlace2dTextureObj,
	MObject *pVmtObj,
	bool *pbTransparent,
	MString *pDebugWhy );

// Returns the node MObject that is connected as an input to the specified attribute on the specified node
MObject FindInputNode( const MObject &dstNodeObj, const MString &dstPlugName );

// Returns the first found node MObject of the specified type in the history of the specified node
MObject FindInputNodeOfType( const MObject &dstNodeObj, const MString &typeName, const MString &dstPlugName );

MObject FindInputNodeOfType( const MObject &dstNodeObj, const MString &typeName );

}  // end namespace ValveMaya


// Make an alias for the ValveMaya namespace

namespace vm = ValveMaya;


//-----------------------------------------------------------------------------
// Purpose: A iostream streambuf which puts stuff in the Maya GUI
//-----------------------------------------------------------------------------
class CMayaStreamBuf : public std::streambuf
{
public:
	enum StreamType { kInfo, kWarning, kError };

	CMayaStreamBuf( const StreamType i_streamType = kInfo )
	: m_streamType( i_streamType )
	{}

	virtual int sync()
	{
		const int n = pptr() - pbase();

		if ( pbase() && n )
		{
			m_string = MString( pbase(), n );
			outputString();
		}

		return n;
	}


	int overflow( int ch )
	{
		const int n = pptr() - pbase();

		if ( n && sync() )
			return EOF;

		if ( ch != EOF )
		{
			if ( ch == '\n' )
				outputString();
			else {
				const char cCh( ch );
				const MString mString( &cCh, 1 );

				m_string += mString;
			}
		}

		pbump( -n );

		return 0;
	}

	virtual int underflow() { return EOF; }

protected:
	virtual void outputString()
	{
		switch ( m_streamType )
		{
		case kWarning:
			MGlobal::displayWarning( m_string );
			break;
		case kError:
			MGlobal::displayError( m_string );
			break;
		default:
			MGlobal::displayInfo( m_string );
			break;
		}

		m_string.clear();
	}

	StreamType m_streamType;
	MString m_string;
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CMayaStream : public std::ostream
{
public:

	CMayaStream()
	: std::ostream( new CMayaStreamBuf )
	{
	}

	~CMayaStream() 
	{
		delete rdbuf(); 
	}
};


//-----------------------------------------------------------------------------
//
// minfo, mwarn & merr are ostreams which can be used to send stuff to
// the Maya history window
//
//-----------------------------------------------------------------------------

extern CMayaStream minfo;
extern CMayaStream mwarn;
extern CMayaStream merr;


#endif // VALVEMAYA_H