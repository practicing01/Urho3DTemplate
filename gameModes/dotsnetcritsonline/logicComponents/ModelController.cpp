/*
 * ModelController.cpp
 *
 *  Created on: Oct 12, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
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
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>

#include "ModelController.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

#include "Speed.h"

ModelController::ModelController(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
}

ModelController::~ModelController()
{
	if (modelNode_ != NULL)
	{
		modelNode_->RemoveAllChildren();
		modelNode_->RemoveAllComponents();
		modelNode_->Remove();
	}
}

void ModelController::Start()
{
	modelNode_ = NULL;
	LoadDefaultPlayer();
	//SubscribeToEvent(E_UPDATE, HANDLER(ModelController, HandleUpdate));
	SubscribeToEvent(E_ANIMATESCENENODE, HANDLER(ModelController, HandleAnimateSceneNode));
}

void ModelController::LoadDefaultPlayer()
{
	LoadPlayer(main_->filesystem_->GetProgramDir()
			+ "Data/Objects/urhoJill.xml");//todo endless loop possibilities :d
}

void ModelController::RemoveModelNode()
{
	if (modelNode_ != NULL)
	{
		modelNode_->RemoveAllChildren();
		modelNode_->RemoveAllComponents();
		modelNode_->Remove();
	}
}

void ModelController::LoadPlayer(String modelPath)
{
	if (!main_->cache_->Exists(modelPath))
	{
		LoadDefaultPlayer();
		return;
	}

	RemoveModelNode();
    XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>(modelPath);
    modelNode_ = node_->GetScene()->InstantiateXML(xmlFile->GetRoot(),
    		node_->GetPosition(), node_->GetRotation(), LOCAL);

    node_->AddChild(modelNode_);
    modelNode_->SetName("modelNode");

    modelPath_ = modelPath;

    String fileName = xmlFile->GetName();

    fileName = fileName.Substring(fileName.FindLast("/") + 1);

    fileName = fileName.Substring(0, fileName.Find("."));

    modelNode_->SetVar("fileName",fileName);
}

void ModelController::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
}

void ModelController::HandleAnimateSceneNode(StringHash eventType, VariantMap& eventData)
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

void ModelController::RecursiveSetAnimation(Node* noed, String ani, bool loop, unsigned char layer)//todo check if animation exists, if not set default.
{
	if (noed->HasComponent<AnimationController>())
	{
		String fileName = noed->GetVar("fileName").GetString();

		String aniPath = "Models/" + fileName + "/" + ani + ".ani";

		if (main_->cache_->Exists(aniPath))
		{
			if (( (noed->GetComponent<AnimationController>()->IsPlaying(aniPath) == false)
					&& (noed->GetComponent<AnimationController>()->IsFadingIn(aniPath) == false) )
					|| (noed->GetComponent<AnimationController>()->IsFadingOut(aniPath) == true)
					|| (noed->GetComponent<AnimationController>()->IsAtEnd(aniPath) == true) )
			//if (noed->GetComponent<AnimationController>()->IsPlaying(aniPath) == false)
			{
				//LOGERRORF("playing %s @ %d",aniPath.CString(), layer);
				//noed->GetComponent<AnimationController>()->FadeOthers(aniPath, 0.0f, 0.0f);
				//noed->GetComponent<AnimationController>()->StopAll(0.0f);
				//noed->GetComponent<AnimationController>()->StopLayer(layer, 0.0f);
				//noed->GetComponent<AnimationController>()->Play(aniPath, layer, loop, 0.0f);
				noed->GetComponent<AnimationController>()->StopLayer(0, 0.0f);
				noed->GetComponent<AnimationController>()->PlayExclusive(aniPath, layer, loop, 0.0f);
				//noed->GetComponent<AnimationController>()->FadeOthers(aniPath, 0.0f, 0.25f);
				//noed->GetComponent<AnimationController>()->SetStartBone(aniPath, "Bone");
				//noed->GetComponent<AnimationController>()->SetRemoveOnCompletion(aniPath, true);
				//noed->GetComponent<AnimationController>()->SetTime(aniPath, 0.0f);
				//noed->GetComponent<AnimationController>()->SetWeight(aniPath, 1.0f);
				//noed->GetComponent<AnimationController>()->Update(0.0f);

				float speed = node_->GetComponent<Speed>()->speed_;
				noed->GetComponent<AnimationController>()->SetSpeed(aniPath, (speed * 0.35f) *
						(noed->GetComponent<AnimationController>()->GetSpeed(aniPath)));

				if (!loop)
				{
					noed->GetComponent<AnimationController>()->SetAutoFade(aniPath, 0.25f);
				}
			}
			else
			{
				return;
			}
		}
	}

	for (int x = 0; x < noed->GetNumChildren(); x++)
	{
		RecursiveSetAnimation(noed->GetChild(x), ani, loop, layer);
	}
}
