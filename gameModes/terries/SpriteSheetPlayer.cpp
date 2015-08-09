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

    modelNode_->SetPosition(main_->mySceneNode_->GetPosition());
    modelNode_->SetRotation(main_->mySceneNode_->GetRotation());

	Node* camera = modelNode_->CreateChild("camera");
	Camera* cam = camera->CreateComponent<Camera>();
	camera->SetPosition(Vector3(0.0f, 5.0f, -5.0f));

	SubscribeToEvent(E_RESPAWNSCENENODE, HANDLER(SpriteSheetPlayer, HandleRespawnSceneNode));
	SubscribeToEvent(E_ANIMATESPRITESHEET, HANDLER(SpriteSheetPlayer, HandleAnimateSpriteSheet));
	SetUpdateEventMask(USE_UPDATE);
	SubscribeToEvent(E_UPDATE, HANDLER(SpriteSheetPlayer, HandleUpdate));

	LoadSprite("cleric");
	LoadSprite("ranger");
	LoadSprite("warrior");
	LoadSprite("wizard");
}

void SpriteSheetPlayer::LoadSprite(String name)
{
	Node* spriteNode = modelNode_->CreateChild(name, LOCAL, 0);

	StaticSprite2D* sprite = spriteNode->CreateComponent<StaticSprite2D>();

	//sprite->SetCustomMaterial(main_->cache_->GetResource<Material>("Materials/" + name + "Mat.xml"));

	AnimatedSpriteSheet* animatedSpriteSheet = new AnimatedSpriteSheet();
	animatedSpriteSheet->sheet_ = main_->cache_->GetResource<SpriteSheet2D>("Urho2D/" + name + "/" + name + "Sheet.xml");
	animatedSpriteSheet->staticSprite_ = sprite;
	animatedSpriteSheet->playing_ = false;
	animatedSpriteSheet->spriteID_ = spriteIDCount_;

	sprites_.Push(animatedSpriteSheet);

	spriteIDCount_++;

	Vector<String> files;

	main_->filesystem_->ScanDir(files,
			main_->filesystem_->GetProgramDir() + "Data/Urho2D/" + name + "/animations/",
			"*.xml", SCAN_FILES, false);

	for (int x = 0; x < files.Size(); x++)
	{
		XMLElement ani = main_->cache_->GetResource<XMLFile>("Urho2D/" + name + "/animations/" + files[x])->GetRoot();

		SpriteSheetAnimation* spriteSheetAni = new SpriteSheetAnimation();
		animatedSpriteSheet->animations_.Push(spriteSheetAni);

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

	VariantMap vm;
	vm[AnimateSpriteSheet::P_NODE] = node_;
	vm[AnimateSpriteSheet::P_SPRITEID] = animatedSpriteSheet->spriteID_;
	vm[AnimateSpriteSheet::P_ANIMATION] = "idleF";
	SendEvent(E_ANIMATESPRITESHEET, vm);
}

void SpriteSheetPlayer::RemoveModelNode()
{
	for (int x = 0; x < sprites_.Size(); x++)
	{
		for (int y = 0; y < sprites_[x]->animations_.Size(); y++)
		{
			for (int z = 0; z < sprites_[x]->animations_[y]->frames_.Size(); z++)
			{
				delete sprites_[x]->animations_[y]->frames_[z];
			}

			delete sprites_[x]->animations_[y];
		}

		delete sprites_[x];
	}
	sprites_.Clear();

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
		Node* spawnNode = (Node*)(eventData[RespawnSceneNode::P_SPAWNNODE].GetPtr());

		modelNode_->SetPosition(spawnNode->GetPosition());
		modelNode_->SetRotation(spawnNode->GetRotation());

		for (int x = 0; x < modelNode_->GetNumChildren(); x++)
		{
			Node* modelChild = modelNode_->GetChild(x);
			Node* spawnChild = spawnNode->GetChild(modelChild->GetName());
			if (spawnChild)
			{
				modelChild->SetWorldPosition(spawnChild->GetWorldPosition());
				modelChild->SetWorldRotation(spawnChild->GetWorldRotation());
			}
		}
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
				for (int y = 0; y < sprites_[x]->animations_.Size(); y++)
				{
					if (sprites_[x]->animations_[y]->name_ == ani)
					{
						sprites_[x]->animation_ = sprites_[x]->animations_[y];
						sprites_[x]->elapsedTime_ = 0.0f;
						sprites_[x]->curFrame_ = 0;
						sprites_[x]->staticSprite_->SetSprite(
								sprites_[x]->sheet_->GetSprite(
										sprites_[x]->animations_[y]->frames_[0]->sprite_));
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
