/*
 * NodeInfo.cpp
 *
 *  Created on: Oct 19, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>

#include "NodeInfo.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

NodeInfo::NodeInfo(Context* context, Urho3DPlayer* main, String LC, int clientID, int nodeID) :
	LogicComponent(context)
{
	main_ = main;
	LC_ = LC;
	clientID_ = clientID;
	nodeID_ = nodeID;
}

NodeInfo::~NodeInfo()
{
}

void NodeInfo::Start()
{
}
