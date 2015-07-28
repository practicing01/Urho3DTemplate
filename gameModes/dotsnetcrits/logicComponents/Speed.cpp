/*
 * Speed.cpp
 *
 *  Created on: Jul 13, 2015
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

#include "Speed.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

Speed::Speed(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	network_ = GetSubsystem<Network>();
	speed_ = 10.0f;
	isServer_ = false;
}

Speed::~Speed()
{
}

void Speed::Start()
{
	SubscribeToEvent(E_GETCLIENTSPEED, HANDLER(Speed, HandleGetSpeed));
	SubscribeToEvent(E_SETISSERVER, HANDLER(Speed, HandleSetIsServer));
	SubscribeToEvent(E_MODIFYCLIENTSPEED, HANDLER(Speed, HandleModifySpeed));
	SubscribeToEvent(E_SETCLIENTID, HANDLER(Speed, HandleSetClientID));
	SubscribeToEvent(E_SETCONNECTION, HANDLER(Speed, HandleSetConnection));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);
}

void Speed::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();

	VariantMap vm;
	vm[GetClientID::P_NODE] = main_->GetRootNode(node_);
	SendEvent(E_GETCLIENTID, vm);
}

void Speed::HandleSetClientID(StringHash eventType, VariantMap& eventData)
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

void Speed::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());

		SubscribeToEvent(E_LCMSG, HANDLER(Speed, HandleLCMSG));
		SubscribeToEvent(E_GETLC, HANDLER(Speed, HandleGetLc));
	}
}

void Speed::HandleGetSpeed(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientSpeed::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetClientSpeed::P_NODE] = clientNode;
		vm[SetClientSpeed::P_SPEED] = speed_;
		SendEvent(E_SETCLIENTSPEED, vm);
	}
}

void Speed::HandleModifySpeed(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[ModifyClientSpeed::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		float speedMod = eventData[ModifyClientSpeed::P_SPEED].GetFloat();
		int operation = eventData[ModifyClientSpeed::P_OPERATION].GetInt();
		bool sendToServer = eventData[ModifyClientSpeed::P_SENDTOSERVER].GetBool();

		ModifySpeed(speedMod, operation, sendToServer);
	}
}

void Speed::ModifySpeed(float speedMod, int operation, bool sendToServer)
{
	if (operation == 0)
	{
		speed_ = speedMod;
	}
	else if (operation == 1)
	{
		speed_ += speedMod;
	}
	else if (operation == -1)
	{
		speed_ -= speedMod;
	}

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Speed");
		msg_.WriteFloat(speedMod);
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

void Speed::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "Speed")
	{
		if (clientID_ == clientID)
		{
			float speedMod = msg.ReadFloat();
			int operation = msg.ReadInt();

			ModifySpeed(speedMod, operation, false);

			if (isServer_)
			{
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("Speed");
				msg_.WriteFloat(speedMod);
				msg_.WriteInt(operation);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}

void Speed::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Speed");
		msg_.WriteFloat(speed_);
		msg_.WriteInt(0);
		conn->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}
