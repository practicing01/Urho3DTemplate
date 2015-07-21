/*
 * Melee.h
 *
 *  Created on: Jul 21, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../../../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Graphics/ParticleEmitter.h>

using namespace Urho3D;

class Melee : public LogicComponent
{
	OBJECT(Melee);
public:
	Melee(Context* context, Urho3DPlayer* main);
	~Melee();
	virtual void Start();
	void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
	void HandleSetClientModelNode(StringHash eventType, VariantMap& eventData);
	void HandleSetClientID(StringHash eventType, VariantMap& eventData);
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleSetConnection(StringHash eventType, VariantMap& eventData);
	void HandleSetIsServer(StringHash eventType, VariantMap& eventData);
	void HandleMechanicRequest(StringHash eventType, VariantMap& eventData);
	void HandleSetBlind(StringHash eventType, VariantMap& eventData);
	void StartMelee(Vector3 pos, bool sendToServer);
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandleSetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	bool isServer_;

	SharedPtr<Scene> scene_;
	SharedPtr<Node> modelNode_;
	int clientID_;
	Connection* conn_;

	ParticleEmitter* emitterStartFX_;
	SharedPtr<Node> particleStartNode_;
	//ParticleEmitter* emitterEndFX_;
	//SharedPtr<Node> particleEndNode_;
	BoundingBox beeBox_;
	Ray rae_;
	PODVector<PhysicsRaycastResult> raeResult_;
	PODVector<RigidBody*> rigidBodies_;
	float radius_;
	float elapsedTime_;
	bool clientExecuting_;
	float cooldown_;
	bool blind_;
	Vector3 victoria_;
	Node* targetSceneNode_;
	Node* targetModelNode_;
	int damage_;
};
