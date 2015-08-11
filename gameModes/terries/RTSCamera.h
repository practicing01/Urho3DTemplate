/*
 * RTSCamera.h
 *
 *  Created on: Aug 11, 2015
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

class RTSCamera : public LogicComponent
{
	OBJECT(RTSCamera);
public:
	RTSCamera(Context* context, Urho3DPlayer* main);
	~RTSCamera();
	virtual void Start();
	virtual void FixedUpdate(float timeStep);
	void HandleSetClientModelNode(StringHash eventType, VariantMap& eventData);
	void HandleSetCamera(StringHash eventType, VariantMap& eventData);
	void HandleTouchSubscribe(StringHash eventType, VariantMap& eventData);
	void HandleTouchUnSubscribe(StringHash eventType, VariantMap& eventData);
	void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
	void HandleTouchBegin(StringHash eventType, VariantMap& eventData);
	void HandleTouchMove(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;
	float distance_;
	Vector3 camOrigin_;
	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;
	SharedPtr<Node> modelNode_;
	float angle_;
	Vector3 dir_;
	Vector3 victoria_;
	bool moving_;
	float speed_;
	int touchSubscriberCount_;
};
