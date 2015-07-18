/*
 * NetworkConstants.h
 *
 *  Created on: Jun 29, 2015
 *      Author: practicing01
 */
#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Object.h>

using namespace Urho3D;

EVENT(E_CLIENTSYNC, ClientSync)
{
	PARAM(P_CONNECTION, Connection);// Connection pointer
	PARAM(P_CLIENTID, ClientID);// int
}

extern const int MSG_PUSHSERVER;
extern const int MSG_POPSERVER;
extern const int MSG_GETSERVERS;
extern const int MSG_SERVERINFO;
extern const int MSG_SERVERSSENT;
extern const int MSG_MYCLIENTID;
extern const int MSG_CLIENTID;
extern const int MSG_CLIENTDISCO;
extern const int MSG_NETPULSE;
extern const int MSG_LOADGAMEMODE;
extern const int MSG_GAMEMODEMSG;
extern const int MSG_GOTMYCLIENTID;
extern const int MSG_LOADEDGAMEMODE;
extern const int MSG_LCMSG;
