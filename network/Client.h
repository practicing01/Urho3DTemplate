/*
 * Client.h
 *
 *  Created on: Jul 8, 2015
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

class Client : public LogicComponent
{
	OBJECT(Client, LogicComponent);
public:
	Client(Context* context, Urho3DPlayer* main);
	~Client();
	virtual void Start();
    void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
    void LoadGameMode(String gameMode);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;
};
