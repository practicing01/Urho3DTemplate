/*
 * SpriteSheetPlayer.h
 *
 *  Created on: Aug 8, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../../../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

using namespace Urho3D;

class SpriteSheetPlayer : public LogicComponent
{
	OBJECT(SpriteSheetPlayer);
public:
	SpriteSheetPlayer(Context* context, Urho3DPlayer* main);
	~SpriteSheetPlayer();
	virtual void Start();
	void LoadDefaultPlayer();
	void LoadPlayer(String modelFilename);
	void RecursiveSetAnimation(Node* noed, String ani, bool loop, unsigned char layer);
	void RemoveModelNode();
	void HandleGetCamera(StringHash eventType, VariantMap& eventData);
	void HandleGetClientModelNode(StringHash eventType, VariantMap& eventData);
	void HandleRespawnSceneNode(StringHash eventType, VariantMap& eventData);
	void HandleAnimateSceneNode(StringHash eventType, VariantMap& eventData);
	void HandleGetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData);
	void HandleGetModelNodeBySceneNode(StringHash eventType, VariantMap& eventData);
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandleAnimateSpriteSheet(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

    SharedPtr<Node> modelNode_;
	SharedPtr<Scene> scene_;

	typedef struct
	{
		float duration_;
		String sprite_;
	}SpriteSheetAnimationFrame;

	typedef struct
	{
		String name_;
		bool loop_;
		Vector<SpriteSheetAnimationFrame*> frames_;
	}SpriteSheetAnimation;

	Vector<SpriteSheetAnimation*> animations_;

	typedef struct
	{
		float elapsedTime_;
		int curFrame_;
		bool playing_;
		SpriteSheetAnimation* animation_;
		SpriteSheet2D* sheet_;
		StaticSprite2D* staticSprite_;
		int spriteID_;
	}AnimatedSpriteSheet;

	Vector<AnimatedSpriteSheet*> sprites_;

	int spriteIDCount_;
};
