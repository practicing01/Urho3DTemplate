/*
 * TopDownCamera.cpp
 *
 *  Created on: Jul 19, 2015
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

#include "TopDownCamera.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

TopDownCamera::TopDownCamera(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	distance_ = 10.0f;
	cameraNode_ = NULL;
	modelNode_ = NULL;
	isEnabled_ = false;
}

TopDownCamera::~TopDownCamera()
{
}

void TopDownCamera::Start()
{
	if (!main_->IsLocalClient(node_))
	{
		return;
	}

	SubscribeToEvent(E_SETCLIENTCAMERA, HANDLER(TopDownCamera, HandleSetCamera));
	SubscribeToEvent(E_SETCLIENTMODELNODE, HANDLER(TopDownCamera, HandleSetClientModelNode));
	SubscribeToEvent(E_MECHANICREQUEST, HANDLER(TopDownCamera, HandleMechanicRequest));

	VariantMap vm0;
	vm0[GetClientModelNode::P_NODE] = node_;
	SendEvent(E_GETCLIENTMODELNODE, vm0);

	VariantMap vm;
	vm[GetClientCamera::P_NODE] = node_;
	SendEvent(E_GETCLIENTCAMERA, vm);
}

void TopDownCamera::HandleSetCamera(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientCamera::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		cameraNode_ = (Node*)(eventData[SetClientCamera::P_CAMERANODE].GetPtr());

		camOrigin_ = cameraNode_->GetPosition();

		UnsubscribeFromEvent(E_SETCLIENTCAMERA);
	}
}

void TopDownCamera::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		modelNode_ = (Node*)(eventData[SetClientModelNode::P_MODELNODE].GetPtr());
		beeBox_ = modelNode_->GetComponent<AnimatedModel>()->GetWorldBoundingBox();

		UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
	}
}

void TopDownCamera::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	if (!cameraNode_){return;}

	float timeStep = eventData[PostUpdate::P_TIMESTEP].GetFloat();

	cameraNode_->SetWorldRotation(Quaternion(90.0f, 0.0f, 0.0f));
}

void TopDownCamera::HandleMechanicRequest(StringHash eventType, VariantMap& eventData)
{
	String mechanicID = eventData[MechanicRequest::P_MECHANICID].GetString();
	if (mechanicID == "CameraToggle")
	{
		if (!isEnabled_)
		{
			isEnabled_ = true;

			camOrigin_ = modelNode_->GetPosition();
			camOrigin_.y_ += beeBox_.Size().y_ * distance_;
			cameraNode_->SetWorldPosition(camOrigin_);
			cameraNode_->SetRotation(Quaternion(90.0f, 0.0f, 0.0f));
			SubscribeToEvent(E_POSTUPDATE, HANDLER(TopDownCamera, HandlePostUpdate));
		}
		else
		{
			isEnabled_ = false;

			UnsubscribeFromEvent(E_POSTUPDATE);
		}
	}
}
