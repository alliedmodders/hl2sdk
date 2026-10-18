#ifndef BASE_UM_HELPERS
#define BASE_UM_HELPERS

#include "game/shared/protobuf/base_usermessages.pb.h"
#include <tier1/utlhashdict.h>

class CUsermessageHelpers
{
public:
	CUsermessageHelpers();

	const google::protobuf::Message *GetPrototype( int index ) const;
	const google::protobuf::Message *GetPrototype( const char *name ) const;

	int GetIndex( const char *name ) const;

	const char *GetName( int index ) const;

private:
	CUtlHashDict<int, false, false> m_NameIndexMap;
	const char *m_IndexNameMap[EUserMessages_ARRAYSIZE];
	const google::protobuf::Message *m_Prototypes[EUserMessages_ARRAYSIZE];
};

extern CUsermessageHelpers g_BaseUsermessageHelpers;

#endif // BASE_UM_HELPERS
