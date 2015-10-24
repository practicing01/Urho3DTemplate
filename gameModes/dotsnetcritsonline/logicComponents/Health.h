/*
 * Health.h
 *
 *  Created on: Oct 23, 2015
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

class Health : public LogicComponent
{
	OBJECT(Health, LogicComponent);
public:
	Health(Context* context, Urho3DPlayer* main);
	~Health();
	virtual void Start();

	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void HandleModifyHealth(StringHash eventType, VariantMap& eventData);
	void ModifyHealth(int healthMod, int operation, bool sendToServer);

	Urho3DPlayer* main_;
	VectorBuffer msg_;

	int health_;
};
