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
	OBJECT(MoveByTouch);
public:
	MoveByTouch(Context* context, Urho3DPlayer* main);
	~MoveByTouch();
	virtual void Start();
	virtual void FixedUpdate(float timeStep);
	void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
	void HandleTouchSubscribe(StringHash eventType, VariantMap& eventData);
	void HandleTouchUnSubscribe(StringHash eventType, VariantMap& eventData);
	void HandleSetCamera(StringHash eventType, VariantMap& eventData);
	void HandleNodeCollision(StringHash eventType, VariantMap& eventData);
	void HandleSetSpeed(StringHash eventType, VariantMap& eventData);
	void HandleSetGravity(StringHash eventType, VariantMap& eventData);
	void HandleSetClientModelNode(StringHash eventType, VariantMap& eventData);
	void HandleSetClientID(StringHash eventType, VariantMap& eventData);
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleSetLagTime(StringHash eventType, VariantMap& eventData);
	void HandleSetConnection(StringHash eventType, VariantMap& eventData);
	void HandleSetIsServer(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void OnMoveToComplete();
	void MoveTo(Vector3 dest, float speed, float speedRamp, float gravity, float gravityRamp, bool stopOnCompletion, bool sendToServer);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	bool isServer_;

	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;
	SharedPtr<Node> modelNode_;
	int clientID_;
	Connection* conn_;

	//Some components may not want to process touches if other components are going to.
	//If a component is going to, it can add to this counter to let the other components know.
	int touchSubscriberCount_;

	float clientSpeed_;
	float clientGravity_;
	Vector3 moveToVector_;
	float lagTime_;

	Vector3 vectoria_;
	Vector3 victoria_;
	Quaternion quarterOnion_;
	Quaternion quarterPounder_;

	bool moveToStopOnTime_;
	bool isMoving_;
	float moveToSpeed_;
	float speedRamp_;
	float gravity_;
	float gravityRamp_;
	float moveToTravelTime_;
	float moveToElapsedTime_;
	float inderp_;
	float remainingDist_;
	float radius_;
	Vector3 moveToDest_;
	Vector3 moveToLoc_;
	Vector3 moveToDir_;
	Ray rae_;
	PhysicsRaycastResult raeResult_;
	BoundingBox beeBox_;
};
