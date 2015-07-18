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
	OBJECT(Speed);
public:
	Speed(Context* context, Urho3DPlayer* main);
	~Speed();
	virtual void Start();
	void HandleGetSpeed(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	float speed_;
};
