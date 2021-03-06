/*
 * DOT.cpp
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

#include "DOT.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

#include "LC.h"
#include "LCTarget.h"

DOT::DOT(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	lc_ = new LC(context, main, main->network_);

	DOT_ = 10.0f;

	SetUpdateEventMask(USE_UPDATE);
	//SubscribeToEvent(E_CLEANSESTATUS, HANDLER(DOT, HandleCleanseStatus));
}

DOT::~DOT()
{
	if (lc_->clientExecuting_)
	{
		VariantMap vm;
		SendEvent(E_TOUCHUNSUBSCRIBE, vm);
	}

	for (int x = 0; x < targets_.Size(); x++)
	{
		delete targets_[x];
	}

	delete lc_;
}

void DOT::Start()
{
	lc_->Start(node_);

	lc_->cooldown_ = 10.0f;

	SubscribeToEvent(E_LCMSG, HANDLER(DOT, HandleLCMSG));
	SubscribeToEvent(E_GETLC, HANDLER(DOT, HandleGetLc));
	SubscribeToEvent(E_CLEANSE, HANDLER(DOT, HandleCleanse));

	if (lc_->main_->IsLocalClient(node_))
	{
		SubscribeToEvent(E_MECHANICREQUEST, HANDLER(DOT, HandleMechanicRequest));
	}
}

void DOT::HandleMechanicRequest(StringHash eventType, VariantMap& eventData)
{
	String mechanicID = eventData[MechanicRequest::P_MECHANICID].GetString();

	if (mechanicID == "DOT")
	{
		SubscribeToEvent(E_SETCLIENTSILENCE, HANDLER(DOT, HandleSetEnabled));

		VariantMap vm;
		vm[GetClientSilence::P_NODE] = node_;
		SendEvent(E_GETCLIENTSILENCE, vm);
	}
}

void DOT::HandleSetEnabled(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetClientSilence::P_NODE].GetPtr());

	if (sceneNode == node_)
	{
		lc_->disabled_ = eventData[SetClientSilence::P_SILENCE].GetBool();

		UnsubscribeFromEvent(E_SETCLIENTSILENCE);

		if (!lc_->clientExecuting_ && !lc_->disabled_)
		{
			lc_->clientExecuting_ = true;
			lc_->elapsedTime_ = 0.0f;

			VariantMap vm;
			SendEvent(E_TOUCHSUBSCRIBE, vm);

			SubscribeToEvent(E_TOUCHEND, HANDLER(DOT, HandleTouchEnd));
		}
	}
}

void DOT::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
{
	if (lc_->main_->ui_->GetFocusElement())
	{
		return;
	}

	VariantMap vm;
	SendEvent(E_TOUCHUNSUBSCRIBE, vm);

	UnsubscribeFromEvent(E_TOUCHEND);

	using namespace TouchEnd;

	Ray cameraRay = lc_->cameraNode_->GetComponent<Camera>()->GetScreenRay(
			(float) eventData[P_X].GetInt() / lc_->main_->graphics_->GetWidth(),
			(float) eventData[P_Y].GetInt() / lc_->main_->graphics_->GetHeight());

	PhysicsRaycastResult raeResult_;

	lc_->scene_->GetComponent<PhysicsWorld>()->RaycastSingle(raeResult_, cameraRay, 1000.0f, 2);//todo define masks.

	if (raeResult_.body_)
	{
		Node* modelNode = raeResult_.body_->GetNode();

		/*for (int x = 0; x < targets_.Size(); x++)
		{
			if (targets_[x]->modelNode_ == modelNode)//Can't cast multiple times on the same model for this skill.
			{
				return;
			}
		}*/

		Exec(lc_->GetModelNodeClientID(modelNode), 0.0f, true);
	}
	else
	{
		lc_->clientExecuting_ = false;
		return;
	}
}

void DOT::Exec(int clientID, float timeRamp, bool sendToServer)
{
	LCTarget* target = new LCTarget(context_);
	targets_.Push(target);

	target->sceneNode_ = lc_->main_->GetSceneNode(clientID);

	target->duration_ = 5.0f;

	target->interval_ = 1.0f;

	target->GetModelNodeBySceneNode();

	target->GetSceneNodeClientID();

	if (!lc_->main_->engine_->IsHeadless())
	{
		target->particleEndNode_ = lc_->scene_->CreateChild(0,LOCAL);
		target->emitterEndFX_ = target->particleEndNode_->CreateComponent<ParticleEmitter>(LOCAL);
		target->emitterEndFX_->SetEffect(lc_->main_->cache_->GetResource<ParticleEffect>("Particle/dot.xml"));
		target->particleEndNode_->SetWorldScale(Vector3::ONE * 500.0f);
		target->emitterEndFX_->SetViewMask(1);

		target->particleEndNode_->CreateComponent<SoundSource3D>(LOCAL);

		target->modelNode_->AddChild(target->particleEndNode_);
		Vector3 victoria = target->modelNode_->GetPosition();
		victoria.y_ += target->beeBox_.Size().y_;
		target->particleEndNode_->SetWorldPosition(victoria);
		target->emitterEndFX_->SetEmitting(true);
		target->particleEndNode_->GetComponent<SoundSource3D>()->Play(lc_->main_->cache_->GetResource<Sound>("Sounds/dot/dot.ogg"));

		VariantMap vm;
		vm[SoundRequest::P_NODE] = node_;
		vm[SoundRequest::P_SOUNDTYPE] = SOUNDTYPE_CAST;
		SendEvent(E_SOUNDREQUEST,vm);
	}

	target->elapsedTime_ = timeRamp;
	target->intervalElapsedTime_ = timeRamp;

	lc_->elapsedTime_ = timeRamp;

	VariantMap vm;
	vm[AnimateSceneNode::P_NODE] = node_;
	vm[AnimateSceneNode::P_ANIMATION] = "attack";
	vm[AnimateSceneNode::P_LOOP] = false;
	vm[AnimateSceneNode::P_LAYER] = 1;
	SendEvent(E_ANIMATESCENENODE, vm);

	if (sendToServer)
	{
		lc_->msg_.Clear();
		lc_->msg_.WriteInt(lc_->clientID_);
		lc_->msg_.WriteString("DOT");
		lc_->msg_.WriteInt(target->clientID_);
		lc_->msg_.WriteFloat(timeRamp);
		if (!lc_->isServer_)
		{
			lc_->network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, lc_->msg_);
		}
		else
		{
			lc_->network_->BroadcastMessage(MSG_LCMSG, true, true, lc_->msg_);
		}
	}

	SubscribeToEvent(E_UPDATE, HANDLER(DOT, HandleUpdate));
}

void DOT::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	lc_->elapsedTime_ += timeStep;
	if (lc_->elapsedTime_ >= lc_->cooldown_)
	{
		lc_->clientExecuting_ = false;
	}

	int targetCount = targets_.Size();

	for (int x = 0; x < targetCount; x++)
	{
		targets_[x]->elapsedTime_ += timeStep;

		if (targets_[x]->elapsedTime_ >= targets_[x]->duration_)
		{
			if (!lc_->main_->engine_->IsHeadless())
			{
				targets_[x]->particleEndNode_->Remove();
			}

			LCTarget* target = targets_[x];
			targets_.Remove(target);
			delete target;
			x--;
			targetCount = targets_.Size();

			break;
		}

		targets_[x]->intervalElapsedTime_ += timeStep;
		if (targets_[x]->intervalElapsedTime_ >= targets_[x]->interval_)
		{
			targets_[x]->intervalElapsedTime_ = 0.0f;

			targetSceneNode_ = targets_[x]->sceneNode_;

			//Get target armor.
			SubscribeToEvent(E_SETCLIENTARMOR, HANDLER(DOT, HandleSetArmor));

			VariantMap vm;
			vm[GetClientArmor::P_NODE] = targets_[x]->sceneNode_;
			SendEvent(E_GETCLIENTARMOR, vm);

			int damage = DOT_ - targetArmor_;

			if (damage > 0)
			{
				vm.Clear();
				vm[ModifyClientHealth::P_NODE] = targets_[x]->sceneNode_;
				vm[ModifyClientHealth::P_HEALTH] = damage;
				vm[ModifyClientHealth::P_OPERATION] = -1;
				vm[ModifyClientHealth::P_SENDTOSERVER] = false;
				SendEvent(E_MODIFYCLIENTHEALTH, vm);
			}
		}
	}

	if (!lc_->clientExecuting_ && !targets_.Size())
	{
		UnsubscribeFromEvent(E_UPDATE);
	}
}

void DOT::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "DOT")
	{
		if (lc_->clientID_ == clientID)
		{
			int clientID = msg.ReadInt();
			float timeRamp = msg.ReadFloat();

			lc_->GetLagTime(lc_->conn_);

			timeRamp += lc_->lagTime_;

			Exec(clientID, timeRamp, false);

			if (lc_->isServer_)
			{
				lc_->msg_.Clear();
				lc_->msg_.WriteInt(lc_->clientID_);
				lc_->msg_.WriteString("DOT");
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

void DOT::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (sceneNode == node_)
	{
		if (!targets_.Size())
		{
			return;
		}

		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		lc_->GetLagTime(conn);

		for (int x = 0; x < targets_.Size(); x++)
		{
			float timeRamp = targets_[x]->elapsedTime_ + lc_->lagTime_;

			lc_->msg_.Clear();
			lc_->msg_.WriteInt(lc_->clientID_);
			lc_->msg_.WriteString("DOT");
			lc_->msg_.WriteInt(targets_[x]->clientID_);
			lc_->msg_.WriteFloat(timeRamp);
			conn->SendMessage(MSG_LCMSG, true, true, lc_->msg_);
		}
	}
}

void DOT::HandleSetArmor(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientArmor::P_NODE].GetPtr());

	if (clientNode == targetSceneNode_)
	{
		targetArmor_ = eventData[SetClientArmor::P_ARMOR].GetInt();

		UnsubscribeFromEvent(E_SETCLIENTARMOR);
	}
}

void DOT::HandleCleanse(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[CleanseStatus::P_NODE].GetPtr());

	int targetCount = targets_.Size();

	for (int x = 0; x < targetCount; x++)
	{
		if (targets_[x]->sceneNode_ >= sceneNode)
		{
			if (!lc_->main_->engine_->IsHeadless())
			{
				targets_[x]->particleEndNode_->Remove();
			}

			LCTarget* target = targets_[x];
			targets_.Remove(target);
			delete target;
			x--;
			targetCount = targets_.Size();
		}
	}
}
