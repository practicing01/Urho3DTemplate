/*
 * ModelController.h
 *
 *  Created on: Oct 12, 2015
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

class ModelController : public LogicComponent
{
	OBJECT(ModelController, LogicComponent);
public:
	ModelController(Context* context, Urho3DPlayer* main);
	~ModelController();
	virtual void Start();
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void LoadDefaultPlayer();
	void LoadPlayer(String modelFilename);
	void RemoveModelNode();
	void HandleAnimateSceneNode(StringHash eventType, VariantMap& eventData);
	void RecursiveSetAnimation(Node* noed, String ani, bool loop, unsigned char layer);

	Urho3DPlayer* main_;
	SharedPtr<Node> modelNode_;
	String modelPath_;
};
