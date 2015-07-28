/*
 * Blind.cpp
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

#include "Blind.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

Blind::Blind(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	blind_ = false;
	network_ = GetSubsystem<Network>();
	isServer_ = false;
}

Blind::~Blind()
{
}

void Blind::Start()
{
	SubscribeToEvent(E_GETCLIENTBLIND, HANDLER(Blind, HandleGetBlind));
	SubscribeToEvent(E_SETISSERVER, HANDLER(Blind, HandleSetIsServer));
	SubscribeToEvent(E_MODIFYCLIENTBLIND, HANDLER(Blind, HandleModifyBlind));
	SubscribeToEvent(E_SETCLIENTID, HANDLER(Blind, HandleSetClientID));
	SubscribeToEvent(E_SETCONNECTION, HANDLER(Blind, HandleSetConnection));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);
}

void Blind::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();
}

void Blind::HandleGetBlind(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientBlind::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetClientBlind::P_NODE] = clientNode;
		vm[SetClientBlind::P_BLIND] = blind_;
		SendEvent(E_SETCLIENTBLIND, vm);
	}
}

void Blind::HandleSetClientID(StringHash eventType, VariantMap& eventData)
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

void Blind::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());

		SubscribeToEvent(E_LCMSG, HANDLER(Blind, HandleLCMSG));
		SubscribeToEvent(E_GETLC, HANDLER(Blind, HandleGetLc));
	}
}

void Blind::HandleModifyBlind(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[ModifyClientBlind::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		bool state = eventData[ModifyClientBlind::P_STATE].GetBool();
		bool sendToServer = eventData[ModifyClientBlind::P_SENDTOSERVER].GetBool();

		ModifyBlind(state, sendToServer);
	}
}

void Blind::ModifyBlind(bool state, bool sendToServer)
{
	blind_ = state;

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Blind");
		msg_.WriteBool(state);
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

void Blind::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "Blind")
	{
		if (clientID_ == clientID)
		{
			bool state = msg.ReadBool();

			ModifyBlind(state, false);

			if (isServer_)
			{
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("Blind");
				msg_.WriteBool(state);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}

void Blind::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Blind");
		msg_.WriteBool(blind_);
		conn->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}
