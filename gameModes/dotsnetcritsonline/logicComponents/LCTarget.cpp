/*
 * LCTarget.cpp
 *
 *  Created on: Jul 24, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource3D.h>

#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

#include "LCTarget.h"

LCTarget::LCTarget(Context* context) :
	Object(context)
{
	elapsedTime_ = 0.0f;
	duration_ = 0.0f;
	interval_ = 0.0f;
	intervalElapsedTime_ = 0.0f;
}

LCTarget::~LCTarget()
{
	if (particleEndNode_)
	{
		particleEndNode_->Remove();
	}
}

void LCTarget::HandleSetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* modelNode = (Node*)(eventData[SetSceneNodeByModelNode::P_MODELNODE].GetPtr());

	if (modelNode == modelNode_)
	{
		sceneNode_ = (Node*)(eventData[SetSceneNodeByModelNode::P_SCENENODE].GetPtr());
		UnsubscribeFromEvent(E_SETSCENENODEBYMODELNODE);
	}
}

void LCTarget::HandleSetSceneNodeClientID(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetSceneNodeClientID::P_NODE].GetPtr());

	if (sceneNode == sceneNode_)
	{
		clientID_ = eventData[SetSceneNodeClientID::P_CLIENTID].GetInt();
		UnsubscribeFromEvent(E_SETSCENENODECLIENTID);
	}
}

void LCTarget::HandleSetModelNodeBySceneNode(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetModelNodeBySceneNode::P_SCENENODE].GetPtr());

	if (sceneNode == sceneNode_)
	{
		modelNode_ = (Node*)(eventData[SetModelNodeBySceneNode::P_MODELNODE].GetPtr());
		UnsubscribeFromEvent(E_SETMODELNODEBYSCENENODE);
	}
}

void LCTarget::GetSceneNodeByModelNode()
{
	SubscribeToEvent(E_SETSCENENODEBYMODELNODE, HANDLER(LCTarget, HandleSetSceneNodeByModelNode));

	VariantMap vm;
	vm[GetSceneNodeByModelNode::P_NODE] = modelNode_;
	SendEvent(E_GETSCENENODEBYMODELNODE, vm);
}

void LCTarget::GetSceneNodeClientID()
{
	SubscribeToEvent(E_SETSCENENODECLIENTID, HANDLER(LCTarget, HandleSetSceneNodeClientID));

	VariantMap vm;
	vm[GetSceneNodeClientID::P_NODE] = sceneNode_;
	SendEvent(E_GETSCENENODECLIENTID, vm);
}

void LCTarget::GetModelNodeBySceneNode()
{
	SubscribeToEvent(E_SETMODELNODEBYSCENENODE, HANDLER(LCTarget, HandleSetModelNodeBySceneNode));

	VariantMap vm;
	vm[GetModelNodeBySceneNode::P_NODE] = sceneNode_;
	SendEvent(E_GETMODELNODEBYSCENENODE, vm);

	//Set bounding box.
	beeBox_ = modelNode_->GetComponent<AnimatedModel>()->GetWorldBoundingBox();
}
