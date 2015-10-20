/*
 * ChickenNPC.h
 *
 *  Created on: Oct 19, 2015
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

class ChickenNPC : public LogicComponent
{
	OBJECT(ChickenNPC, LogicComponent);
public:
	ChickenNPC(Context* context, Urho3DPlayer* main);
	~ChickenNPC();
	virtual void Start();

	Urho3DPlayer* main_;
};
