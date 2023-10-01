#include "entitysystem.h"

// In IDA a __int64 can even be a class
typedef long __int64;


__int64 CEntityClass::UnserializeKey(
    const CEntityClass::UnserializationContext_t* const context,
    CEntityInstance* pEntity,
    char* pFieldMemory,
    const ComponentUnserializerFieldInfo_t* pFieldInfo,
    CEntityVariant* value,
    /*__m128d a7*/)
{


}





// still need to figure out what this returns
__int64 CEntityClass::Unserialize(CEntityInstance* pEntity, const CEntityKeyValues* pKeyValues) {

    EntityIOConnectionDesc_t* m_Size;
    void** p_outputs;

    UnserializeComponentInfo_t* v8; // keep an eye on this thing
    EntComponentInfo_t* m_pInfo;

    // Some stuff is only used for logging
    const char* m_pAsString; // v7, e.g. weapon_glock
    const char* m_pszCPPClassname; // v9, e.g. CWeaponGlock

    __int64 entity_index; // v8, e.g. ent_attachments entity_index



    CEntityClass* c;
    CEntityClass::UnserializationContext_t context;

    CUtlVector<CEntityIOOutput*, CUtlMemory<CEntityIOOutput*, int> > outputs;
    EntityIOConnectionDesc_t desc;
    CUtlVectorFixedGrowable<UnserializeComponentInfo_t, 64ul> components;


    c = this;





}
