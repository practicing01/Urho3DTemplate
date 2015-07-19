/*
 * TopDownCamera.h
 *
 *  Created on: Jul 19, 2015
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

class TopDownCamera : public LogicComponent
{
	OBJECT(TopDownCamera);
public:
	TopDownCamera(Context* context, Urho3DPlayer* main);
	~TopDownCamera();
	virtual void Start();
	void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
	void HandleSetClientModelNode(StringHash eventType, VariantMap& eventData);
	void HandleSetCamera(StringHash eventType, VariantMap& eventData);
	void HandleMechanicRequest(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;
	float distance_;
	Vector3 camOrigin_;
	BoundingBox beeBox_;
	SharedPtr<Node> cameraNode_;
	SharedPtr<Node> modelNode_;
	bool isEnabled_;
};
