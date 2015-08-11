/*
 * RTSCamera.cpp
 *
 *  Created on: Aug 11, 2015
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

#include "RTSCamera.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

RTSCamera::RTSCamera(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	distance_ = 2.5f;
	cameraNode_ = NULL;
	modelNode_ = NULL;
	angle_ = 45.0f;
	touchSubscriberCount_ = 0;
	dir_ = Vector3::ZERO;
	moving_ = false;
	speed_ = 5.0f;
}

RTSCamera::~RTSCamera()
{
}

void RTSCamera::Start()
{
	if (!main_->IsLocalClient(node_))
	{
		return;
	}

	SubscribeToEvent(E_TOUCHSUBSCRIBE, HANDLER(RTSCamera, HandleTouchSubscribe));
	SubscribeToEvent(E_TOUCHUNSUBSCRIBE, HANDLER(RTSCamera, HandleTouchUnSubscribe));
	SubscribeToEvent(E_SETCLIENTCAMERA, HANDLER(RTSCamera, HandleSetCamera));
	SubscribeToEvent(E_SETCLIENTMODELNODE, HANDLER(RTSCamera, HandleSetClientModelNode));

	VariantMap vm0;
	vm0[GetClientModelNode::P_NODE] = node_;
	SendEvent(E_GETCLIENTMODELNODE, vm0);

	VariantMap vm;
	vm[GetClientCamera::P_NODE] = node_;
	SendEvent(E_GETCLIENTCAMERA, vm);

	camOrigin_ = modelNode_->GetPosition();
	camOrigin_.y_ += distance_;
	camOrigin_.z_ -= distance_;
	cameraNode_->SetWorldPosition(camOrigin_);
	cameraNode_->SetRotation(Quaternion(angle_, 0.0f, 0.0f));
	scene_ = node_->GetScene();

	SubscribeToEvent(E_TOUCHBEGIN, HANDLER(RTSCamera, HandleTouchBegin));
	SubscribeToEvent(E_TOUCHMOVE, HANDLER(RTSCamera, HandleTouchMove));
	SubscribeToEvent(E_TOUCHEND, HANDLER(RTSCamera, HandleTouchEnd));
	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void RTSCamera::HandleTouchSubscribe(StringHash eventType, VariantMap& eventData)
{
	touchSubscriberCount_++;//Hazardous if a component gets deleted and it doesn't unsubscribe.
}

void RTSCamera::HandleTouchUnSubscribe(StringHash eventType, VariantMap& eventData)
{
	touchSubscriberCount_--;
	if (touchSubscriberCount_ < 0)
	{
		touchSubscriberCount_ = 0;
	}
}

void RTSCamera::HandleSetCamera(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientCamera::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		cameraNode_ = (Node*)(eventData[SetClientCamera::P_CAMERANODE].GetPtr());

		camOrigin_ = cameraNode_->GetPosition();

		UnsubscribeFromEvent(E_SETCLIENTCAMERA);
	}
}

void RTSCamera::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		modelNode_ = (Node*)(eventData[SetClientModelNode::P_MODELNODE].GetPtr());

		UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
	}
}

void RTSCamera::FixedUpdate(float timeStep)
{
	if (!moving_){return;}

	if (!modelNode_)
	{
		return;
	}

	if (!cameraNode_)
	{
		return;
	}

	Vector3 projection = modelNode_->GetPosition() + (dir_ * (speed_ * timeStep));
	modelNode_->SetPosition(projection);
}

void RTSCamera::HandleTouchBegin(StringHash eventType, VariantMap& eventData)
{
	if (main_->ui_->GetFocusElement() || touchSubscriberCount_)
	{
		return;
	}

	if (!scene_)
	{
		return;
	}

	if (!modelNode_)
	{
		return;
	}

	using namespace TouchBegin;

	cameraNode_->SetRotation(Quaternion(90.0f, 0.0f, 0.0f));

	victoria_ = cameraNode_->GetComponent<Camera>()->
			ScreenToWorldPoint(Vector3(
					( ( (float) main_->graphics_->GetWidth() ) * 0.5f ) / ( (float)(main_->graphics_->GetWidth()) ),
					( ( (float) main_->graphics_->GetHeight() ) * 0.5f ) / ( (float)(main_->graphics_->GetHeight()) ),
					0.0f));

	Vector3 dest = cameraNode_->GetComponent<Camera>()->
			ScreenToWorldPoint(Vector3(
					(float) eventData[P_X].GetInt() / main_->graphics_->GetWidth(),
					(float) eventData[P_Y].GetInt() / main_->graphics_->GetHeight(),
					0.0f));

	dir_ = (dest - victoria_).Normalized();
	moving_ = true;

	cameraNode_->SetRotation(Quaternion(angle_, 0.0f, 0.0f));
}

void RTSCamera::HandleTouchMove(StringHash eventType, VariantMap& eventData)
{
	if (main_->ui_->GetFocusElement() || touchSubscriberCount_)
	{
		return;
	}

	if (!scene_)
	{
		return;
	}

	if (!modelNode_)
	{
		return;
	}

	using namespace TouchMove;

	cameraNode_->SetRotation(Quaternion(90.0f, 0.0f, 0.0f));

	victoria_ = cameraNode_->GetComponent<Camera>()->
			ScreenToWorldPoint(Vector3(
					( ( (float) main_->graphics_->GetWidth() ) * 0.5f ) / ( (float)(main_->graphics_->GetWidth()) ),
					( ( (float) main_->graphics_->GetHeight() ) * 0.5f ) / ( (float)(main_->graphics_->GetHeight()) ),
					0.0f));

	Vector3 dest = cameraNode_->GetComponent<Camera>()->
			ScreenToWorldPoint(Vector3(
					(float) eventData[P_X].GetInt() / main_->graphics_->GetWidth(),
					(float) eventData[P_Y].GetInt() / main_->graphics_->GetHeight(),
					0.0f));

	dir_ = (dest - victoria_).Normalized();
	moving_ = true;

	cameraNode_->SetRotation(Quaternion(angle_, 0.0f, 0.0f));
}

void RTSCamera::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
{
	if (!moving_){return;}

	moving_ = false;
}
