/*
 * ModelPlayer.h
 *
 *  Created on: Jul 11, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../../../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>

using namespace Urho3D;

class ModelPlayer : public LogicComponent
{
	OBJECT(ModelPlayer);
public:
	ModelPlayer(Context* context, Urho3DPlayer* main);
	~ModelPlayer();
	virtual void Start();
	void LoadDefaultPlayer();
	void LoadPlayer(String modelFilename);
	void RecursiveSetAnimation(Node* noed, String ani, bool loop, unsigned char layer);
	void RemoveModelNode();
	void HandleGetCamera(StringHash eventType, VariantMap& eventData);
	void HandleGetClientModelNode(StringHash eventType, VariantMap& eventData);
	void HandleRespawnSceneNode(StringHash eventType, VariantMap& eventData);
	void HandleAnimateSceneNode(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

    String modelFilename_;
    SharedPtr<Node> modelNode_;
	SharedPtr<Scene> scene_;
};
