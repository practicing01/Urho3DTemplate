/*
 * DotsNetCrits.h
 *
 *  Created on: Jul 6, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>

using namespace Urho3D;

class DotsNetCrits : public LogicComponent
{
	OBJECT(DotsNetCrits);
public:
	DotsNetCrits(Context* context, Urho3DPlayer* main, bool isServer);
	~DotsNetCrits();
	virtual void Start();
    void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
	void HandleDisplayMenu(StringHash eventType, VariantMap& eventData);
	void HandleNewClientID(StringHash eventType, VariantMap& eventData);
	void RespawnNode(SharedPtr<Node> sceneNode, int index = -1);
	void AttachLogicComponents(SharedPtr<Node> sceneNode);
	void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
	void HandleGetIsServer(StringHash eventType, VariantMap& eventData);
	void HandleClientHealthSet(StringHash eventType, VariantMap& eventData);
	void HandleSetSceneNodeClientID(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	bool isServer_;

	SharedPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;

	Vector< SharedPtr<Node> > spawnPoints_;

	Node* targetSceneNode_;
	int targetSceneNodeClientID_;
};
