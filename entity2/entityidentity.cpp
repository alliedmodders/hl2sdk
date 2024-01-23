#include "entityidentity.h"
#include "entitysystem.h"
#include "tier1/strtools.h"

bool CEntityIdentity::NameMatches( const char* pszNameOrWildcard ) const
{
	if ( pszNameOrWildcard && pszNameOrWildcard[0] == '!' )
		return GameEntitySystem()->FindEntityProcedural( pszNameOrWildcard ) == m_pInstance;

	return V_CompareNameWithWildcards( pszNameOrWildcard, m_name.String() ) == 0;
}

bool CEntityIdentity::ClassMatches( const char* pszClassOrWildcard ) const
{
	return V_CompareNameWithWildcards( pszClassOrWildcard, m_designerName.String() ) == 0;
}
