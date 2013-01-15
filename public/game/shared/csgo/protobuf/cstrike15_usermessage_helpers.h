#ifndef CSTRIKE15_UM_HELPERS
#define CSTRIKE15_UM_HELPERS

#include "game/shared/csgo/protobuf/cstrike15_usermessages.pb.h"
#include <tier1/utlhashdict.h>

class CCstrike15UsermessageHelpers
{
public:
	CCstrike15UsermessageHelpers();

	const google::protobuf::Message *GetPrototype( int index ) const;
	const google::protobuf::Message *GetPrototype( const char *name ) const;

	int GetIndex( const char *name ) const;

	const char *GetName( int index ) const;

private:
	CUtlHashDict<int, false, false> m_NameIndexMap;
	const char *m_IndexNameMap[ECstrike15UserMessages_ARRAYSIZE];
	const google::protobuf::Message *m_Prototypes[ECstrike15UserMessages_ARRAYSIZE];
};

extern CCstrike15UsermessageHelpers g_Cstrike15UsermessageHelpers;

#endif // CSTRIKE15_UM_HELPERS
