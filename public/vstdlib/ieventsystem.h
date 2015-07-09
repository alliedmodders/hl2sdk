#include <appframework/IAppSystem.h>
#include <tier1/functors.h>

class IEventQueue;

abstract_class IEventSystem : public IAppSystem
{
public:
	virtual IEventQueue *CreateEventQueue() = 0;
	virtual void DestroyEventQueue( IEventQueue *pQueue ) = 0;
	virtual void RunEvents( IEventQueue *pQueue ) = 0;
	virtual int RegisterEvent( const char *pszName ) = 0;
	virtual void FireEvent( int eventId, IEventQueue *pQueue, const void *pListener, CFunctorData *pData ) = 0;
	virtual void RegisterListener( int eventId, IEventQueue *pQueue, CFunctorCallback *pCallback ) = 0;
	virtual void UnregisterListener( int eventId, IEventQueue *pQueue, CFunctorCallback *pCallback ) = 0;
};