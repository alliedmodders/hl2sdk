#include "entityidentity.h"
#include "entitysystem.h"
#include "tier1/strtools.h"

bool CEntityIdentity::NameMatches( const char* szName ) const
{
	if ( szName && szName[0] == '!' )
		return GameEntitySystem()->FindEntityProcedural( szName ) == m_pInstance;

	return V_CompareNameWithWildcards( szName, m_name.String() ) == 0;
}

bool CEntityIdentity::ClassMatches( const char* szClassName ) const
{
	return V_CompareNameWithWildcards( szClassName, m_designerName.String() ) == 0;
}
