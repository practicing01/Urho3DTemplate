/*
 * RotateTo.cpp
 *
 *  Created on: Jul 15, 2015
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

#include "RotateTo.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

RotateTo::RotateTo(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	isRotating_ = false;
	modelNode_ = NULL;
}

RotateTo::~RotateTo()
{
}

void RotateTo::Start()
{
	//SubscribeToURHO3D_EVENT(E_SETCLIENTMODELNODE, URHO3D_HANDLER(RotateTo, HandleSetClientModelNode));
	SubscribeToEvent(E_ROTATEMODELNODE, URHO3D_HANDLER(RotateTo, HandleRotateModelNode));

	/*VariantMap vm;
	vm[GetClientModelNode::P_NODE] = node_;
	SendURHO3D_EVENT(E_GETCLIENTMODELNODE, vm);*/

	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void RotateTo::OnRotateToComplete()
{
	//todo
}

void RotateTo::RotateModelNodeTo(Quaternion dest, float speed, float speedRamp, bool stopOnCompletion)
{
	rotateToDest_ = dest;
	rotateToLoc_ = node_->GetRotation();
	rotateToSpeed_ = speed;
	rotateToSpeedRamp_ = speedRamp;

	rotateToTravelTime_ = 1.0f / rotateToSpeed_;

	inderp_ = 0.0f;
	rotateToElapsedTime_ = 0.0f;
	rotateToStopOnTime_ = stopOnCompletion;
	isRotating_ = true;

	if (rotateToSpeedRamp_ < rotateToSpeed_)
	{
		rotateToSpeedRamp_ = rotateToSpeed_;
	}

}

void RotateTo::FixedUpdate(float timeStep)
{
	if (isRotating_ == true)
	{
		if (rotateToSpeedRamp_ > rotateToSpeed_)
		{
			rotateToSpeedRamp_ -= timeStep;
			if (rotateToSpeedRamp_ < rotateToSpeed_)
			{
				rotateToSpeedRamp_ = rotateToSpeed_;
			}
		}

		inderp_ += rotateToSpeedRamp_*timeStep;
		node_->SetRotation(rotateToLoc_.Slerp(rotateToDest_, inderp_));
		rotateToElapsedTime_ += timeStep;
		if (rotateToElapsedTime_ >= rotateToTravelTime_)
		{
			isRotating_ = false;
			if (rotateToStopOnTime_ == true)
			{
			}
			OnRotateToComplete();
		}
	}
}

void RotateTo::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		modelNode_ = (Node*)(eventData[SetClientModelNode::P_MODELNODE].GetPtr());
	}

	UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
}

void RotateTo::HandleRotateModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[RotateModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		Quaternion dest = eventData[RotateModelNode::P_ROTATION].GetQuaternion();
		float speed = eventData[RotateModelNode::P_SPEED].GetFloat();
		float speedRamp = eventData[RotateModelNode::P_SPEEDRAMP].GetFloat();
		bool stopOnCompletion = eventData[RotateModelNode::P_STOPONCOMPLETION].GetBool();
		RotateModelNodeTo(dest, speed, speedRamp, stopOnCompletion);
	}
}
