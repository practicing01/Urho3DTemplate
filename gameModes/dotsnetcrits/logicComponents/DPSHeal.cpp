/*
 * DPSHeal.cpp
 *
 *  Created on: Jul 22, 2015
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

#include "DPSHeal.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"
#include "TimedRemove.h"

DPSHeal::DPSHeal(Context* context, Urho3DPlayer* main) :
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

DPSHeal::~DPSHeal()
{
}

void DPSHeal::Start()
{
	scene_ = node_->GetScene();

	particleStartNode_ = scene_->CreateChild(0,LOCAL);
	emitterStartFX_ = particleStartNode_->CreateComponent<ParticleEmitter>(LOCAL);
	emitterStartFX_->SetEffect(main_->cache_->GetResource<ParticleEffect>("Particle/dpsheal.xml"));
	particleStartNode_->SetWorldScale(Vector3::ONE * 2.0f);
	emitterStartFX_->SetEmitting(false);
	emitterStartFX_->SetViewMask(1);

	SubscribeToEvent(E_SETISSERVER, HANDLER(DPSHeal, HandleSetIsServer));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);

	SubscribeToEvent(E_SETCLIENTMODELNODE, HANDLER(DPSHeal, HandleSetClientModelNode));

	VariantMap vm;
	vm[GetClientModelNode::P_NODE] = node_;
	SendEvent(E_GETCLIENTMODELNODE, vm);

	UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
	SubscribeToEvent(E_SETCLIENTID, HANDLER(DPSHeal, HandleSetClientID));

	VariantMap vm1;
	vm1[GetClientID::P_NODE] = main_->GetRootNode(node_);
	SendEvent(E_GETCLIENTID, vm1);

	UnsubscribeFromEvent(E_SETCLIENTID);
	SubscribeToEvent(E_SETCONNECTION, HANDLER(DPSHeal, HandleSetConnection));

	VariantMap vm2;
	vm2[GetConnection::P_NODE] = main_->GetRootNode(node_);
	SendEvent(E_GETCONNECTION, vm2);

	UnsubscribeFromEvent(E_SETCONNECTION);

	SubscribeToEvent(E_LCMSG, HANDLER(DPSHeal, HandleLCMSG));

	if (main_->IsLocalClient(node_))
	{
		SubscribeToEvent(E_MECHANICREQUEST, HANDLER(DPSHeal, HandleMechanicRequest));
	}
}

void DPSHeal::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();
}

void DPSHeal::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
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

void DPSHeal::HandleSetClientID(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientID::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		clientID_ = eventData[SetClientID::P_CLIENTID].GetInt();
	}
}

void DPSHeal::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());
	}
}

void DPSHeal::HandleMechanicRequest(StringHash eventType, VariantMap& eventData)
{
	String mechanicID = eventData[MechanicRequest::P_MECHANICID].GetString();

	if (mechanicID == "DPSHeal")
	{
		SubscribeToEvent(E_SETCLIENTSILENCE, HANDLER(DPSHeal, HandleSetSilence));

		if (!clientExecuting_)
		{
			VariantMap vm;
			vm[GetClientSilence::P_NODE] = node_;
			SendEvent(E_GETCLIENTSILENCE, vm);

			if (!silence_)
			{
				clientExecuting_ = true;
				elapsedTime_ = 0.0f;

				StartDPSHeal(modelNode_->GetPosition(), true);

				SubscribeToEvent(E_UPDATE, HANDLER(DPSHeal, HandleUpdate));
			}
		}
	}
}

void DPSHeal::HandleSetSilence(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientSilence::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		silence_ = eventData[SetClientSilence::P_SILENCE].GetBool();

		UnsubscribeFromEvent(E_GETCLIENTSILENCE);
	}
}

void DPSHeal::HandleSetArmor(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientArmor::P_NODE].GetPtr());

	if (clientNode == targetSceneNode_)
	{
		targetArmor_ = eventData[SetClientArmor::P_ARMOR].GetInt();

		UnsubscribeFromEvent(E_SETCLIENTARMOR);
	}
}

void DPSHeal::StartDPSHeal(Vector3 pos, bool sendToServer)
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
					emitterEndFX->SetEffect(main_->cache_->GetResource<ParticleEffect>("Particle/dpsheal.xml"));
					particleEndNode->SetWorldScale(Vector3::ONE);
					emitterEndFX->SetEmitting(true);
					emitterEndFX->SetViewMask(1);

					particleEndNode->AddComponent(new TimedRemove(context_, main_, 3.0f), 0, LOCAL);
				}

				targetModelNode_ = noed;
				//Get target scene node.
				SubscribeToEvent(E_SETSCENENODEBYMODELNODE, HANDLER(DPSHeal, HandleSetSceneNodeByModelNode));

				VariantMap vm0;
				vm0[GetSceneNodeByModelNode::P_NODE] = targetModelNode_;
				SendEvent(E_GETSCENENODEBYMODELNODE, vm0);

				UnsubscribeFromEvent(E_SETSCENENODEBYMODELNODE);

				//Get target armor.
				SubscribeToEvent(E_SETCLIENTARMOR, HANDLER(DPSHeal, HandleSetArmor));

				VariantMap vm2;
				vm2[GetClientArmor::P_NODE] = targetSceneNode_;
				SendEvent(E_GETCLIENTARMOR, vm2);

				int damage = damage_ + targetArmor_;

				if (damage > 0)
				{
					VariantMap vm1;
					vm1[ModifyClientHealth::P_NODE] = targetSceneNode_;
					vm1[ModifyClientHealth::P_HEALTH] = damage;
					vm1[ModifyClientHealth::P_OPERATION] = 1;
					vm1[ModifyClientHealth::P_SENDTOSERVER] = false;
					SendEvent(E_MODIFYCLIENTHEALTH, vm1);
				}
			}
		}
	}

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("DPSHeal");
		msg_.WriteVector3(pos);
		network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}

void DPSHeal::HandleSetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* modelNode = (Node*)(eventData[SetSceneNodeByModelNode::P_MODELNODE].GetPtr());

	if (modelNode == targetModelNode_)
	{
		targetSceneNode_ = (Node*)(eventData[SetSceneNodeByModelNode::P_SCENENODE].GetPtr());
	}
}

void DPSHeal::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	elapsedTime_ += timeStep;
	if (elapsedTime_ >= cooldown_)
	{
		clientExecuting_ = false;
	}
}

void DPSHeal::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "DPSHeal")
	{
		if (clientID_ == clientID)
		{
			Vector3 pos = msg.ReadVector3();

			StartDPSHeal(pos, false);

			if (isServer_)
			{
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("DPSHeal");
				msg_.WriteVector3(pos);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}
