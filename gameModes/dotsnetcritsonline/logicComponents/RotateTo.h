/*
 * RotateTo.h
 *
 *  Created on: Jul 15, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../../../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Math/Quaternion.h>

using namespace Urho3D;

class RotateTo : public LogicComponent
{
	OBJECT(RotateTo, LogicComponent);
public:
	RotateTo(Context* context, Urho3DPlayer* main);
	~RotateTo();
	virtual void Start();
	virtual void FixedUpdate(float timeStep);
	void OnRotateToComplete();
	void RotateModelNodeTo(Quaternion dest, float speed, float speedRamp, bool stopOnCompletion);
	void HandleSetClientModelNode(StringHash eventType, VariantMap& eventData);
	void HandleRotateModelNode(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	SharedPtr<Node> modelNode_;

    VectorBuffer msg_;

	Quaternion rotateToDest_;
	Quaternion rotateToLoc_;
	float rotateToSpeed_;
	float rotateToSpeedRamp_;
	float rotateToTravelTime_;
	float rotateToElapsedTime_;
	float inderp_;
	float remainingDist_;
	bool rotateToStopOnTime_;
	bool isRotating_;
};
