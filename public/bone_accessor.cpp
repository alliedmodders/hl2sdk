//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "bone_accessor.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


#if defined( CLIENT_DLL ) && defined( _DEBUG )

	void CBoneAccessor::SanityCheckBone( int iBone, bool bReadable ) const
	{
		if ( m_pAnimating )
		{
			CStudioHdr *pHdr = m_pAnimating->GetModelPtr();
			if ( pHdr )
			{
				mstudiobone_t *pBone = pHdr->pBone( iBone );
				if ( bReadable )
				{
					AssertOnce( pBone->flags & m_ReadableBones );
				}
				else
				{
					AssertOnce( pBone->flags & m_WritableBones );
				}
			}
		}
	}

#endif

