/*
 * Teleport.h
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
#include <Urho3D/Graphics/ParticleEmitter.h>

using namespace Urho3D;

class Teleport : public LogicComponent
{
	OBJECT(Teleport);
public:
	Teleport(Context* context, Urho3DPlayer* main);
	~Teleport();
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
	void TeleportTo(Vector3 dest, bool sendToServer);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	bool isServer_;

	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;
	SharedPtr<Node> modelNode_;
	int clientID_;
	Connection* conn_;

	ParticleEmitter* emitterStartFX_;
	SharedPtr<Node> particleStartNode_;
	ParticleEmitter* emitterEndFX_;
	SharedPtr<Node> particleEndNode_;
	float radius_;
	float elapsedTime_;
	bool clientExecuting_;
	float cooldown_;
	float lagTime_;
	bool silence_;
    Vector3 victoria_;
	Vector3 vectoria_;
};
