/*
 * AOEHeal.cpp
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

#include "AOEHeal.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

AOEHeal::AOEHeal(Context* context, Urho3DPlayer* main) :
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
	AOEHealed_ = false;
	AOEHealDuration_ = 5.0f;
	AOEHealInterval_ = 1.0f;
	damage_ = 10;
	SetUpdateEventMask(USE_UPDATE);
	//SubscribeToEvent(E_CLEANSESTATUS, HANDLER(AOEHeal, HandleCleanseStatus));
}

AOEHeal::~AOEHeal()
{
	if (clientExecuting_)
	{
		VariantMap vm;
		SendEvent(E_TOUCHUNSUBSCRIBE, vm);
	}
}

void AOEHeal::Start()
{
	scene_ = node_->GetScene();

	particleEndNode_ = scene_->CreateChild(0,LOCAL);
	emitterEndFX_ = particleEndNode_->CreateComponent<ParticleEmitter>(LOCAL);
	emitterEndFX_->SetEffect(main_->cache_->GetResource<ParticleEffect>("Particle/aoeheal.xml"));
	particleEndNode_->SetWorldScale(Vector3::ONE * 5.0f);
	emitterEndFX_->SetEmitting(false);
	emitterEndFX_->SetViewMask(1);

	radius_ = particleEndNode_->GetWorldScale().x_ * 3.0f;

	particleEndNode_->CreateComponent<SoundSource3D>(LOCAL);

	SubscribeToEvent(E_SETISSERVER, HANDLER(AOEHeal, HandleSetIsServer));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);

	SubscribeToEvent(E_SETCLIENTCAMERA, HANDLER(AOEHeal, HandleSetCamera));

	VariantMap vm;
	vm[GetClientCamera::P_NODE] = node_;
	SendEvent(E_GETCLIENTCAMERA, vm);
}

void AOEHeal::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();
}

void AOEHeal::HandleSetCamera(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientCamera::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		cameraNode_ = (Node*)(eventData[SetClientCamera::P_CAMERANODE].GetPtr());

		UnsubscribeFromEvent(E_SETCLIENTCAMERA);
		SubscribeToEvent(E_SETCLIENTMODELNODE, HANDLER(AOEHeal, HandleSetClientModelNode));

		VariantMap vm;
		vm[GetClientModelNode::P_NODE] = node_;
		SendEvent(E_GETCLIENTMODELNODE, vm);
	}
}

void AOEHeal::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		modelNode_ = (Node*)(eventData[SetClientModelNode::P_MODELNODE].GetPtr());

		beeBox_ = modelNode_->GetComponent<AnimatedModel>()->GetWorldBoundingBox();

		UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
		SubscribeToEvent(E_SETCLIENTID, HANDLER(AOEHeal, HandleSetClientID));

		VariantMap vm;
		vm[GetClientID::P_NODE] = main_->GetRootNode(node_);
		SendEvent(E_GETCLIENTID, vm);
	}
}

void AOEHeal::HandleSetClientID(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientID::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		clientID_ = eventData[SetClientID::P_CLIENTID].GetInt();

		UnsubscribeFromEvent(E_SETCLIENTID);
		SubscribeToEvent(E_SETCONNECTION, HANDLER(AOEHeal, HandleSetConnection));

		VariantMap vm;
		vm[GetConnection::P_NODE] = main_->GetRootNode(node_);
		SendEvent(E_GETCONNECTION, vm);

	}
}

void AOEHeal::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());

		UnsubscribeFromEvent(E_SETCONNECTION);

		SubscribeToEvent(E_LCMSG, HANDLER(AOEHeal, HandleLCMSG));
		SubscribeToEvent(E_GETLC, HANDLER(AOEHeal, HandleGetLc));

		if (main_->IsLocalClient(node_))
		{
			SubscribeToEvent(E_MECHANICREQUEST, HANDLER(AOEHeal, HandleMechanicRequest));
		}
	}
}

void AOEHeal::HandleMechanicRequest(StringHash eventType, VariantMap& eventData)
{
	String mechanicID = eventData[MechanicRequest::P_MECHANICID].GetString();

	if (mechanicID == "AOEHeal")
	{
		SubscribeToEvent(E_SETCLIENTSILENCE, HANDLER(AOEHeal, HandleSetSilence));

		VariantMap vm;
		vm[GetClientSilence::P_NODE] = node_;
		SendEvent(E_GETCLIENTSILENCE, vm);
	}
}

void AOEHeal::HandleSetSilence(StringHash eventType, VariantMap& eventData)
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

			SubscribeToEvent(E_TOUCHEND, HANDLER(AOEHeal, HandleTouchEnd));
		}
	}
}

void AOEHeal::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
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
		targetPos_ = raeResult_.position_;

		StartAOEHeal(targetPos_, 0.0f, true);
	}
	else
	{
		clientExecuting_ = false;
		return;
	}
}

void AOEHeal::HandleSetSceneNodeByModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* modelNode = (Node*)(eventData[SetSceneNodeByModelNode::P_MODELNODE].GetPtr());

	if (modelNode == targetModelNode_)
	{
		targetSceneNode_ = (Node*)(eventData[SetSceneNodeByModelNode::P_SCENENODE].GetPtr());
	}
}

void AOEHeal::HandleSetSceneNodeClientID(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetSceneNodeClientID::P_NODE].GetPtr());

	if (sceneNode == targetSceneNode_)
	{
		targetClientID_ = eventData[SetSceneNodeClientID::P_CLIENTID].GetInt();
	}
}

void AOEHeal::StartAOEHeal(Vector3 targetPos, float timeRamp, bool sendToServer)
{
	if (!main_->engine_->IsHeadless())
	{
		particleEndNode_->SetWorldPosition(targetPos_);
		emitterEndFX_->SetEmitting(true);
		particleEndNode_->GetComponent<SoundSource3D>()->Play(main_->cache_->GetResource<Sound>("Sounds/aoeheal/aoeheal.ogg"));

		VariantMap vm;
		vm[SoundRequest::P_NODE] = node_;
		vm[SoundRequest::P_SOUNDTYPE] = SOUNDTYPE_CAST;
		SendEvent(E_SOUNDREQUEST,vm);
	}

	AOEHealed_ = true;
	AOEHealElapsedTime_ = timeRamp;
	AOEHealIntervalElapsedTime_ = timeRamp;

	clientExecuting_ = true;
	elapsedTime_ = timeRamp;
	SubscribeToEvent(E_UPDATE, HANDLER(AOEHeal, HandleUpdate));

	VariantMap vm;
	vm[AnimateSceneNode::P_NODE] = node_;
	vm[AnimateSceneNode::P_ANIMATION] = "attack";
	vm[AnimateSceneNode::P_LOOP] = false;
	vm[AnimateSceneNode::P_LAYER] = 1;
	SendEvent(E_ANIMATESCENENODE, vm);

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("AOEHeal");
		msg_.WriteVector3(targetPos_);
		msg_.WriteFloat(timeRamp);
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

void AOEHeal::HandleSetModelNodeBySceneNode(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetModelNodeBySceneNode::P_SCENENODE].GetPtr());

	if (sceneNode == targetSceneNode_)
	{
		targetModelNode_ = (Node*)(eventData[SetModelNodeBySceneNode::P_MODELNODE].GetPtr());

		UnsubscribeFromEvent(E_SETMODELNODEBYSCENENODE);
	}
}

void AOEHeal::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	elapsedTime_ += timeStep;
	if (elapsedTime_ >= cooldown_)
	{
		clientExecuting_ = false;
	}

	if (AOEHealed_)
	{
		AOEHealElapsedTime_ += timeStep;
		if (AOEHealElapsedTime_ >= AOEHealDuration_)
		{
			AOEHealed_ = false;

			if (!main_->engine_->IsHeadless())
			{
				emitterEndFX_->SetEmitting(false);
			}
		}

		AOEHealIntervalElapsedTime_ += timeStep;
		if (AOEHealIntervalElapsedTime_ >= AOEHealInterval_)
		{
			AOEHealIntervalElapsedTime_ = 0.0f;

			scene_->GetComponent<PhysicsWorld>()->GetRigidBodies(rigidBodies_, Sphere(targetPos_, radius_), 2);

			if (rigidBodies_.Size())
			{
				for (int x = 0; x < rigidBodies_.Size(); x++)
				{
					Node* noed = rigidBodies_[x]->GetNode();
					//if (noed != modelNode_)
					{
						targetModelNode_ = noed;

						SubscribeToEvent(E_SETSCENENODEBYMODELNODE, HANDLER(AOEHeal, HandleSetSceneNodeByModelNode));

						VariantMap vm;
						vm[GetSceneNodeByModelNode::P_NODE] = targetModelNode_;
						SendEvent(E_GETSCENENODEBYMODELNODE, vm);

						UnsubscribeFromEvent(E_SETSCENENODEBYMODELNODE);
						SubscribeToEvent(E_SETSCENENODECLIENTID, HANDLER(AOEHeal, HandleSetSceneNodeClientID));

						VariantMap vm0;
						vm0[GetSceneNodeClientID::P_NODE] = targetSceneNode_;
						SendEvent(E_GETSCENENODECLIENTID, vm0);

						UnsubscribeFromEvent(E_SETSCENENODECLIENTID);

						//Get target armor.
						SubscribeToEvent(E_SETCLIENTARMOR, HANDLER(AOEHeal, HandleSetArmor));

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
		}
	}

	if (cooldown_ > AOEHealDuration_)
	{
		if (elapsedTime_ >= cooldown_)
		{
			UnsubscribeFromEvent(E_UPDATE);
		}
	}
	else
	{
		if (AOEHealElapsedTime_ >= AOEHealDuration_)
		{
			UnsubscribeFromEvent(E_UPDATE);
		}
	}
}

void AOEHeal::HandleSetArmor(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientArmor::P_NODE].GetPtr());

	if (clientNode == targetSceneNode_)
	{
		targetArmor_ = eventData[SetClientArmor::P_ARMOR].GetInt();

		UnsubscribeFromEvent(E_SETCLIENTARMOR);
	}
}

void AOEHeal::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "AOEHeal")
	{
		if (clientID_ == clientID)
		{
			targetPos_ = msg.ReadVector3();
			float timeRamp = msg.ReadFloat();

			SubscribeToEvent(E_SETLAGTIME, HANDLER(AOEHeal, HandleSetLagTime));

			VariantMap vm;
			vm[GetLagTime::P_CONNECTION] = conn_;
			SendEvent(E_GETLAGTIME, vm);

			timeRamp += lagTime_;

			StartAOEHeal(targetPos_, timeRamp, false);

			if (isServer_)
			{
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("AOEHeal");
				msg_.WriteVector3(targetPos_);
				msg_.WriteFloat(timeRamp);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}

void AOEHeal::HandleSetLagTime(StringHash eventType, VariantMap& eventData)
{
	Connection* sender = (Connection*)(eventData[SetLagTime::P_CONNECTION].GetPtr());
	if (sender == conn_)
	{
		lagTime_ = eventData[SetLagTime::P_LAGTIME].GetFloat();

		UnsubscribeFromEvent(E_SETLAGTIME);
	}
}

void AOEHeal::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		if (!AOEHealed_)
		{
			return;
		}

		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		SubscribeToEvent(E_SETLAGTIME, HANDLER(AOEHeal, HandleSetLagTime));

		VariantMap vm;
		vm[GetLagTime::P_CONNECTION] = conn;
		SendEvent(E_GETLAGTIME, vm);

		float timeRamp = elapsedTime_ + lagTime_;

		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("AOEHeal");
		msg_.WriteVector3(targetPos_);
		msg_.WriteFloat(timeRamp);
		conn->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}
