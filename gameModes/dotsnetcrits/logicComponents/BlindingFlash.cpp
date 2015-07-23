/*
 * BlindingFlash.cpp
 *
 *  Created on: Jul 23, 2015
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
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource3D.h>

#include "BlindingFlash.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

BlindingFlash::BlindingFlash(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	network_ = GetSubsystem<Network>();
	modelNode_ = NULL;
	cameraNode_ = NULL;
	clientID_ = -1;
	isServer_ = false;
	clientExecuting_ = false;
	cooldown_ = 10.0f;
	BlindingFlashed_ = false;
	BlindingFlashDuration_ = 5.0f;
	SetUpdateEventMask(USE_UPDATE);
	//SubscribeToEvent(E_CLEANSESTATUS, HANDLER(BlindingFlash, HandleCleanseStatus));
}

BlindingFlash::~BlindingFlash()
{
	if (clientExecuting_)
	{
		VariantMap vm;
		SendEvent(E_TOUCHUNSUBSCRIBE, vm);
	}
}

void BlindingFlash::Start()
{
	scene_ = node_->GetScene();

	particleEndNode_ = scene_->CreateChild(0,LOCAL);
	emitterEndFX_ = particleEndNode_->CreateComponent<ParticleEmitter>(LOCAL);
	emitterEndFX_->SetEffect(main_->cache_->GetResource<ParticleEffect>("Particle/blind.xml"));
	particleEndNode_->SetWorldScale(Vector3::ONE * 500.0f);
	emitterEndFX_->SetEmitting(false);
	emitterEndFX_->SetViewMask(1);

	particleEndNode_->CreateComponent<SoundSource3D>(LOCAL);

	SubscribeToEvent(E_SETISSERVER, HANDLER(BlindingFlash, HandleSetIsServer));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);

	SubscribeToEvent(E_SETCLIENTCAMERA, HANDLER(BlindingFlash, HandleSetCamera));

	VariantMap vm;
	vm[GetClientCamera::P_NODE] = node_;
	SendEvent(E_GETCLIENTCAMERA, vm);
}

void BlindingFlash::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();
}

void BlindingFlash::HandleSetCamera(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientCamera::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		cameraNode_ = (Node*)(eventData[SetClientCamera::P_CAMERANODE].GetPtr());

		UnsubscribeFromEvent(E_SETCLIENTCAMERA);
		SubscribeToEvent(E_SETCLIENTMODELNODE, HANDLER(BlindingFlash, HandleSetClientModelNode));

		VariantMap vm;
		vm[GetClientModelNode::P_NODE] = node_;
		SendEvent(E_GETCLIENTMODELNODE, vm);
	}
}

void BlindingFlash::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		modelNode_ = (Node*)(eventData[SetClientModelNode::P_MODELNODE].GetPtr());

		beeBox_ = modelNode_->GetComponent<AnimatedModel>()->GetWorldBoundingBox();

		UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
		SubscribeToEvent(E_SETCLIENTID, HANDLER(BlindingFlash, HandleSetClientID));

		VariantMap vm;
		vm[GetClientID::P_NODE] = main_->GetRootNode(node_);
		SendEvent(E_GETCLIENTID, vm);
	}
}

void BlindingFlash::HandleSetClientID(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientID::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		clientID_ = eventData[SetClientID::P_CLIENTID].GetInt();

		UnsubscribeFromEvent(E_SETCLIENTID);
		SubscribeToEvent(E_SETCONNECTION, HANDLER(BlindingFlash, HandleSetConnection));

		VariantMap vm;
		vm[GetConnection::P_NODE] = main_->GetRootNode(node_);
		SendEvent(E_GETCONNECTION, vm);

	}
}

void BlindingFlash::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());

		UnsubscribeFromEvent(E_SETCONNECTION);

		SubscribeToEvent(E_LCMSG, HANDLER(BlindingFlash, HandleLCMSG));
		SubscribeToEvent(E_GETLC, HANDLER(BlindingFlash, HandleGetLc));

		if (main_->IsLocalClient(node_))
		{
			SubscribeToEvent(E_MECHANICREQUEST, HANDLER(BlindingFlash, HandleMechanicRequest));
		}
	}
}

void BlindingFlash::HandleMechanicRequest(StringHash eventType, VariantMap& eventData)
{
	String mechanicID = eventData[MechanicRequest::P_MECHANICID].GetString();

	if (mechanicID == "Blind")
	{
		SubscribeToEvent(E_SETCLIENTSILENCE, HANDLER(BlindingFlash, HandleSetSilence));

		VariantMap vm;
		vm[GetClientSilence::P_NODE] = node_;
		SendEvent(E_GETCLIENTSILENCE, vm);
	}
}

void BlindingFlash::HandleSetSilence(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientSilence::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		silence_ = eventData[SetClientSilence::P_SILENCE].GetBool();

		UnsubscribeFromEvent(E_SETCLIENTSILENCE);

		if (!clientExecuting_ && !silence_)
		{
			clientExecuting_ = true;
			elapsedTime_ = 0.0f;

			VariantMap vm;
			SendEvent(E_TOUCHSUBSCRIBE, vm);

			SubscribeToEvent(E_TOUCHEND, HANDLER(BlindingFlash, HandleTouchEnd));
		}
	}
}

void BlindingFlash::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
{
	if (main_->ui_->GetFocusElement())
	{
		return;
	}

	VariantMap vm;
	SendEvent(E_TOUCHUNSUBSCRIBE, vm);

	UnsubscribeFromEvent(E_TOUCHEND);

	using namespace TouchEnd;

	Ray cameraRay = cameraNode_->GetComponent<Camera>()->GetScreenRay(
			(float) eventData[P_X].GetInt() / main_->graphics_->GetWidth(),
			(float) eventData[P_Y].GetInt() / main_->graphics_->GetHeight());

	PhysicsRaycastResult raeResult_;

	scene_->GetComponent<PhysicsWorld>()->RaycastSingle(raeResult_, cameraRay, 1000.0f, 2);//todo define masks.

	targetClientID_ = -1;
	targetModelNode_ = NULL;
	targetSceneNode_ = NULL;

	if (raeResult_.body_)
	{
		targetModelNode_ = raeResult_.body_->GetNode();

		SubscribeToEvent(E_SETSCENENODEBYMODELNODE, HANDLER(BlindingFlash, HandleSetSceneNodeByModelNode));

		VariantMap vm;
		vm[GetSceneNodeByModelNode::P_NODE] = targetModelNode_;
		SendEvent(E_GETSCENENODEBYMODELNODE, vm);

		UnsubscribeFromEvent(E_SETSCENENODEBYMODELNODE);
		SubscribeToEvent(E_SETSCENENODECLIENTID, HANDLER(BlindingFlash, HandleSetSceneNodeClientID));

		VariantMap vm0;
		vm0[GetSceneNodeClientID::P_NODE] = targetSceneNode_;
		SendEvent(E_GETSCENENODECLIENTID, vm0);

		UnsubscribeFromEvent(E_SETSCENENODECLIENTID);

		StartBlindingFlash(targetClientID_, 0.0f, true);
	}
	else
	{
		clientExecuting_ = false;
		return;
	}
}

void BlindingFlash::HandleSetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* modelNode = (Node*)(eventData[SetSceneNodeByModelNode::P_MODELNODE].GetPtr());

	if (modelNode == targetModelNode_)
	{
		targetSceneNode_ = (Node*)(eventData[SetSceneNodeByModelNode::P_SCENENODE].GetPtr());
	}
}

void BlindingFlash::HandleSetSceneNodeClientID(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetSceneNodeClientID::P_NODE].GetPtr());

	if (sceneNode == targetSceneNode_)
	{
		targetClientID_ = eventData[SetSceneNodeClientID::P_CLIENTID].GetInt();
	}
}

void BlindingFlash::StartBlindingFlash(int clientID, float timeRamp, bool sendToServer)
{
	targetSceneNode_ = main_->GetSceneNode(clientID);

	SubscribeToEvent(E_SETMODELNODEBYSCENENODE, HANDLER(BlindingFlash, HandleSetModelNodeBySceneNode));

	VariantMap vm0;
	vm0[GetModelNodeBySceneNode::P_NODE] = targetSceneNode_;
	SendEvent(E_GETMODELNODEBYSCENENODE, vm0);

	if (!isServer_)
	{
		targetModelNode_->AddChild(particleEndNode_);
		victoria_ = targetModelNode_->GetPosition();
		victoria_.y_ += beeBox_.Size().y_;
		particleEndNode_->SetWorldPosition(victoria_);
		emitterEndFX_->SetEmitting(true);
		//particleEndNode_->GetComponent<SoundSource3D>()->Play(main_->cache_->GetResource<Sound>("Sounds/BlindingFlash/BlindingFlash.ogg"));

	}

	BlindingFlashed_ = true;
	BlindingFlashElapsedTime_ = timeRamp;

	clientExecuting_ = true;
	elapsedTime_ = timeRamp;
	SubscribeToEvent(E_UPDATE, HANDLER(BlindingFlash, HandleUpdate));

	VariantMap vm;
	vm[AnimateSceneNode::P_NODE] = node_;
	vm[AnimateSceneNode::P_ANIMATION] = "attack";
	vm[AnimateSceneNode::P_LOOP] = false;
	vm[AnimateSceneNode::P_LAYER] = 1;
	SendEvent(E_ANIMATESCENENODE, vm);

	VariantMap vm1;
	vm1[ModifyClientBlind::P_NODE] = targetSceneNode_;
	vm1[ModifyClientBlind::P_STATE] = true;
	vm1[ModifyClientBlind::P_SENDTOSERVER] = false;
	SendEvent(E_MODIFYCLIENTBLIND, vm1);

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("BlindingFlash");
		msg_.WriteInt(targetClientID_);
		msg_.WriteFloat(timeRamp);
		network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}

void BlindingFlash::HandleSetModelNodeBySceneNode(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetModelNodeBySceneNode::P_SCENENODE].GetPtr());

	if (sceneNode == targetSceneNode_)
	{
		targetModelNode_ = (Node*)(eventData[SetModelNodeBySceneNode::P_MODELNODE].GetPtr());

		UnsubscribeFromEvent(E_SETMODELNODEBYSCENENODE);
	}
}

void BlindingFlash::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	elapsedTime_ += timeStep;
	if (elapsedTime_ >= cooldown_)
	{
		clientExecuting_ = false;
	}

	if (BlindingFlashed_)
	{
		BlindingFlashElapsedTime_ += timeStep;
		if (BlindingFlashElapsedTime_ >= BlindingFlashDuration_)
		{
			BlindingFlashed_ = false;
//todo the following will not work if the cooldown is less than the duration.
//need to store for each client in vectors
			SubscribeToEvent(E_SETSCENENODEBYMODELNODE, HANDLER(BlindingFlash, HandleSetSceneNodeByModelNode));

			VariantMap vm;
			vm[GetSceneNodeByModelNode::P_NODE] = targetModelNode_;
			SendEvent(E_GETSCENENODEBYMODELNODE, vm);

			UnsubscribeFromEvent(E_SETSCENENODEBYMODELNODE);

			VariantMap vm1;
			vm1[ModifyClientBlind::P_NODE] = targetSceneNode_;
			vm1[ModifyClientBlind::P_STATE] = false;
			vm1[ModifyClientBlind::P_SENDTOSERVER] = false;
			SendEvent(E_MODIFYCLIENTBLIND, vm1);

			if (!isServer_)
			{
				targetModelNode_->RemoveChild(particleEndNode_);
				emitterEndFX_->SetEmitting(false);
			}
		}
	}

	if (cooldown_ > BlindingFlashDuration_)
	{
		if (elapsedTime_ >= cooldown_)
		{
			UnsubscribeFromEvent(E_UPDATE);
		}
	}
	else
	{
		if (BlindingFlashElapsedTime_ >= BlindingFlashDuration_)
		{
			UnsubscribeFromEvent(E_UPDATE);
		}
	}
}

void BlindingFlash::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "BlindingFlash")
	{
		if (clientID_ == clientID)
		{
			targetClientID_ = msg.ReadInt();
			float timeRamp = msg.ReadFloat();

			SubscribeToEvent(E_SETLAGTIME, HANDLER(BlindingFlash, HandleSetLagTime));

			VariantMap vm;
			vm[GetLagTime::P_CONNECTION] = conn_;
			SendEvent(E_GETLAGTIME, vm);

			timeRamp += lagTime_;

			StartBlindingFlash(targetClientID_, timeRamp, false);

			if (isServer_)
			{
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("BlindingFlash");
				msg_.WriteInt(targetClientID_);
				msg_.WriteFloat(timeRamp);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}

void BlindingFlash::HandleSetLagTime(StringHash eventType, VariantMap& eventData)
{
	Connection* sender = (Connection*)(eventData[SetLagTime::P_CONNECTION].GetPtr());
	if (sender == conn_)
	{
		lagTime_ = eventData[SetLagTime::P_LAGTIME].GetFloat();

		UnsubscribeFromEvent(E_SETLAGTIME);
	}
}

void BlindingFlash::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		if (!BlindingFlashed_)
		{
			return;
		}

		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		SubscribeToEvent(E_SETLAGTIME, HANDLER(BlindingFlash, HandleSetLagTime));

		VariantMap vm;
		vm[GetLagTime::P_CONNECTION] = conn;
		SendEvent(E_GETLAGTIME, vm);

		float timeRamp = elapsedTime_ + lagTime_;

		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("BlindingFlash");
		msg_.WriteInt(targetClientID_);
		msg_.WriteFloat(timeRamp);
		conn->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}
