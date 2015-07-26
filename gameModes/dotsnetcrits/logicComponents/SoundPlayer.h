/*
 * SoundPlayer.h
 *
 *  Created on: Jul 25, 2015
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

#include "LC.h"

using namespace Urho3D;

class SoundPlayer : public LogicComponent
{
	OBJECT(SoundPlayer);
public:
	SoundPlayer(Context* context, Urho3DPlayer* main);
	~SoundPlayer();
	virtual void Start();
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void HandleSoundRequest(StringHash eventType, VariantMap& eventData);

	LC* lc_;

	Vector<String> castSounds_;
	Vector<String> meleeSounds_;
	Vector<String> hurtSounds_;
};
