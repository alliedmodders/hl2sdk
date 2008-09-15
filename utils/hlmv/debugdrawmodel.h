//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef DEBUGDRAWMODEL_H
#define DEBUGDRAWMODEL_H
#ifdef _WIN32
#pragma once
#endif

int DebugDrawModel( IStudioRender *pStudioRender, DrawModelInfo_t& info, const Vector &modelOrigin,
			        int *pLodUsed, float *pMetric, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL );
int DebugDrawModelNormals( IStudioRender *pStudioRender, DrawModelInfo_t& info, const Vector &modelOrigin,
			        int *pLodUsed, float *pMetric, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL );
int DebugDrawModelTangentS( IStudioRender *pStudioRender, DrawModelInfo_t& info, const Vector &modelOrigin,
			        int *pLodUsed, float *pMetric, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL );
int DebugDrawModelTangentT( IStudioRender *pStudioRender, DrawModelInfo_t& info, const Vector &modelOrigin,
			        int *pLodUsed, float *pMetric, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL );
int DebugDrawModelBoneWeights( IStudioRender *pStudioRender, DrawModelInfo_t& info, const Vector &modelOrigin,
			        int *pLodUsed, float *pMetric, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL  );

#endif // DEBUGDRAWMODEL_H
