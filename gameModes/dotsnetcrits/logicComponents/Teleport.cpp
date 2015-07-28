/*
 * Teleport.cpp
 *
 *  Created on: Jul 19, 2015
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

#include "Teleport.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

Teleport::Teleport(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	network_ = GetSubsystem<Network>();
	modelNode_ = NULL;
	cameraNode_ = NULL;
	clientID_ = -1;
	lagTime_ = 0;
	isServer_ = false;
	clientExecuting_ = false;
	cooldown_ = 5.0f;
	silence_ = false;
	SetUpdateEventMask(USE_UPDATE);
}

Teleport::~Teleport()
{
	if (clientExecuting_)
	{
		VariantMap vm;
		SendEvent(E_TOUCHUNSUBSCRIBE, vm);
	}
}

void Teleport::Start()
{
	scene_ = node_->GetScene();

	particleStartNode_ = scene_->CreateChild(0,LOCAL);
	emitterStartFX_ = particleStartNode_->CreateComponent<ParticleEmitter>(LOCAL);
	emitterStartFX_->SetEffect(main_->cache_->GetResource<ParticleEffect>("Particle/teleport.xml"));
	particleStartNode_->SetWorldScale(Vector3::ONE);
	emitterStartFX_->SetEmitting(false);
	emitterStartFX_->SetViewMask(1);

	particleEndNode_ = scene_->CreateChild(0,LOCAL);
	emitterEndFX_ = particleEndNode_->CreateComponent<ParticleEmitter>(LOCAL);
	emitterEndFX_->SetEffect(main_->cache_->GetResource<ParticleEffect>("Particle/teleport.xml"));
	particleEndNode_->SetWorldScale(Vector3::ONE);
	emitterEndFX_->SetEmitting(false);
	emitterEndFX_->SetViewMask(1);

	particleEndNode_->CreateComponent<SoundSource3D>(LOCAL);

	SubscribeToEvent(E_SETISSERVER, HANDLER(Teleport, HandleSetIsServer));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);

	SubscribeToEvent(E_SETCLIENTCAMERA, HANDLER(Teleport, HandleSetCamera));

	VariantMap vm;
	vm[GetClientCamera::P_NODE] = node_;
	SendEvent(E_GETCLIENTCAMERA, vm);
}

void Teleport::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();
}

void Teleport::HandleSetCamera(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientCamera::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		cameraNode_ = (Node*)(eventData[SetClientCamera::P_CAMERANODE].GetPtr());

		UnsubscribeFromEvent(E_SETCLIENTCAMERA);
		SubscribeToEvent(E_SETCLIENTMODELNODE, HANDLER(Teleport, HandleSetClientModelNode));

		VariantMap vm;
		vm[GetClientModelNode::P_NODE] = node_;
		SendEvent(E_GETCLIENTMODELNODE, vm);
	}
}

void Teleport::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		modelNode_ = (Node*)(eventData[SetClientModelNode::P_MODELNODE].GetPtr());

		radius_ = modelNode_->GetComponent<CollisionShape>()->GetSize().x_;
		radius_ *= modelNode_->GetWorldScale().x_;

		UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
		SubscribeToEvent(E_SETCLIENTID, HANDLER(Teleport, HandleSetClientID));

		VariantMap vm;
		vm[GetClientID::P_NODE] = main_->GetRootNode(node_);
		SendEvent(E_GETCLIENTID, vm);
	}
}

void Teleport::HandleSetClientID(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientID::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		clientID_ = eventData[SetClientID::P_CLIENTID].GetInt();

		UnsubscribeFromEvent(E_SETCLIENTID);
		SubscribeToEvent(E_SETCONNECTION, HANDLER(Teleport, HandleSetConnection));

		VariantMap vm;
		vm[GetConnection::P_NODE] = main_->GetRootNode(node_);
		SendEvent(E_GETCONNECTION, vm);

	}
}

void Teleport::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());

		UnsubscribeFromEvent(E_SETCONNECTION);

		SubscribeToEvent(E_LCMSG, HANDLER(Teleport, HandleLCMSG));

		if (main_->IsLocalClient(node_))
		{
			SubscribeToEvent(E_MECHANICREQUEST, HANDLER(Teleport, HandleMechanicRequest));
		}
	}
}

void Teleport::HandleMechanicRequest(StringHash eventType, VariantMap& eventData)
{
	String mechanicID = eventData[MechanicRequest::P_MECHANICID].GetString();

	if (mechanicID == "Teleport")
	{
		SubscribeToEvent(E_SETCLIENTSILENCE, HANDLER(Teleport, HandleSetSilence));

		VariantMap vm;
		vm[GetClientSilence::P_NODE] = node_;
		SendEvent(E_GETCLIENTSILENCE, vm);
	}
}

void Teleport::HandleSetSilence(StringHash eventType, VariantMap& eventData)
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

			SubscribeToEvent(E_TOUCHEND, HANDLER(Teleport, HandleTouchEnd));
		}
	}
}

void Teleport::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
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

	scene_->GetComponent<PhysicsWorld>()->RaycastSingle(raeResult_, cameraRay, 1000.0f, 1);//todo define masks.

	if (raeResult_.body_)
	{
		victoria_ = raeResult_.position_;
		vectoria_ -= -modelNode_->GetDirection() * (radius_ * 1.01f);
	}
	else
	{
		clientExecuting_ = false;
		return;
	}

	TeleportTo(victoria_, true);

	SubscribeToEvent(E_UPDATE, HANDLER(Teleport, HandleUpdate));
}

void Teleport::TeleportTo(Vector3 dest, bool sendToServer)
{
	particleStartNode_->SetPosition(modelNode_->GetPosition());
	emitterStartFX_->SetEmitting(true);

	modelNode_->SetPosition(dest);

	VariantMap vm;
	vm[AnimateSceneNode::P_NODE] = node_;
	vm[AnimateSceneNode::P_ANIMATION] = "attack";
	vm[AnimateSceneNode::P_LOOP] = false;
	vm[AnimateSceneNode::P_LAYER] = 1;
	SendEvent(E_ANIMATESCENENODE, vm);

	particleEndNode_->SetPosition(dest);
	emitterEndFX_->SetEmitting(true);

	particleEndNode_->GetComponent<SoundSource3D>()->Play(main_->cache_->GetResource<Sound>("Sounds/teleport/teleport.ogg"), 0.0f, 0.5f);

	vm.Clear();
	vm[SoundRequest::P_NODE] = node_;
	vm[SoundRequest::P_SOUNDTYPE] = SOUNDTYPE_CAST;
	SendEvent(E_SOUNDREQUEST,vm);

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Teleport");
		msg_.WriteVector3(dest);
		if (!isServer_)
		{
			network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg_);
		}
		else
		{
			network_->BroadcastMessage(MSG_LCMSG, true, true, msg_);
		}
	}
}

void Teleport::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	elapsedTime_ += timeStep;
	if (elapsedTime_ >= cooldown_)
	{
		clientExecuting_ = false;
		UnsubscribeFromEvent(E_UPDATE);
	}
}

void Teleport::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "Teleport")
	{
		if (clientID_ == clientID)
		{
			Vector3 dest = msg.ReadVector3();

			TeleportTo(dest, false);

			if (isServer_)
			{
				//todo asynchronous messaging
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("Teleport");
				msg_.WriteVector3(dest);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}
