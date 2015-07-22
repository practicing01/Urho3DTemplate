/*
 * Silence.cpp
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

#include "Silence.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

Silence::Silence(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	silence_ = false;
	network_ = GetSubsystem<Network>();
	isServer_ = false;
}

Silence::~Silence()
{
}

void Silence::Start()
{
	SubscribeToEvent(E_GETCLIENTSILENCE, HANDLER(Silence, HandleGetSilence));
	SubscribeToEvent(E_SETISSERVER, HANDLER(Silence, HandleSetIsServer));
	SubscribeToEvent(E_MODIFYCLIENTSILENCE, HANDLER(Silence, HandleModifySilence));
	SubscribeToEvent(E_SETCLIENTID, HANDLER(Silence, HandleSetClientID));
	SubscribeToEvent(E_SETCONNECTION, HANDLER(Silence, HandleSetConnection));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);
}

void Silence::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();
}

void Silence::HandleGetSilence(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientSilence::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetClientSilence::P_NODE] = clientNode;
		vm[SetClientSilence::P_SILENCE] = silence_;
		SendEvent(E_SETCLIENTSILENCE, vm);
	}
}

void Silence::HandleSetClientID(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientID::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		clientID_ = eventData[SetClientID::P_CLIENTID].GetInt();

		VariantMap vm;
		vm[GetConnection::P_NODE] = main_->GetRootNode(node_);
		SendEvent(E_GETCONNECTION, vm);

	}
}

void Silence::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());

		SubscribeToEvent(E_LCMSG, HANDLER(Silence, HandleLCMSG));
		SubscribeToEvent(E_GETLC, HANDLER(Silence, HandleGetLc));
	}
}

void Silence::HandleModifySilence(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[ModifyClientSpeed::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		bool state = eventData[ModifyClientSilence::P_STATE].GetBool();
		bool sendToServer = eventData[ModifyClientSilence::P_SENDTOSERVER].GetBool();

		ModifySilence(state, sendToServer);
	}
}

void Silence::ModifySilence(bool state, bool sendToServer)
{
	silence_ = state;

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Silence");
		msg_.WriteBool(state);
		network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}

void Silence::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "Silence")
	{
		if (clientID_ == clientID)
		{
			bool state = msg.ReadBool();

			ModifySilence(state, false);

			if (isServer_)
			{
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("Silence");
				msg_.WriteBool(state);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}

void Silence::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Silence");
		msg_.WriteBool(silence_);
		conn->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}
