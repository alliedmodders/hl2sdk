#ifndef VIETNAM_UM_HELPERS
#define VIETNAM_UM_HELPERS

#include "game/shared/vietnam/protobuf/vietnam_usermessages.pb.h"
#include <tier1/utlhashdict.h>

class CVietnamUsermessageHelpers
{
public:
	CVietnamUsermessageHelpers();

	const google::protobuf::Message *GetPrototype( int index ) const;
	const google::protobuf::Message *GetPrototype( const char *name ) const;

	int GetIndex( const char *name ) const;

	const char *GetName( int index ) const;

private:
	CUtlHashDict<int, false, false> m_NameIndexMap;
	const char *m_IndexNameMap[ECstrike15UserMessages_ARRAYSIZE];
	const google::protobuf::Message *m_Prototypes[ECstrike15UserMessages_ARRAYSIZE];
};

extern CVietnamUsermessageHelpers g_VietnamUsermessageHelpers;

#endif // VIETNAM_UM_HELPERS
