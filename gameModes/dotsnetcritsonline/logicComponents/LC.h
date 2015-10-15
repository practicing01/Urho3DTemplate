/*
 * LC.h
 *
 *  Created on: Jul 24, 2015
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
#include <Urho3D/Graphics/Material.h>

using namespace Urho3D;

class LC : public Object
{
	OBJECT(LC, Object);
public:
	LC(Context* context, Urho3DPlayer* main, Network* network);
	~LC();
	void HandleSetIsServer(StringHash eventType, VariantMap& eventData);
	void HandleSetCamera(StringHash eventType, VariantMap& eventData);
	void HandleSetClientModelNode(StringHash eventType, VariantMap& eventData);
	void HandleSetClientID(StringHash eventType, VariantMap& eventData);
	void HandleSetConnection(StringHash eventType, VariantMap& eventData);
	void HandleSetLagTime(StringHash eventType, VariantMap& eventData);
	void Start(Node* sceneNode);
	void GetLagTime(Connection* targetConn);
	int GetModelNodeClientID(Node* modelNode);
	void HandleSetTargetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	bool isServer_;

	SharedPtr<Scene> scene_;
	SharedPtr<Node> sceneNode_;
	SharedPtr<Node> cameraNode_;
	SharedPtr<Node> modelNode_;

	int clientID_;
	Connection* conn_;
	float lagTime_;

	float elapsedTime_;
	bool clientExecuting_;
	float cooldown_;
	BoundingBox beeBox_;
	bool disabled_;

	Connection* targetConn_;
	int targetClientID_;
	Node* targetModelNode_;
};
