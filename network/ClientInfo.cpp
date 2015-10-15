/*
 * ClientInfo.cpp
 *
 *  Created on: Jul 5, 2015
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

#include "ClientInfo.h"
#include "NetworkConstants.h"
#include "../Constants.h"

ClientInfo::ClientInfo(Context* context, Urho3DPlayer* main, int clientID, Connection* connection) :
	LogicComponent(context)
{
	main_ = main;
	clientID_ = clientID;
	connection_ = connection;
	network_ = GetSubsystem<Network>();
}

ClientInfo::~ClientInfo()
{
}

void ClientInfo::Start()
{
	SubscribeToEvent(E_CLIENTSYNC, HANDLER(ClientInfo, HandleClientSync));
	//SubscribeToEvent(E_NETWORKMESSAGE, HANDLER(ClientInfo, HandleNetworkMessage));
	SubscribeToEvent(E_GETCLIENTID, HANDLER(ClientInfo, HandleGetClientID));
	SubscribeToEvent(E_GETCONNECTION, HANDLER(ClientInfo, HandleGetConnection));
	SubscribeToEvent(E_GETSCENENODECLIENTID, HANDLER(ClientInfo, HandleGetSceneNodeClientID));
}

void ClientInfo::HandleClientSync(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("client sync requested");

	Connection* sender = static_cast<Connection*>(eventData[ClientSync::P_CONNECTION].GetPtr());
	int clientID = eventData[ClientSync::P_CLIENTID].GetInt();

	if (clientID == clientID_)
	{
		return;
	}

	msg_.Clear();
	msg_.WriteInt(clientID_);
	sender->SendMessage(MSG_CLIENTID, true, true, msg_);
	//LOGERRORF("synching: sent clientID %d to new client %s",clientID_,sender->GetAddress().CString());

	if (connection_ != NULL)//send this old client the new client id.
	{
		msg_.Clear();
		msg_.WriteInt(clientID);
		connection_->SendMessage(MSG_CLIENTID, true, true, msg_);
	}
}

void ClientInfo::HandleNetworkMessage(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("network message");
	using namespace NetworkMessage;

	int msgID = eventData[P_MESSAGEID].GetInt();

	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	//if (msgID == MSG_BANANA)
	{
		const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
		MemoryBuffer msg(data);
		int clientID = msg.ReadInt();

	}
}

void ClientInfo::HandleGetClientID(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientID::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetClientID::P_NODE] = clientNode;
		vm[SetClientID::P_CLIENTID] = clientID_;
		SendEvent(E_SETCLIENTID, vm);
	}
}

void ClientInfo::HandleGetConnection(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetConnection::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		VariantMap vm;
		vm[SetConnection::P_NODE] = clientNode;
		vm[SetConnection::P_CONNECTION] = connection_;
		SendEvent(E_SETCONNECTION, vm);
	}
}

void ClientInfo::HandleGetSceneNodeClientID(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetClientID::P_NODE].GetPtr());

	if (main_->GetRootNode(clientNode) == node_)
	{
		VariantMap vm;
		vm[SetSceneNodeClientID::P_NODE] = clientNode;
		vm[SetSceneNodeClientID::P_CLIENTID] = clientID_;
		SendEvent(E_SETSCENENODECLIENTID, vm);
	}
}
