/*
 * Melee.cpp
 *
 *  Created on: Jul 21, 2015
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

#include "Melee.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"
#include "TimedRemove.h"

Melee::Melee(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	clientExecuting_ = false;
	cooldown_ = 1.0f;
	network_ = GetSubsystem<Network>();
	modelNode_ = NULL;
	clientID_ = -1;
	isServer_ = false;
	damage_ = 10;
}

Melee::~Melee()
{
}

void Melee::Start()
{
	scene_ = node_->GetScene();

	particleStartNode_ = scene_->CreateChild(0,LOCAL);
	emitterStartFX_ = particleStartNode_->CreateComponent<ParticleEmitter>(LOCAL);
	emitterStartFX_->SetEffect(main_->cache_->GetResource<ParticleEffect>("Particle/sweat.xml"));
	particleStartNode_->SetWorldScale(Vector3::ONE);
	emitterStartFX_->SetEmitting(false);
	emitterStartFX_->SetViewMask(1);

	SubscribeToEvent(E_SETISSERVER, HANDLER(Melee, HandleSetIsServer));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);

	SubscribeToEvent(E_SETCLIENTMODELNODE, HANDLER(Melee, HandleSetClientModelNode));

	VariantMap vm;
	vm[GetClientModelNode::P_NODE] = node_;
	SendEvent(E_GETCLIENTMODELNODE, vm);

	UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
	SubscribeToEvent(E_SETCLIENTID, HANDLER(Melee, HandleSetClientID));

	VariantMap vm1;
	vm1[GetClientID::P_NODE] = main_->GetRootNode(node_);
	SendEvent(E_GETCLIENTID, vm1);

	UnsubscribeFromEvent(E_SETCLIENTID);
	SubscribeToEvent(E_SETCONNECTION, HANDLER(Melee, HandleSetConnection));

	VariantMap vm2;
	vm2[GetConnection::P_NODE] = main_->GetRootNode(node_);
	SendEvent(E_GETCONNECTION, vm2);

	UnsubscribeFromEvent(E_SETCONNECTION);

	SubscribeToEvent(E_LCMSG, HANDLER(Melee, HandleLCMSG));

	if (main_->IsLocalClient(node_))
	{
		SubscribeToEvent(E_MECHANICREQUEST, HANDLER(Melee, HandleMechanicRequest));
	}
}

void Melee::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();
}

void Melee::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		modelNode_ = (Node*)(eventData[SetClientModelNode::P_MODELNODE].GetPtr());

		beeBox_ = modelNode_->GetComponent<AnimatedModel>()->GetWorldBoundingBox();
		radius_ = modelNode_->GetComponent<CollisionShape>()->GetSize().x_;
		radius_ *= modelNode_->GetWorldScale().x_;
	}
}

void Melee::HandleSetClientID(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientID::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		clientID_ = eventData[SetClientID::P_CLIENTID].GetInt();
	}
}

void Melee::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());
	}
}

void Melee::HandleMechanicRequest(StringHash eventType, VariantMap& eventData)
{
	String mechanicID = eventData[MechanicRequest::P_MECHANICID].GetString();

	if (mechanicID == "Melee")
	{
		SubscribeToEvent(E_SETCLIENTBLIND, HANDLER(Melee, HandleSetBlind));

		if (!clientExecuting_)
		{
			VariantMap vm;
			vm[GetClientBlind::P_NODE] = node_;
			SendEvent(E_GETCLIENTBLIND, vm);

			if (!blind_)
			{
				clientExecuting_ = true;
				elapsedTime_ = 0.0f;

				StartMelee(modelNode_->GetPosition(), true);

				SubscribeToEvent(E_UPDATE, HANDLER(Melee, HandleUpdate));
			}
		}
	}
}

void Melee::HandleSetBlind(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientSilence::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		blind_ = eventData[SetClientBlind::P_BLIND].GetBool();

		UnsubscribeFromEvent(E_GETCLIENTBLIND);
	}
}

void Melee::StartMelee(Vector3 pos, bool sendToServer)
{
	VariantMap vm;
	vm[AnimateSceneNode::P_NODE] = node_;
	vm[AnimateSceneNode::P_ANIMATION] = "attack";
	vm[AnimateSceneNode::P_LOOP] = false;
	vm[AnimateSceneNode::P_LAYER] = 1;
	SendEvent(E_ANIMATESCENENODE, vm);

	victoria_ = modelNode_->GetPosition();
	victoria_.y_ += beeBox_.Size().y_;
	particleStartNode_->SetPosition(victoria_);
	emitterStartFX_->SetEmitting(true);

	scene_->GetComponent<PhysicsWorld>()->GetRigidBodies(rigidBodies_, Sphere(pos, radius_), 2);

	if (rigidBodies_.Size())
	{
		for (int x = 0; x < rigidBodies_.Size(); x++)
		{
			Node* noed = rigidBodies_[x]->GetNode();
			if (noed != modelNode_)
			{
				if (!isServer_)
				{
					victoria_ = noed->GetPosition();
					BoundingBox beeBox = noed->GetComponent<AnimatedModel>()->GetWorldBoundingBox();
					victoria_.y_ += beeBox.Size().y_;

					SharedPtr<Node> particleEndNode = SharedPtr<Node>(scene_->CreateChild(0,LOCAL));
					particleEndNode->SetPosition(victoria_);
					ParticleEmitter* emitterEndFX = particleEndNode->CreateComponent<ParticleEmitter>(LOCAL);
					emitterEndFX->SetEffect(main_->cache_->GetResource<ParticleEffect>("Particle/blood.xml"));
					particleEndNode->SetWorldScale(Vector3::ONE);
					emitterEndFX->SetEmitting(true);
					emitterEndFX->SetViewMask(1);

					particleEndNode->AddComponent(new TimedRemove(context_, main_, 3.0f), 0, LOCAL);
				}
				else
				{
					targetModelNode_ = noed;
					SubscribeToEvent(E_SETSCENENODEBYMODELNODE, HANDLER(Melee, HandleSetSceneNodeByModelNode));

					VariantMap vm0;
					vm0[GetSceneNodeByModelNode::P_NODE] = targetModelNode_;
					SendEvent(E_GETSCENENODEBYMODELNODE, vm0);

					UnsubscribeFromEvent(E_SETSCENENODEBYMODELNODE);

					VariantMap vm1;
					vm1[ModifyClientHealth::P_NODE] = targetSceneNode_;
					vm1[ModifyClientHealth::P_HEALTH] = damage_;
					vm1[ModifyClientHealth::P_OPERATION] = -1;
					vm1[ModifyClientHealth::P_SENDTOSERVER] = sendToServer;
					SendEvent(E_MODIFYCLIENTHEALTH, vm1);
				}
			}
		}
	}

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Melee");
		msg_.WriteVector3(pos);
		network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}

void Melee::HandleSetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* modelNode = (Node*)(eventData[SetSceneNodeByModelNode::P_MODELNODE].GetPtr());

	if (modelNode == targetModelNode_)
	{
		targetSceneNode_ = (Node*)(eventData[SetSceneNodeByModelNode::P_SCENENODE].GetPtr());
	}
}

void Melee::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	elapsedTime_ += timeStep;
	if (elapsedTime_ >= cooldown_)
	{
		clientExecuting_ = false;
	}
}

void Melee::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "Melee")
	{
		if (clientID_ == clientID)
		{
			Vector3 pos = msg.ReadVector3();

			StartMelee(pos, false);

			if (isServer_)
			{
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("Melee");
				msg_.WriteVector3(pos);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}
