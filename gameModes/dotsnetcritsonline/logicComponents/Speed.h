/*
 * Speed.h
 *
 *  Created on: Jul 13, 2015
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

class Speed : public LogicComponent
{
	URHO3D_OBJECT(Speed, LogicComponent);
public:
	Speed(Context* context, Urho3DPlayer* main);
	~Speed();
	virtual void Start();

	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void HandleModifySpeed(StringHash eventType, VariantMap& eventData);
	void ModifySpeed(float speedMod, int operation, bool sendToServer);

	Urho3DPlayer* main_;
	VectorBuffer msg_;

	float speed_;
};
