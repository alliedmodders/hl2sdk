//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//


#include "dt_recv.h"
#include "vector.h"
#include "vstdlib/strtools.h"
#include "dt_utlvector_common.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)

char *s_ClientElementNames[MAX_ARRAY_ELEMENTS] =
{
	"000", "001", "002", "003", "004", "005", "006", "007", "008", "009", 
	"010", "011", "012", "013", "014", "015", "016", "017", "018", "019",
	"020", "021", "022", "023", "024", "025", "026", "027", "028", "029",
	"030", "031", "032", "033", "034", "035", "036", "037", "038", "039",
	"040", "041", "042", "043", "044", "045", "046", "047", "048", "049",
	"050", "051", "052", "053", "054", "055", "056", "057", "058", "059",
	"060", "061", "062", "063", "064", "065", "066", "067", "068", "069",
	"070", "071", "072", "073", "074", "075", "076", "077", "078", "079",
	"080", "081", "082", "083", "084", "085", "086", "087", "088", "089",
	"090", "091", "092", "093", "094", "095", "096", "097", "098", "099",
	"100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
	"110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
	"120", "121", "122", "123", "124", "125", "126", "127", "128", "129",
	"130", "131", "132", "133", "134", "135", "136", "137", "138", "139",
	"140", "141", "142", "143", "144", "145", "146", "147", "148", "149",
	"150", "151", "152", "153", "154", "155", "156", "157", "158", "159",
	"160", "161", "162", "163", "164", "165", "166", "167", "168", "169",
	"170", "171", "172", "173", "174", "175", "176", "177", "178", "179",
	"180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
	"190", "191", "192", "193", "194", "195", "196", "197", "198", "199",
	"200", "201", "202", "203", "204", "205", "206", "207", "208", "209",
	"210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
	"220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
	"230", "231", "232", "233", "234", "235", "236", "237", "238", "239",
	"240", "241", "242", "243", "244", "245", "246", "247", "248", "249",
	"250", "251", "252", "253", "254", "255"
};

CStandardRecvProxies::CStandardRecvProxies()
{
	m_Int32ToInt8 = RecvProxy_Int32ToInt8;
	m_Int32ToInt16 = RecvProxy_Int32ToInt16;
	m_Int32ToInt32 = RecvProxy_Int32ToInt32;
	m_FloatToFloat = RecvProxy_FloatToFloat;
	m_VectorToVector = RecvProxy_VectorToVector;
}

CStandardRecvProxies g_StandardRecvProxies;
	

// ---------------------------------------------------------------------- //
// RecvProp.
// ---------------------------------------------------------------------- //
RecvProp::RecvProp()
{
	m_pExtraData = NULL;
	m_pVarName = NULL;
	m_Offset = 0;
	m_RecvType = DPT_Int;
	m_Flags = 0;
	m_ProxyFn = NULL;
	m_DataTableProxyFn = NULL;
	m_pDataTable = NULL;
	m_nElements = 1;
	m_ElementStride = -1;
	m_pArrayProp = NULL;
	m_ArrayLengthProxy = NULL;
	m_bInsideArray = false;
}

// ---------------------------------------------------------------------- //
// RecvTable.
// ---------------------------------------------------------------------- //
RecvTable::RecvTable()
{
	Construct( NULL, 0, NULL );
}

RecvTable::RecvTable(RecvProp *pProps, int nProps, char *pNetTableName)
{
	Construct( pProps, nProps, pNetTableName );
}

RecvTable::~RecvTable()
{
}

void RecvTable::Construct( RecvProp *pProps, int nProps, char *pNetTableName )
{
	m_pProps = pProps;
	m_nProps = nProps;
	m_pDecoder = NULL;
	m_pNetTableName = pNetTableName;
	m_bInitialized = false;
	m_bInMainList = false;
}


// ---------------------------------------------------------------------- //
// Prop setup functions (for building tables).
// ---------------------------------------------------------------------- //

RecvProp RecvPropFloat(
	char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	// Debug type checks.
	if ( varProxy == RecvProxy_FloatToFloat )
	{
		Assert( sizeofVar == 0 || sizeofVar == 4 );
	}

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Float;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropVector(
	char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	// Debug type checks.
	#ifdef _DEBUG
		if(varProxy == RecvProxy_VectorToVector)
		{
			Assert(sizeofVar == sizeof(Vector));
		}
	#endif

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Vector;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!

RecvProp RecvPropQuaternion(
	char *pVarName, 
	int offset, 
	int sizeofVar,	// Handled by RECVINFO macro, but set to SIZEOF_IGNORE if you don't want to bother.
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	// Debug type checks.
	#ifdef _DEBUG
		if(varProxy == RecvProxy_QuaternionToQuaternion)
		{
			Assert(sizeofVar == sizeof(Quaternion));
		}
	#endif

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Quaternion;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}
#endif

RecvProp RecvPropInt(
	char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	// If they didn't specify a proxy, then figure out what type we're writing to.
	if(varProxy == NULL)
	{
		if(sizeofVar == 1)
		{
			varProxy = RecvProxy_Int32ToInt8;
		}
		else if(sizeofVar == 2)
		{
			varProxy = RecvProxy_Int32ToInt16;
		}
		else if(sizeofVar == 4)
		{
			varProxy = RecvProxy_Int32ToInt32;
		}
		else
		{
			Assert(!"RecvPropInt var has invalid size");
			varProxy = RecvProxy_Int32ToInt8;	// safest one...
		}
	}

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Int;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropString(
	char *pVarName,
	int offset,
	int bufferSize,
	int flags,
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_String;
	ret.m_Flags = flags;
	ret.m_StringBufferSize = bufferSize;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropDataTable(
	char *pVarName,
	int offset,
	int flags,
	RecvTable *pTable,
	DataTableRecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_DataTable;
	ret.m_Flags = flags;
	ret.SetDataTableProxyFn( varProxy );
	ret.SetDataTable( pTable );

	return ret;
}

RecvProp RecvPropArray3(
	char *pVarName,
	int offset,
	int sizeofVar,
	int elements,
	RecvProp pArrayProp,
	DataTableRecvVarProxyFn varProxy						
	)
{
	RecvProp ret;

	Assert( elements <= MAX_ARRAY_ELEMENTS );

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_DataTable;
	ret.SetDataTableProxyFn( varProxy );

	RecvProp *pProps = new RecvProp[elements]; // TODO free that again

	const char *pParentArrayPropName = AllocateStringHelper( "%s", pVarName );

	for ( int i=0; i < elements; i++ )
	{
		pProps[i] = pArrayProp; // copy basic property settings 
		pProps[i].SetOffset( i * sizeofVar ); // adjust offset
		pProps[i].m_pVarName = s_ClientElementNames[i]; // give unique name
		pProps[i].SetParentArrayPropName( pParentArrayPropName ); // For debugging...
	}

	RecvTable *pTable = new RecvTable( pProps, elements, pVarName ); // TODO free that again

	ret.SetDataTable( pTable );

	return ret;
}

RecvProp InternalRecvPropArray(
	const int elementCount,
	const int elementStride,
	char *pName,
	ArrayLengthRecvProxyFn proxy
	)
{
	RecvProp ret;

	ret.InitArray( elementCount, elementStride );
	ret.m_pVarName = pName;
	ret.SetArrayLengthProxy( proxy );

	return ret;
}


// ---------------------------------------------------------------------- //
// Proxies.
// ---------------------------------------------------------------------- //

void RecvProxy_FloatToFloat( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	Assert( IsFinite( pData->m_Value.m_Float ) );
	*((float*)pOut) = pData->m_Value.m_Float;
}

void RecvProxy_VectorToVector( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	const float *v = pData->m_Value.m_Vector;
	
	Assert( IsFinite( v[0] ) && IsFinite( v[1] ) && IsFinite( v[2] ) );
	((float*)pOut)[0] = v[0];
	((float*)pOut)[1] = v[1];
	((float*)pOut)[2] = v[2];
}

void RecvProxy_QuaternionToQuaternion( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	const float *v = pData->m_Value.m_Vector;
	
	Assert( IsFinite( v[0] ) && IsFinite( v[1] ) && IsFinite( v[2] ) && IsFinite( v[3] ) );
	((float*)pOut)[0] = v[0];
	((float*)pOut)[1] = v[1];
	((float*)pOut)[2] = v[2];
	((float*)pOut)[3] = v[3];
}

void RecvProxy_Int32ToInt8( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((unsigned char*)pOut) = *((unsigned char*)&pData->m_Value.m_Int);
}

void RecvProxy_Int32ToInt16( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((unsigned short*)pOut) = *((unsigned short*)&pData->m_Value.m_Int);
}

void RecvProxy_Int32ToInt32( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((unsigned long*)pOut) = *((unsigned long*)&pData->m_Value.m_Int);
}

void RecvProxy_StringToString( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	char *pStrOut = (char*)pOut;
	if ( pData->m_pRecvProp->m_StringBufferSize <= 0 )
	{
		return;
	}

	for ( int i=0; i < pData->m_pRecvProp->m_StringBufferSize; i++ )
	{
		pStrOut[i] = pData->m_Value.m_pString[i];
		if(pStrOut[i] == 0)
			break;
	}
	
	pStrOut[pData->m_pRecvProp->m_StringBufferSize-1] = 0;
}

void DataTableRecvProxy_StaticDataTable( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	*pOut = pData;
}

void DataTableRecvProxy_PointerDataTable( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	*pOut = *((void**)pData);
}

#endif
