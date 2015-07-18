/*
 * ServerInfo.h
 *
 *  Created on: Jun 30, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>

using namespace Urho3D;

class ServerInfo
{
public:
	String serverName_;
	String gameMode_;
	String address_;
	Connection* connection_;
};
