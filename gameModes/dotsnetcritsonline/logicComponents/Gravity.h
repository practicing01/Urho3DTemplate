/*
 * Gravity.h
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

class Gravity : public LogicComponent
{
	OBJECT(Gravity, LogicComponent);
public:
	Gravity(Context* context, Urho3DPlayer* main);
	~Gravity();
	virtual void Start();
	void HandleGetGravity(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	float gravity_;
};
