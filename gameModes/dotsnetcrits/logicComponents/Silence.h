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

	Urho3DPlayer* main_;

	float silence_;
};
