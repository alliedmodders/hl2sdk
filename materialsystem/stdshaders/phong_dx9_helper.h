//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHONG_DX9_HELPER_H
#define PHONG_DX9_HELPER_H

#include <string.h>

#include "vertexlitgeneric_dx9_helper.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;

void InitParamsPhong_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, VertexLitGeneric_DX9_Vars_t &info );
void InitPhong_DX9( CBaseVSShader *pShader, IMaterialVar** params, VertexLitGeneric_DX9_Vars_t &info );
void DrawPhong_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow,
				  VertexLitGeneric_DX9_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr );

#endif // PHONG_DX9_HELPER_H
