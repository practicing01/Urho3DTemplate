/*
 * TimedRemove.h
 *
 *  Created on: Jul 21, 2015
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

class TimedRemove : public LogicComponent
{
	OBJECT(TimedRemove, LogicComponent);
public:
	TimedRemove(Context* context, Urho3DPlayer* main, float lifeTime);
	~TimedRemove();
	virtual void Start();
	void HandleUpdate(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;
	float elapsedTime_;
	float lifeTime_;
};
