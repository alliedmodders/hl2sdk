//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: interface to friends data in Steam
//
//=============================================================================

#ifndef ISTEAMFRIENDS_H
#define ISTEAMFRIENDS_H
#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Purpose: set of relationships to other users
//-----------------------------------------------------------------------------
enum EFriendRelationship
{
	k_EFriendRelationshipNone = 0,
	k_EFriendRelationshipBlocked = 1,
	k_EFriendRelationshipRequestingFriendship = 2,
	k_EFriendRelationshipFriend = 3,
};


//-----------------------------------------------------------------------------
// Purpose: list of states a friend can be in
//-----------------------------------------------------------------------------
enum EPersonaState
{
	k_EPersonaStateOffline = 0,			// friend is not currently logged on
	k_EPersonaStateOnline = 1,			// friend is logged on
	k_EPersonaStateBusy = 2,			// user is on, but busy
	k_EPersonaStateAway = 3,			// auto-away feature
	k_EPersonaStateSnooze = 4,			// auto-away for a long time
	k_EPersonaStateMax,
};


//-----------------------------------------------------------------------------
// Purpose: friend-to-friend message types
//-----------------------------------------------------------------------------
enum EFriendMsgType
{
	k_EFriendMsgTypeChat = 1,			// chat test message
	k_EFriendMsgTypeTyping = 2,			// lets the friend know the other user has starting typing a chat message
	k_EFriendMsgTypeInvite = 3,			// invites the friend into the users current game
	k_EFriendMsgTypeChatSent = 4,		// chat that the user has sent to a friend
};


enum { k_cchPersonaNameMax = 128 };

//-----------------------------------------------------------------------------
// Purpose: interface to friends
//-----------------------------------------------------------------------------
class ISteamFriends
{
public:
	// returns the local players name - guaranteed to not be NULL.
	virtual const char *GetPersonaName() = 0;
	// sets the player name, stores it on the server and publishes the changes to all friends who are online
	virtual void SetPersonaName( const char *pchPersonaName ) = 0;
	// gets the friend status of the current user
	virtual EPersonaState GetPersonaState() = 0;
	// sets the status, communicates to server, tells all friends
	virtual void SetPersonaState( EPersonaState ePersonaState ) = 0;

	// adds a friend to the users list.  Friend will be notified that they have been added, and have the option of accept/decline
	virtual bool AddFriend( CSteamID steamIDFriend ) = 0;
	// removes the friend from the list, and blocks them from contacting the user again
	virtual bool RemoveFriend( CSteamID steamIDFriend ) = 0;
	// returns true if the specified user is considered a friend (can see our online status)
	virtual bool HasFriend( CSteamID steamIDFriend ) = 0;
	// gets the relationship to a user
	virtual EFriendRelationship GetFriendRelationship( CSteamID steamIDFriend ) = 0;
	// returns true if the specified user is considered a friend (can see our online status)
	virtual EPersonaState GetFriendPersonaState( CSteamID steamIDFriend ) = 0;
    // retrieves details about the game the friend is currently playing - returns false if the friend is not playing any games
	virtual bool GetFriendGamePlayed( CSteamID steamIDFriend, int32 *pnGameID, uint32 *punGameIP, uint16 *pusGamePort ) = 0;
	// returns the name of a friend - guaranteed to not be NULL.
	virtual const char *GetFriendPersonaName( CSteamID steamIDFriend ) = 0;
	// adds a friend by email address or account name - value returned in callback
	virtual HSteamCall AddFriendByName( const char *pchEmailOrAccountName ) = 0;

	// friend iteration
	virtual int GetFriendCount() = 0;
	virtual CSteamID GetFriendByIndex(int iFriend) = 0;

	// generic friend->friend message sending
	// DEPRECATED, use the sized-buffer version instead (has much higher max buffer size)
	virtual void SendMsgToFriend( CSteamID steamIDFriend, EFriendMsgType eFriendMsgType, const char *pchMsgBody ) = 0;

	// steam registry, accessed by friend
	virtual void SetFriendRegValue( CSteamID steamIDFriend, const char *pchKey, const char *pchValue ) = 0;
	virtual const char *GetFriendRegValue( CSteamID steamIDFriend, const char *pchKey ) = 0;

	// accesses old friends names - returns an empty string when their are no more items in the history
	virtual const char *GetFriendPersonaNameHistory( CSteamID steamIDFriend, int iPersonaName ) = 0;

	// chat message iteration
	// returns the number of bytes in the message, filling pvData with as many of those bytes as possible
	// returns 0 if the steamID or iChatID are invalid
	virtual int GetChatMessage( CSteamID steamIDFriend, int iChatID, void *pvData, int cubData, EFriendMsgType *peFriendMsgType ) = 0;

	// generic friend->friend message sending, takes a sized buffer
	virtual bool SendMsgToFriend( CSteamID steamIDFriend, EFriendMsgType eFriendMsgType, const void *pvMsgBody, int cubMsgBody ) = 0;

	// returns the chatID that a chat should be resumed from when switching chat contexts
	virtual int GetChatIDOfChatHistoryStart( CSteamID steamIDFriend ) = 0;
	// sets where a chat with a user should resume
	virtual void SetChatHistoryStart( CSteamID steamIDFriend, int iChatID ) = 0;
	// clears the chat history - should be called when a chat dialog closes
	// the chat history can still be recovered by another context using SetChatHistoryStart() to reset the ChatIDOfChatHistoryStart
	virtual void ClearChatHistory( CSteamID steamIDFriend ) = 0;
};


#define STEAMFRIENDS_INTERFACE_VERSION "SteamFriends001"


enum {	k_iSteamFriendsCallbacks = 300 };

//-----------------------------------------------------------------------------
// Purpose: called after a friend has been successfully added
//-----------------------------------------------------------------------------
struct FriendAdded_t
{
	enum { k_iCallback = k_iSteamFriendsCallbacks + 1 };
	uint8 m_bSuccess;
	uint64 m_ulSteamID;	// steamID of the friend who was just added
};


//-----------------------------------------------------------------------------
// Purpose: called when a user is requesting friendship
//			the persona details of this user are guaranteed to be available locally
//			at the point this callback occurs
//-----------------------------------------------------------------------------
struct UserRequestingFriendship_t
{
	enum { k_iCallback = k_iSteamFriendsCallbacks + 2 };
	uint64 m_ulSteamID;		// steamID of the friend who just added us
};


//-----------------------------------------------------------------------------
// Purpose: called when a friends' status changes
//-----------------------------------------------------------------------------
struct PersonaStateChange_t
{
	enum { k_iCallback = k_iSteamFriendsCallbacks + 3 };
	uint64 m_ulSteamID;					// steamID of the friend who changed

	// previous state of the user, so comparisons can be done of exactly what changed
	int32 m_ePersonaStatePrevious;
	int32 m_nGameIDPrevious;
	uint32 m_unGameServerIPPrevious;
	uint16 m_usGameServerPortPrevious;
};


const int k_cchSystemIMTextMax = 4096;					// upper bound of length of system IM text

//-----------------------------------------------------------------------------
// Purpose: used to send a system IM from the service to a user
//-----------------------------------------------------------------------------
struct SystemIM_t
{
	enum { k_iCallback = k_iSteamFriendsCallbacks + 5 };
	uint32 m_ESystemIMType;					// type of system IM
	char m_rgchMsgBody[k_cchSystemIMTextMax];		// text associated with message (if any)
};


// 32KB max size on chat messages
enum { k_cchFriendChatMsgMax = 32 * 1024 };

//-----------------------------------------------------------------------------
// Purpose: called when this client has received a chat/invite/etc. message from a friend
//-----------------------------------------------------------------------------
struct FriendChatMsg_t
{
	enum { k_iCallback = k_iSteamFriendsCallbacks + 6 };

	uint64 m_ulSteamID;				// steamID of the friend who has sent this message
	uint8 m_eFriendMsgType;			// type of message
	uint32 m_iChatID;				// index of the chat message to lookup
};



#endif // ISTEAMFRIENDS_H
