/*
 * ClientInfo.h
 *
 *  Created on: Jul 5, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>

using namespace Urho3D;

class ClientInfo : public LogicComponent
{
	OBJECT(ClientInfo);
public:
	ClientInfo(Context* context, Urho3DPlayer* main, int clientID, Connection* connection);
	~ClientInfo();
	virtual void Start();
    void HandleClientSync(StringHash eventType, VariantMap& eventData);
    void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
	void HandleGetClientID(StringHash eventType, VariantMap& eventData);
	void HandleGetConnection(StringHash eventType, VariantMap& eventData);
	void HandleGetSceneNodeClientID(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	int clientID_;
	Connection* connection_;
};
