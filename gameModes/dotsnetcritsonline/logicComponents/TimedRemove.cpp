/*
 * TimedRemove.cpp
 *
 *  Created on: Jul 21, 2015
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

#include "TimedRemove.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

TimedRemove::TimedRemove(Context* context, Urho3DPlayer* main, float lifeTime) :
	LogicComponent(context)
{
	main_ = main;
	lifeTime_ = lifeTime;
	elapsedTime_ = 0.0f;
}

TimedRemove::~TimedRemove()
{
}

void TimedRemove::Start()
{
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TimedRemove, HandleUpdate));
}

void TimedRemove::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	elapsedTime_ += timeStep;
	if (elapsedTime_ >= lifeTime_)
	{
		node_->Remove();
	}
}
