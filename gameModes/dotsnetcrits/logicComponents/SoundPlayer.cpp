/*
 * SoundPlayer.cpp
 *
 *  Created on: Jul 25, 2015
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
#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundListener.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource3D.h>

#include "SoundPlayer.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

#include "LC.h"

SoundPlayer::SoundPlayer(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	lc_ = new LC(context, main, main->network_);

	castSounds_.Push("Sounds/witch3/cast/goblin-1.ogg");
	castSounds_.Push("Sounds/witch3/cast/goblin-5.ogg");
	castSounds_.Push("Sounds/witch3/cast/goblin-7.ogg");
	castSounds_.Push("Sounds/witch3/cast/goblin-10.ogg");
	castSounds_.Push("Sounds/witch3/cast/goblin-13.ogg");

	meleeSounds_.Push("Sounds/witch3/melee/goblin-2.ogg");
	meleeSounds_.Push("Sounds/witch3/melee/goblin-6.ogg");
	meleeSounds_.Push("Sounds/witch3/melee/goblin-8.ogg");
	meleeSounds_.Push("Sounds/witch3/melee/goblin-11.ogg");
	meleeSounds_.Push("Sounds/witch3/melee/goblin-14.ogg");

	hurtSounds_.Push("Sounds/witch3/hurt/goblin-3.ogg");
	hurtSounds_.Push("Sounds/witch3/hurt/goblin-4.ogg");
	hurtSounds_.Push("Sounds/witch3/hurt/goblin-9.ogg");
	hurtSounds_.Push("Sounds/witch3/hurt/goblin-12.ogg");
	hurtSounds_.Push("Sounds/witch3/hurt/goblin-15.ogg");
}

SoundPlayer::~SoundPlayer()
{
	delete lc_;
}

void SoundPlayer::Start()
{
	lc_->Start(node_);

	if (lc_->main_->engine_->IsHeadless())
	{
		return;
	}

	SubscribeToEvent(E_LCMSG, HANDLER(SoundPlayer, HandleLCMSG));
	SubscribeToEvent(E_GETLC, HANDLER(SoundPlayer, HandleGetLc));

	if (lc_->main_->IsLocalClient(node_))
	{
		lc_->main_->audio_->SetListener(lc_->modelNode_->GetChild("soundListener")->GetComponent<SoundListener>());
	}

	SubscribeToEvent(E_SOUNDREQUEST, HANDLER(SoundPlayer, HandleSoundRequest));
}

void SoundPlayer::HandleSoundRequest(StringHash eventType, VariantMap& eventData)
{
	SharedPtr<Node> node = SharedPtr<Node>(static_cast<Node*>(eventData[SoundRequest::P_NODE].GetPtr()));

	if (node == node_)
	{
		int soundType = static_cast<int>( eventData[SoundRequest::P_SOUNDTYPE].GetInt() );

		if (soundType == SOUNDTYPE_CAST)
		{
			lc_->modelNode_->GetComponent<SoundSource3D>()->Play(lc_->main_->cache_->GetResource<Sound>( castSounds_[ Random( 0,castSounds_.Size() ) ] ) , 0, 0.05f);
		}
		else if (soundType == SOUNDTYPE_MELEE)
		{
			lc_->modelNode_->GetComponent<SoundSource3D>()->Play(lc_->main_->cache_->GetResource<Sound>( meleeSounds_[ Random( 0,meleeSounds_.Size() ) ] )  , 0, 0.05f);
		}
		else if (soundType == SOUNDTYPE_HURT)
		{
			lc_->modelNode_->GetComponent<SoundSource3D>()->Play(lc_->main_->cache_->GetResource<Sound>( hurtSounds_[ Random( 0,hurtSounds_.Size() ) ] )  , 0, 0.05f);
		}
	}
}

void SoundPlayer::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "SoundPlayer")
	{
		if (lc_->clientID_ == clientID)
		{
			int clientID = msg.ReadInt();
			float timeRamp = msg.ReadFloat();

			lc_->GetLagTime(lc_->conn_);

			timeRamp += lc_->lagTime_;

			//exec

			if (lc_->isServer_)
			{
				lc_->msg_.Clear();
				lc_->msg_.WriteInt(lc_->clientID_);
				lc_->msg_.WriteString("SoundPlayer");
				lc_->msg_.WriteInt(clientID);
				lc_->msg_.WriteFloat(timeRamp);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = lc_->conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = lc_->msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}

void SoundPlayer::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (sceneNode == node_)
	{
		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		lc_->GetLagTime(conn);

		//send
	}
}
