#ifndef DOTA_CM_HELPERS
#define DOTA_CM_HELPERS

#include <game/shared/dota/protobuf/dota_clientmessages.pb.h>
#include <tier1/utlhashdict.h>

class CDotaClientMessageHelpers
{
public:
	CDotaClientMessageHelpers();

	const google::protobuf::Message *GetPrototype( int index ) const;
	const google::protobuf::Message *GetPrototype( const char *name ) const;

	int GetIndex( const char *name ) const;

	const char *GetName( int index ) const;

private:
	CUtlHashDict<int, false, false> m_NameIndexMap;
	const char *m_IndexNameMap[EDotaClientMessages_ARRAYSIZE];
	const google::protobuf::Message *m_Prototypes[EDotaClientMessages_ARRAYSIZE];
};

extern CDotaClientMessageHelpers g_DotaClientMessageHelpers;

#endif // DOTA_CM_HELPERS
