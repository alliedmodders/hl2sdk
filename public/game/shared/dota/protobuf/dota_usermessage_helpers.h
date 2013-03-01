#ifndef DOTA_UM_HELPERS
#define DOTA_UM_HELPERS

#include "game/shared/protobuf/usermessages.pb.h"
#include "game/shared/dota/protobuf/dota_usermessages.pb.h"
#include <tier1/utlhashdict.h>

class CDotaUsermessageHelpers
{
public:
	CDotaUsermessageHelpers();

	const google::protobuf::Message *GetPrototype( int index ) const;
	const google::protobuf::Message *GetPrototype( const char *name ) const;

	int GetIndex( const char *name ) const;

	const char *GetName( int index ) const;

private:
	CUtlHashDict<int, false, false> m_NameIndexMap;
	const char *m_IndexNameMap[EDotaUserMessages_ARRAYSIZE];
	const google::protobuf::Message *m_Prototypes[EDotaUserMessages_ARRAYSIZE];
};

extern CDotaUsermessageHelpers g_DotaUsermessageHelpers;

#endif // DOTA_UM_HELPERS
