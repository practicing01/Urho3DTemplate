/*
 * Health.cpp
 *
 *  Created on: Oct 23, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>

#include "Health.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

#include "NodeInfo.h"

Health::Health(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	health_ = 100;
}

Health::~Health()
{
}

void Health::Start()
{
	SubscribeToEvent(E_LCMSG, URHO3D_HANDLER(Health, HandleLCMSG));
	SubscribeToEvent(E_GETLC, URHO3D_HANDLER(Health, HandleGetLc));
	SubscribeToEvent(E_MODIFYCLIENTHEALTH, URHO3D_HANDLER(Health, HandleModifyHealth));
}

void Health::HandleModifyHealth(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[ModifyClientHealth::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		int healthMod = eventData[ModifyClientHealth::P_HEALTH].GetInt();
		int operation = eventData[ModifyClientHealth::P_OPERATION].GetInt();
		bool sendToServer = eventData[ModifyClientHealth::P_SENDTOSERVER].GetBool();

		ModifyHealth(healthMod, operation, sendToServer);
	}
}

void Health::ModifyHealth(int healthMod, int operation, bool sendToServer)
{
	if (operation == 0)
	{
		health_ = healthMod;
	}
	else if (operation == 1)
	{
		health_ += healthMod;
	}
	else if (operation == -1)
	{
		health_ -= healthMod;

		VariantMap vm;
		vm[SoundRequest::P_NODE] = node_;
		vm[SoundRequest::P_SOUNDTYPE] = SOUNDTYPE_HURT;
		SendEvent(E_SOUNDREQUEST,vm);

	}

	VariantMap vm;
	vm[ClientHealthSet::P_NODE] = node_;
	vm[ClientHealthSet::P_HEALTH] = health_;
	SendEvent(E_CLIENTHEALTHSET, vm);

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(node_->GetComponent<NodeInfo>()->clientID_);
		msg_.WriteString("Health");
		msg_.WriteInt(node_->GetComponent<NodeInfo>()->nodeID_);
		msg_.WriteInt(healthMod);
		msg_.WriteInt(operation);
		if (!main_->network_->IsServerRunning())
		{
			main_->network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg_);
		}
		else
		{
			main_->network_->BroadcastMessage(MSG_LCMSG, true, true, msg_);
		}
	}
}

void Health::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "Health")
	{
		int nodeID = msg.ReadInt();

		if (node_->GetComponent<NodeInfo>()->clientID_ == clientID
				&& node_->GetComponent<NodeInfo>()->nodeID_ == nodeID)
		{
			int healthMod = msg.ReadInt();
			int operation = msg.ReadInt();

			ModifyHealth(healthMod, operation, false);

			if (main_->network_->IsServerRunning())
			{
				msg_.Clear();
				msg_.WriteInt(clientID);
				msg_.WriteString("Health");
				msg_.WriteInt(nodeID);
				msg_.WriteInt(healthMod);
				msg_.WriteInt(operation);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] =
						main_->GetConnByClientID(clientID);
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}

void Health::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode != node_)
	{
		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		msg_.Clear();
		msg_.WriteInt(node_->GetComponent<NodeInfo>()->clientID_);
		msg_.WriteString("Health");
		msg_.WriteInt(node_->GetComponent<NodeInfo>()->nodeID_);
		msg_.WriteInt(health_);
		msg_.WriteInt(0);
		conn->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}
