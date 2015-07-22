/*
 * Silence.h
 *
 *  Created on: Jul 19, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../../../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>

using namespace Urho3D;

class Silence : public LogicComponent
{
	OBJECT(Silence);
public:
	Silence(Context* context, Urho3DPlayer* main);
	~Silence();
	virtual void Start();
	void HandleGetSilence(StringHash eventType, VariantMap& eventData);
	void HandleSetIsServer(StringHash eventType, VariantMap& eventData);
	void HandleSetClientID(StringHash eventType, VariantMap& eventData);
	void HandleSetConnection(StringHash eventType, VariantMap& eventData);
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void HandleModifySilence(StringHash eventType, VariantMap& eventData);
	void ModifySilence(bool state, bool sendToServer);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	bool isServer_;
	int clientID_;
	Connection* conn_;

	bool silence_;
};
