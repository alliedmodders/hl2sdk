#ifndef TERROR_UM_HELPERS
#define TERROR_UM_HELPERS

#include "game/shared/terror/protobuf/terror_usermessages.pb.h"
#include <tier1/utlhashdict.h>

class CTerrorUsermessageHelpers
{
public:
	CTerrorUsermessageHelpers();

	const google::protobuf::Message *GetPrototype( int index ) const;
	const google::protobuf::Message *GetPrototype( const char *name ) const;

	int GetIndex( const char *name ) const;

	const char *GetName( int index ) const;

private:
	CUtlHashDict<int, false, false> m_NameIndexMap;
	const char *m_IndexNameMap[ETerrorUserMessages_ARRAYSIZE];
	const google::protobuf::Message *m_Prototypes[ETerrorUserMessages_ARRAYSIZE];
};

extern CTerrorUsermessageHelpers g_TerrorUsermessageHelpers;

#endif // TERROR_UM_HELPERS
