/*
 * ThirdPersonCamera.h
 *
 *  Created on: Jul 11, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../../../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>

using namespace Urho3D;

class ThirdPersonCamera : public LogicComponent
{
	OBJECT(ThirdPersonCamera);
public:
	ThirdPersonCamera(Context* context, Urho3DPlayer* main);
	~ThirdPersonCamera();
	virtual void Start();
	void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
	virtual void OnSetEnabled();
	void HandleSetCamera(StringHash eventType, VariantMap& eventData);
	void HandleSetClientModelNode(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;
	SharedPtr<Node> modelNode_;

    float rayDistance_;
    float inderp_;
    float remainingDist_;
	Vector3 victoria_;
	Vector3 vectoria_;
	Vector3 camOrigin_;
	BoundingBox beeBox_;
	Ray cameraRay_;
};
