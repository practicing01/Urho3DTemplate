/*
 * DOT.h
 *
 *  Created on: Jul 23, 2015
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

class DOT : public LogicComponent
{
	OBJECT(DOT);
public:
	DOT(Context* context, Urho3DPlayer* main);
	~DOT();
	virtual void Start();
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandleSetIsServer(StringHash eventType, VariantMap& eventData);
	void HandleSetCamera(StringHash eventType, VariantMap& eventData);
	void HandleSetClientModelNode(StringHash eventType, VariantMap& eventData);
	void HandleSetClientID(StringHash eventType, VariantMap& eventData);
	void HandleSetConnection(StringHash eventType, VariantMap& eventData);
	void HandleMechanicRequest(StringHash eventType, VariantMap& eventData);
	void HandleSetSilence(StringHash eventType, VariantMap& eventData);
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
    void HandleCleanseStatus(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void HandleSetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData);
	void HandleSetSceneNodeClientID(StringHash eventType, VariantMap& eventData);
	void HandleSetModelNodeBySceneNode(StringHash eventType, VariantMap& eventData);
	void HandleSetLagTime(StringHash eventType, VariantMap& eventData);
	void StartDOT(int clientID, float timeRamp, bool sendToServer);
	void HandleSetArmor(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	bool isServer_;

	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;
	SharedPtr<Node> modelNode_;
	int clientID_;
	Connection* conn_;
	float lagTime_;

	ParticleEmitter* emitterEndFX_;
	SharedPtr<Node> particleEndNode_;
	BoundingBox beeBox_;
	Vector3 victoria_;
	float elapsedTime_;
	bool clientExecuting_;
	float cooldown_;
	bool silence_;
	float DOTDuration_;
	float DOTElapsedTime_;
	float DOTInterval_;
	float DOTIntervalElapsedTime_;
	bool DOTed_;
	int targetClientID_;
	Node* targetModelNode_;
	Node* targetSceneNode_;
	int targetArmor_;
	int damage_;
};
