//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: A base class for all material proxies in the client dll
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
// identifier was truncated to '255' characters in the debug information
//#pragma warning(disable: 4786)

#include "ProxyEntity.h"
#include "materialsystem/IMaterialVar.h"

class CEntityOriginMaterialProxy : public CEntityMaterialProxy
{
public:
	CEntityOriginMaterialProxy()
	{
		m_pMaterial = NULL;
		m_pOriginVar = NULL;
	}
	virtual ~CEntityOriginMaterialProxy()
	{
	}
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		m_pMaterial = pMaterial;
		bool found;
		m_pOriginVar = m_pMaterial->FindVar( "$entityorigin", &found );
		if( !found )
		{
			m_pOriginVar = NULL;
			return false;
		}
		return true;
	}
	virtual void OnBind( C_BaseEntity *pC_BaseEntity )
	{
		const Vector &origin = pC_BaseEntity->GetAbsOrigin();
		m_pOriginVar->SetVecValue( origin.x, origin.y, origin.z );
	}

protected:
	IMaterial *m_pMaterial;
	IMaterialVar *m_pOriginVar;
};

EXPOSE_INTERFACE( CEntityOriginMaterialProxy, IMaterialProxy, "EntityOrigin" IMATERIAL_PROXY_INTERFACE_VERSION );
