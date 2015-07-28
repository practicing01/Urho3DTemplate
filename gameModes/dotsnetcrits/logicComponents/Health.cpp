/*
 * Health.cpp
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

#include "Health.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

Health::Health(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	network_ = GetSubsystem<Network>();
	health_ = 100;
	isServer_ = false;
}

Health::~Health()
{
}

void Health::Start()
{
	SubscribeToEvent(E_GETCLIENTHEALTH, HANDLER(Health, HandleGetHealth));
	SubscribeToEvent(E_SETISSERVER, HANDLER(Health, HandleSetIsServer));
	SubscribeToEvent(E_MODIFYCLIENTHEALTH, HANDLER(Health, HandleModifyHealth));
	SubscribeToEvent(E_SETCLIENTID, HANDLER(Health, HandleSetClientID));
	SubscribeToEvent(E_SETCONNECTION, HANDLER(Health, HandleSetConnection));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);
}

void Health::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();

	VariantMap vm;
	vm[GetClientID::P_NODE] = main_->GetRootNode(node_);
	SendEvent(E_GETCLIENTID, vm);
}

void Health::HandleSetClientID(StringHash eventType, VariantMap& eventData)
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

void Health::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());

		SubscribeToEvent(E_LCMSG, HANDLER(Health, HandleLCMSG));
		SubscribeToEvent(E_GETLC, HANDLER(Health, HandleGetLc));
	}
}

void Health::HandleGetHealth(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientHealth::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetClientHealth::P_NODE] = clientNode;
		vm[SetClientHealth::P_HEALTH] = health_;
		SendEvent(E_SETCLIENTHEALTH, vm);
	}
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
		msg_.WriteInt(clientID_);
		msg_.WriteString("Health");
		msg_.WriteInt(healthMod);
		msg_.WriteInt(operation);
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

void Health::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "Health")
	{
		if (clientID_ == clientID)
		{
			int healthMod = msg.ReadInt();
			int operation = msg.ReadInt();

			ModifyHealth(healthMod, operation, false);

			if (isServer_)
			{
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("Health");
				msg_.WriteInt(healthMod);
				msg_.WriteInt(operation);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}

void Health::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Health");
		msg_.WriteInt(health_);
		msg_.WriteInt(0);
		conn->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}
