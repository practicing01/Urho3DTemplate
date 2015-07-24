/*
 * LCTarget.h
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

class LCTarget : public Object
{
	OBJECT(LCTarget);
public:
	LCTarget(Context* context);
	~LCTarget();
	void HandleSetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData);
	void HandleSetSceneNodeClientID(StringHash eventType, VariantMap& eventData);
	void HandleSetModelNodeBySceneNode(StringHash eventType, VariantMap& eventData);
	void GetSceneNodeByModelNode();
	void GetSceneNodeClientID();
	void GetModelNodeBySceneNode();

	Node* modelNode_;
	Node* sceneNode_;
	int clientID_;

	ParticleEmitter* emitterEndFX_;
	SharedPtr<Node> particleEndNode_;
	BoundingBox beeBox_;

	float duration_;
	float elapsedTime_;
};
