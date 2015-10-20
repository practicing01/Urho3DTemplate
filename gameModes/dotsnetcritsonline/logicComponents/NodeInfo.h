/*
 * NodeInfo.h
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

class NodeInfo : public LogicComponent
{
	OBJECT(NodeInfo, LogicComponent);
public:
	NodeInfo(Context* context, Urho3DPlayer* main, String LC, int clientID, int nodeID);
	~NodeInfo();
	virtual void Start();

	Urho3DPlayer* main_;

	String LC_;
	int clientID_;
	int nodeID_;
};
