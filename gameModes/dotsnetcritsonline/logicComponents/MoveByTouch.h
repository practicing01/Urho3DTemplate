/*
 * MoveByTouch.h
 *
 *  Created on: Jul 12, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../../../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/CollisionShape.h>

using namespace Urho3D;

class MoveByTouch : public LogicComponent
{
	OBJECT(MoveByTouch, LogicComponent);
public:
	MoveByTouch(Context* context, Urho3DPlayer* main);
	~MoveByTouch();
	virtual void Start();
	virtual void FixedUpdate(float timeStep);
	void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
	void HandleTouchSubscribe(StringHash eventType, VariantMap& eventData);
	void HandleTouchUnSubscribe(StringHash eventType, VariantMap& eventData);
	void HandleNodeCollision(StringHash eventType, VariantMap& eventData);
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void HandleSetLagTime(StringHash eventType, VariantMap& eventData);
	void HandleMoveModelNode(StringHash eventType, VariantMap& eventData);
	void OnMoveToComplete();
	void MoveTo(Vector3 dest, float speed, float speedRamp, float gravity, float gravityRamp, bool rotate, bool sendToServer);

	Urho3DPlayer* main_;
	float lagTime_;

	//Some components may not want to process touches if other components are going to.
	//If a component is going to, it can add to this counter to let the other components know.
	int touchSubscriberCount_;

	bool isMoving_;
	float speed_;
	float speedRamp_;
	float gravity_;
	float gravityRamp_;
	float moveToTravelTime_;
	float moveToElapsedTime_;
	float inderp_;
	float remainingDist_;
	float radius_;
	Vector3 dest_;
	Vector3 loc_;
	Vector3 dir_;
	bool rotate_;

	//SharedPtr<Node> debugNode_;
};
