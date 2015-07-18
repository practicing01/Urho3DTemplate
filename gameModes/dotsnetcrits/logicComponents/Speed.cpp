/*
 * Speed.cpp
 *
 *  Created on: Jul 13, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>

#include "Speed.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

Speed::Speed(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	speed_ = 10.0f;
}

Speed::~Speed()
{
}

void Speed::Start()
{
	SubscribeToEvent(E_GETCLIENTSPEED, HANDLER(Speed, HandleGetSpeed));
}

void Speed::HandleGetSpeed(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientSpeed::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetClientSpeed::P_NODE] = clientNode;
		vm[SetClientSpeed::P_SPEED] = speed_;
		SendEvent(E_SETCLIENTSPEED, vm);
	}
}
