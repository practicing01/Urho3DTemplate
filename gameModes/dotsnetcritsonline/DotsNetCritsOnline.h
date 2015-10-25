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

class DotsNetCritsOnline : public LogicComponent
{
	OBJECT(DotsNetCritsOnline, LogicComponent);
public:
	DotsNetCritsOnline(Context* context, Urho3DPlayer* main, bool isServer, String defaultScene);
	~DotsNetCritsOnline();
	virtual void Start();
    void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
	void HandleDisplayMenu(StringHash eventType, VariantMap& eventData);
	void HandleNewClientID(StringHash eventType, VariantMap& eventData);
	void RespawnNode(SharedPtr<Node> sceneNode, int index = -1);
	void AttachLogicComponents(SharedPtr<Node> sceneNode, int nodeID);
	void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
	void HandleGetIsServer(StringHash eventType, VariantMap& eventData);
	void HandleClientHealthSet(StringHash eventType, VariantMap& eventData);
	void HandleSetSceneNodeClientID(StringHash eventType, VariantMap& eventData);
	void SpawnChicken(int clientID, int nodeID);
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleSetLagTime(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void LoadScene(String fileName);
	void HandleGetSceneName(StringHash eventType, VariantMap& eventData);
	void HandleSetSceneVote(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	bool isServer_;

	SharedPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;

	Vector< SharedPtr<Node> > spawnPoints_;

	String sceneFileName_;

	Node* targetSceneNode_;
	int targetSceneNodeClientID_;

	int nodeIDCounter_;

	float lagTime_;

	int sceneVoteCount_;
	HashMap<String, int> sceneCandidates_;

	int GAMEMODEMSG_SPAWNCHICKEN;
	int GAMEMODEMSG_LOADSCENE;
};
