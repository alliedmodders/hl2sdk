#include "entitysystem.h"

// In IDA a __int64 can even be a class
typedef long __int64;


// still need to figure out what this returns
__int64 CEntityClass::Unserialize(CEntityClass *const this, CEntityInstance *pEntity, const CEntityKeyValues *pKeyValues) {

    EntityIOConnectionDesc_t *m_Size;
    void **p_outputs;

    UnserializeComponentInfo_t *v8; // keep an eye on this thing
    EntComponentInfo_t *m_pInfo;

    const char *m_pAsString; // v7, e.g. weapon_glock
    const char *m_pszCPPClassname; // v9, e.g. CWeaponGlock
  

    __int64 entity_index; // v8, e.g. ent_attachments entity_index
}
