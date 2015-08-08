/*
 * SpriteSheetPlayer.cpp
 *
 *  Created on: Aug 8, 2015
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
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/IO/Log.h>

#include "SpriteSheetPlayer.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

SpriteSheetPlayer::SpriteSheetPlayer(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	spriteIDCount_ = 0;
}

SpriteSheetPlayer::~SpriteSheetPlayer()
{
	RemoveModelNode();
}

void SpriteSheetPlayer::Start()
{
	modelNode_ = NULL;
	scene_ = node_->GetScene();
	LoadDefaultPlayer();
	//RecursiveSetAnimation(modelNode_, "idle1", true, 0);

	SubscribeToEvent(E_GETCLIENTCAMERA, HANDLER(SpriteSheetPlayer, HandleGetCamera));
	SubscribeToEvent(E_GETCLIENTMODELNODE, HANDLER(SpriteSheetPlayer, HandleGetClientModelNode));
	SubscribeToEvent(E_ANIMATESCENENODE, HANDLER(SpriteSheetPlayer, HandleAnimateSceneNode));
	SubscribeToEvent(E_GETSCENENODEBYMODELNODE, HANDLER(SpriteSheetPlayer, HandleGetSceneNodeByModelNode));
	SubscribeToEvent(E_GETMODELNODEBYSCENENODE, HANDLER(SpriteSheetPlayer, HandleGetModelNodeBySceneNode));
}

void SpriteSheetPlayer::RecursiveSetAnimation(Node* noed, String ani, bool loop, unsigned char layer)//todo check if animation exists, if not set default.
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

void SpriteSheetPlayer::LoadDefaultPlayer()
{
	LoadPlayer("");
}

void SpriteSheetPlayer::LoadPlayer(String modelFilename)
{
	RemoveModelNode();

	modelNode_ = scene_->CreateChild("", LOCAL, 0);

	Node* camera = modelNode_->CreateChild("camera");
	Camera* cam = camera->CreateComponent<Camera>();
	camera->SetPosition(Vector3(0.0f, 5.0f, -5.0f));

    Node* cleric = modelNode_->CreateChild("cleric");

    StaticSprite2D* clericSprite = cleric->CreateComponent<StaticSprite2D>();

    //clericSprite->SetCustomMaterial(main_->cache_->GetResource<Material>("Materials/clericMat.xml"));

    //RecursiveSetAnimation(modelNode_, "idle1", true, 0);

    modelNode_->SetPosition(main_->mySceneNode_->GetPosition());
    modelNode_->SetRotation(main_->mySceneNode_->GetRotation());

    //todo free sprites_
    AnimatedSpriteSheet* animatedSpriteSheet = new AnimatedSpriteSheet();
    animatedSpriteSheet->sheet_ = main_->cache_->GetResource<SpriteSheet2D>("Urho2D/cleric/clericSheet.xml");
    animatedSpriteSheet->staticSprite_ = clericSprite;
    animatedSpriteSheet->playing_ = false;
    animatedSpriteSheet->spriteID_ = spriteIDCount_;

    sprites_.Push(animatedSpriteSheet);

    spriteIDCount_++;

    Vector<String> files;

    main_->filesystem_->ScanDir(files,
    		main_->filesystem_->GetProgramDir() + "Data/Urho2D/cleric/animations/",
    		"*.xml", SCAN_FILES, false);

    for (int x = 0; x < files.Size(); x++)
    {
    	XMLElement ani = main_->cache_->GetResource<XMLFile>("Urho2D/cleric/animations/" + files[x])->GetRoot();

    	SpriteSheetAnimation* spriteSheetAni = new SpriteSheetAnimation();
    	animations_.Push(spriteSheetAni);

    	spriteSheetAni->name_ = ani.GetChild("Name").GetAttribute("name");
    	spriteSheetAni->loop_ = ani.GetChild("Loop").GetBool("loop");

    	int frameCount = ani.GetChild("FrameCount").GetInt("frameCount");

    	for (int x = 0; x < frameCount; x++)
    	{
    		SpriteSheetAnimationFrame* frame = new SpriteSheetAnimationFrame();
    		spriteSheetAni->frames_.Push(frame);

    		String child = "Frame" + String(x);

    		frame->duration_ = ani.GetChild(child).GetFloat("duration");
    		frame->sprite_ = ani.GetChild(child).GetAttribute("sprite");
    	}
    }

	SubscribeToEvent(E_RESPAWNSCENENODE, HANDLER(SpriteSheetPlayer, HandleRespawnSceneNode));
	SubscribeToEvent(E_ANIMATESPRITESHEET, HANDLER(SpriteSheetPlayer, HandleAnimateSpriteSheet));
	SetUpdateEventMask(USE_UPDATE);
	SubscribeToEvent(E_UPDATE, HANDLER(SpriteSheetPlayer, HandleUpdate));

	VariantMap vm;
	vm[AnimateSpriteSheet::P_NODE] = node_;
	vm[AnimateSpriteSheet::P_SPRITEID] = animatedSpriteSheet->spriteID_;
	vm[AnimateSpriteSheet::P_ANIMATION] = "idleF";
	SendEvent(E_ANIMATESPRITESHEET, vm);
}

void SpriteSheetPlayer::RemoveModelNode()
{
	if (modelNode_ != NULL)
	{
		modelNode_->RemoveAllChildren();
		modelNode_->RemoveAllComponents();
		modelNode_->Remove();
	}
}

void SpriteSheetPlayer::HandleGetCamera(StringHash eventType, VariantMap& eventData)
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

void SpriteSheetPlayer::HandleGetClientModelNode(StringHash eventType, VariantMap& eventData)
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

void SpriteSheetPlayer::HandleRespawnSceneNode(StringHash eventType, VariantMap& eventData)
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

void SpriteSheetPlayer::HandleAnimateSceneNode(StringHash eventType, VariantMap& eventData)
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

void SpriteSheetPlayer::HandleGetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData)
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

void SpriteSheetPlayer::HandleGetModelNodeBySceneNode(StringHash eventType, VariantMap& eventData)
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

void SpriteSheetPlayer::HandleAnimateSpriteSheet(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[GetModelNodeBySceneNode::P_NODE].GetPtr());

	if (sceneNode == node_)
	{
		int spriteID = eventData[AnimateSpriteSheet::P_SPRITEID].GetInt();
		String ani = eventData[AnimateSpriteSheet::P_ANIMATION].GetString();

		for (int x = 0; x < sprites_.Size(); x++)
		{
			if (sprites_[x]->spriteID_ == spriteID)
			{
				for (int y = 0; y < animations_.Size(); y++)
				{
					if (animations_[y]->name_ == ani)
					{
						sprites_[x]->animation_ = animations_[y];
						sprites_[x]->elapsedTime_ = 0.0f;
						sprites_[x]->curFrame_ = 0;
						sprites_[x]->staticSprite_->SetSprite(
								sprites_[x]->sheet_->GetSprite(
										animations_[y]->frames_[0]->sprite_));
						sprites_[x]->playing_ = true;
						break;
					}
				}
			}
		}
	}
}

void SpriteSheetPlayer::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	for (int x = 0; x < sprites_.Size(); x++)
	{
		if (sprites_[x]->playing_)
		{
			sprites_[x]->elapsedTime_ += timeStep;

			if (sprites_[x]->elapsedTime_ >=
					sprites_[x]->animation_->frames_[sprites_[x]->curFrame_]->duration_)
			{
				sprites_[x]->curFrame_++;

				if (sprites_[x]->curFrame_ >= sprites_[x]->animation_->frames_.Size())
				{
					sprites_[x]->curFrame_ = 0;
				}

				sprites_[x]->staticSprite_->SetSprite(
						sprites_[x]->sheet_->GetSprite(
								sprites_[x]->animation_->frames_[sprites_[x]->curFrame_]->sprite_));

				sprites_[x]->elapsedTime_ = 0.0f;

				if (!sprites_[x]->animation_->loop_)
				{
					sprites_[x]->playing_ = false;
				}
			}
		}
	}
}
