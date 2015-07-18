/*
 * ServerQuery.h
 *
 *  Created on: Jul 3, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>

class ServerQuery
{
public:
	ServerQuery();
	Connection* connection_;
	int index_;
};
