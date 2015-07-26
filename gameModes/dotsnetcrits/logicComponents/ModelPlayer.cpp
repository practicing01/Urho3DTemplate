/*
 * ModelPlayer.cpp
 *
 *  Created on: Jul 11, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>

#include "ModelPlayer.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

ModelPlayer::ModelPlayer(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
}

ModelPlayer::~ModelPlayer()
{
	if (modelNode_ != NULL)
	{
		modelNode_->RemoveAllChildren();
		modelNode_->RemoveAllComponents();
		modelNode_->Remove();
	}
}

void ModelPlayer::Start()
{
	modelNode_ = NULL;
	scene_ = node_->GetScene();
	LoadDefaultPlayer();
	RecursiveSetAnimation(modelNode_, "idle1", true, 0);

	SubscribeToEvent(E_GETCLIENTCAMERA, HANDLER(ModelPlayer, HandleGetCamera));
	SubscribeToEvent(E_GETCLIENTMODELNODE, HANDLER(ModelPlayer, HandleGetClientModelNode));
	SubscribeToEvent(E_ANIMATESCENENODE, HANDLER(ModelPlayer, HandleAnimateSceneNode));
	SubscribeToEvent(E_GETSCENENODEBYMODELNODE, HANDLER(ModelPlayer, HandleGetSceneNodeByModelNode));
	SubscribeToEvent(E_GETMODELNODEBYSCENENODE, HANDLER(ModelPlayer, HandleGetModelNodeBySceneNode));
}

void ModelPlayer::RecursiveSetAnimation(Node* noed, String ani, bool loop, unsigned char layer)//todo check if animation exists, if not set default.
{
	if (noed->HasComponent<AnimationController>())
	{
		noed->GetComponent<AnimationController>()->PlayExclusive("Models/Witch3/" + ani + ".ani", layer, loop, 0.25f);//reduce archive size by reusing animations
		//noed->GetComponent<AnimationController>()->PlayExclusive("Models/" + noed->GetName() + "/" + ani + ".ani", layer, loop, 0.25f);//proper way
		if (!loop)
		{
			noed->GetComponent<AnimationController>()->SetAutoFade(ani, 0.25f);
		}
	}

	for (int x = 0; x < noed->GetNumChildren(); x++)
	{
		RecursiveSetAnimation(noed->GetChild(x), ani, loop, layer);
	}
}

void ModelPlayer::LoadDefaultPlayer()
{
	LoadPlayer("Witch3");//todo endless loop possibilities :d
}

void ModelPlayer::LoadPlayer(String modelFilename)
{
	if (!main_->cache_->Exists("Models/" + modelFilename + "/" + modelFilename + ".xml"))
	{
		LoadDefaultPlayer();
		return;
	}

	RemoveModelNode();
    XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>("Models/" + modelFilename + "/" + modelFilename + ".xml");
    modelNode_ = scene_->InstantiateXML(xmlFile->GetRoot(),
    		node_->GetPosition(), node_->GetRotation(), LOCAL);

	RecursiveSetAnimation(modelNode_, "idle1", true, 0);
	modelFilename_ = modelFilename;

	SubscribeToEvent(E_RESPAWNSCENENODE, HANDLER(ModelPlayer, HandleRespawnSceneNode));
}

void ModelPlayer::RemoveModelNode()
{
	if (modelNode_ != NULL)
	{
		modelNode_->RemoveAllChildren();
		modelNode_->RemoveAllComponents();
		modelNode_->Remove();
	}
}

void ModelPlayer::HandleGetCamera(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientCamera::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetClientCamera::P_NODE] = clientNode;
		vm[SetClientCamera::P_CAMERANODE] = modelNode_->GetChild("camera", true);
		SendEvent(E_SETCLIENTCAMERA, vm);
	}
}

void ModelPlayer::HandleGetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetClientModelNode::P_NODE] = clientNode;
		vm[SetClientModelNode::P_MODELNODE] = modelNode_;
		SendEvent(E_SETCLIENTMODELNODE, vm);
	}
}

void ModelPlayer::HandleRespawnSceneNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[RespawnSceneNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		Vector3 victoria = eventData[RespawnSceneNode::P_POSITION].GetVector3();
		Quaternion quarterOnion = eventData[RespawnSceneNode::P_ROTATION].GetQuaternion();

		modelNode_->SetPosition(victoria);
		modelNode_->SetRotation(quarterOnion);
	}
}

void ModelPlayer::HandleAnimateSceneNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[AnimateSceneNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		String ani = eventData[AnimateSceneNode::P_ANIMATION].GetString();
		bool loop = eventData[AnimateSceneNode::P_LOOP].GetBool();
		unsigned char layer = (unsigned char)(eventData[AnimateSceneNode::P_LAYER].GetUInt());
		RecursiveSetAnimation(modelNode_, ani, loop, layer);
	}
}

void ModelPlayer::HandleGetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* modelNode = (Node*)(eventData[GetSceneNodeByModelNode::P_NODE].GetPtr());

	if (modelNode == modelNode_)
	{
		VariantMap vm;
		vm[SetSceneNodeByModelNode::P_MODELNODE] = modelNode;
		vm[SetSceneNodeByModelNode::P_SCENENODE] = node_;
		SendEvent(E_SETSCENENODEBYMODELNODE, vm);
	}
}

void ModelPlayer::HandleGetModelNodeBySceneNode(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[GetModelNodeBySceneNode::P_NODE].GetPtr());

	if (sceneNode == node_)
	{
		VariantMap vm;
		vm[SetModelNodeBySceneNode::P_SCENENODE] = sceneNode;
		vm[SetModelNodeBySceneNode::P_MODELNODE] = modelNode_;
		SendEvent(E_SETMODELNODEBYSCENENODE, vm);
	}
}
