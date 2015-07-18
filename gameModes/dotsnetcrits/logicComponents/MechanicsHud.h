/*
 * MechanicsHud.h
 *
 *  Created on: Jul 10, 2015
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

class MechanicsHud : public LogicComponent
{
	OBJECT(MechanicsHud);
public:
	MechanicsHud(Context* context, Urho3DPlayer* main);
	~MechanicsHud();
	virtual void Start();
	void HandleRelease(StringHash eventType, VariantMap& eventData);
	void HandleGameModeRemoved(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	SharedPtr<UIElement> mechanicsHUD_;
};
