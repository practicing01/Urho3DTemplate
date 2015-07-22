/*
 * Armor.cpp
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

#include "Armor.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

Armor::Armor(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	network_ = GetSubsystem<Network>();
	armor_ = 0;
	isServer_ = false;
}

Armor::~Armor()
{
}

void Armor::Start()
{
	SubscribeToEvent(E_GETCLIENTARMOR, HANDLER(Armor, HandleGetArmor));
	SubscribeToEvent(E_SETISSERVER, HANDLER(Armor, HandleSetIsServer));
	SubscribeToEvent(E_MODIFYCLIENTARMOR, HANDLER(Armor, HandleModifyArmor));
	SubscribeToEvent(E_SETCLIENTID, HANDLER(Armor, HandleSetClientID));
	SubscribeToEvent(E_SETCONNECTION, HANDLER(Armor, HandleSetConnection));

	VariantMap vm0;
	SendEvent(E_GETISSERVER, vm0);
}

void Armor::HandleSetIsServer(StringHash eventType, VariantMap& eventData)
{
	isServer_ = eventData[SetIsServer::P_ISSERVER].GetBool();

	VariantMap vm;
	vm[GetClientID::P_NODE] = main_->GetRootNode(node_);
	SendEvent(E_GETCLIENTID, vm);
}

void Armor::HandleSetClientID(StringHash eventType, VariantMap& eventData)
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

void Armor::HandleSetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetConnection::P_NODE].GetPtr());

	if (main_->GetSceneNode(clientNode) == node_)
	{
		conn_ = (Connection*)(eventData[SetConnection::P_CONNECTION].GetPtr());

		SubscribeToEvent(E_LCMSG, HANDLER(Armor, HandleLCMSG));
		SubscribeToEvent(E_GETLC, HANDLER(Armor, HandleGetLc));
	}
}

void Armor::HandleGetArmor(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientArmor::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetClientArmor::P_NODE] = clientNode;
		vm[SetClientArmor::P_ARMOR] = armor_;
		SendEvent(E_SETCLIENTARMOR, vm);
	}
}

void Armor::HandleModifyArmor(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[ModifyClientArmor::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		int armorMod = eventData[ModifyClientArmor::P_ARMOR].GetInt();
		int operation = eventData[ModifyClientArmor::P_OPERATION].GetInt();
		bool sendToServer = eventData[ModifyClientArmor::P_SENDTOSERVER].GetBool();

		ModifyArmor(armorMod, operation, sendToServer);
	}
}

void Armor::ModifyArmor(int armorMod, int operation, bool sendToServer)
{
	if (operation == 0)
	{
		armor_ = armorMod;
	}
	else if (operation == 1)
	{
		armor_ += armorMod;
	}
	else if (operation == -1)
	{
		armor_ -= armorMod;
	}

	if (sendToServer)
	{
		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Armor");
		msg_.WriteInt(armorMod);
		msg_.WriteInt(operation);
		network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}

void Armor::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "Armor")
	{
		if (clientID_ == clientID)
		{
			int armorMod = msg.ReadInt();
			int operation = msg.ReadInt();

			ModifyArmor(armorMod, operation, false);

			if (isServer_)
			{
				msg_.Clear();
				msg_.WriteInt(clientID_);
				msg_.WriteString("Armor");
				msg_.WriteInt(armorMod);
				msg_.WriteInt(operation);

				VariantMap vm0;
				vm0[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = conn_;
				vm0[ExclusiveNetBroadcast::P_MSG] = msg_.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm0);
			}
		}
	}
}

void Armor::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		msg_.Clear();
		msg_.WriteInt(clientID_);
		msg_.WriteString("Armor");
		msg_.WriteInt(armor_);
		msg_.WriteInt(0);
		conn->SendMessage(MSG_LCMSG, true, true, msg_);
	}
}
