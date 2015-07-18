/*
 * MasterServer.h
 *
 *  Created on: Jun 29, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>

#include "ServerInfo.h"
#include "ServerQuery.h"

using namespace Urho3D;

class MasterServer : public LogicComponent
{
	OBJECT(MasterServer);
public:
	MasterServer(Context* context, Urho3DPlayer* main);
	~MasterServer();

	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	virtual void Start();
	void HandleClientDisconnect(StringHash eventType, VariantMap& eventData);
	void HandleClientConnect(StringHash eventType, VariantMap& eventData);
    void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;
	float elapsedTime_;

	Network* network_;

	Vector<ServerInfo*> servers_;
	Vector<ServerQuery*> serverQueries_;

	VectorBuffer msg_;
};
