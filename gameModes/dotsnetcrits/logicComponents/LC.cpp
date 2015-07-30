/*
 * LC.cpp
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

#include "LC.h"

LC::LC(Context* context, Urho3DPlayer* main, Network* network) :
	Object(context)
{
	main_ = main;
	network_ = network;
}

LC::~LC()
{
}

void LC::Start(Node* sceneNode)
{
	sceneNode_ = SharedPtr<Node>(sceneNode);
	modelNode_ = NULL;
	cameraNode_ = NULL;
	clientID_ = -1;
	isServer_ = false;
	clientExecuting_ = false;
	cooldown_ = 0.0f;
	disabled_ = false;
	lagTime_ = 0.0f;

	scene_ = sceneNode_->GetScene();

	//Get isServer.
	SubscribeToEvent(E_SETISSERVER, HANDLER(LC, HandleSetIsServer));

	VariantMap vm;
	SendEvent(E_GETISSERVER, vm);

	//Get camera.
	SubscribeToEvent(E_SETCLIENTCAMERA, HANDLER(LC, HandleSetCamera));

	vm.Clear();
	vm[GetClientCamera::P_NODE] = sceneNode_;
	SendEvent(E_GETCLIENTCAMERA, vm);

	//Get modelNode.
	SubscribeToEvent(E_SETCLIENTMODELNODE, HANDLER(LC, HandleSetClientModelNode));

	vm.Clear();
	vm[GetClientModelNode::P_NODE] = sceneNode_;
	SendEvent(E_GETCLIENTMODELNODE, vm);

	//Get clientID.
	SubscribeToEvent(E_SETCLIENTID, HANDLER(LC, HandleSetClientID));

	vm.Clear();
	vm[GetClientID::P_NODE] = main_->GetRootNode(sceneNode_);
	SendEvent(E_GETCLIENTID, vm);

	//Get connection.
	SubscribeToEvent(E_SETCONNECTION, HANDLER(LC, HandleSetConnection));

	vm.Clear();
	vm[GetConnection::P_NODE] = main_->GetRootNode(sceneNode_);
	SendEvent(E_GETCONNECTION, vm);
}

void LC::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();

	UnsubscribeFromEvent(E_SETISSERVER);
}

void LC::HandleSetCamera(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetClientCamera::P_NODE].GetPtr());

	if (sceneNode == sceneNode_)
	{
		cameraNode_ = (Node*)(eventData[SetClientCamera::P_CAMERANODE].GetPtr());

		UnsubscribeFromEvent(E_SETCLIENTCAMERA);
	}
}

void LC::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetClientModelNode::P_NODE].GetPtr());

	if (sceneNode == sceneNode_)
	{
		modelNode_ = (Node*)(eventData[SetClientModelNode::P_MODELNODE].GetPtr());
		//Set bounding box.
		beeBox_ = modelNode_->GetComponent<AnimatedModel>()->GetWorldBoundingBox();
		UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
	}
}

void LC::HandleSetClientID(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetClientID::P_NODE].GetPtr());

	if (main_->GetSceneNode(sceneNode) == sceneNode_)
	{
		clientID_ = eventData[SetClientID::P_CLIENTID].GetInt();
		UnsubscribeFromEvent(E_SETCLIENTID);
	}
}

void LC::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(sceneNode) == sceneNode_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());
		UnsubscribeFromEvent(E_SETCONNECTION);
	}
}

void LC::HandleSetLagTime(StringHash eventType, VariantMap& eventData)
{
	Connection* sender = (Connection*)(eventData[SetLagTime::P_CONNECTION].GetPtr());
	if (sender == targetConn_)
	{
		lagTime_ = eventData[SetLagTime::P_LAGTIME].GetFloat();
		UnsubscribeFromEvent(E_SETLAGTIME);
	}
}

void LC::GetLagTime(Connection* targetConn)
{
	targetConn_ = targetConn;
	lagTime_ = 0.0f;

	SubscribeToEvent(E_SETLAGTIME, HANDLER(LC, HandleSetLagTime));

	VariantMap vm;
	vm[GetLagTime::P_CONNECTION] = targetConn_;
	SendEvent(E_GETLAGTIME, vm);
}

int LC::GetModelNodeClientID(Node* modelNode)
{
	targetModelNode_ = modelNode;
	targetClientID_ = -1;

	SubscribeToEvent(E_SETSCENENODEBYMODELNODE, HANDLER(LC, HandleSetTargetSceneNodeByModelNode));

	VariantMap vm;
	vm[GetSceneNodeByModelNode::P_NODE] = targetModelNode_;
	SendEvent(E_GETSCENENODEBYMODELNODE, vm);

	return targetClientID_;
}

void LC::HandleSetTargetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* modelNode = (Node*)(eventData[SetSceneNodeByModelNode::P_MODELNODE].GetPtr());

	if (modelNode == targetModelNode_)
	{
		Node* sceneNode = (Node*)(eventData[SetSceneNodeByModelNode::P_SCENENODE].GetPtr());
		targetClientID_ = main_->GetClientID(sceneNode);
		UnsubscribeFromEvent(E_SETSCENENODEBYMODELNODE);
	}
}
