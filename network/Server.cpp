/*
 * Server.cpp
 *
 *  Created on: Jun 29, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>

#include "Server.h"
#include "NetworkConstants.h"
#include "ClientInfo.h"
#include "NetPulse.h"
#include "../Constants.h"

Server::Server(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	elapsedTime_ = 0.0f;

	main_->network_->StartServer(9002);

	network_ = GetSubsystem<Network>();

	clientIDCount_ = 0;

    SubscribeToEvent(E_SERVERCONNECTED, HANDLER(Server, HandleServerConnect));
    SubscribeToEvent(E_SERVERDISCONNECTED, HANDLER(Server, HandleServerDisconnect));
    SubscribeToEvent(E_CONNECTFAILED, HANDLER(Server, HandleConnectFailed));
	SubscribeToEvent(E_CLIENTDISCONNECTED, HANDLER(Server, HandleClientDisconnect));
	SubscribeToEvent(E_CLIENTCONNECTED, HANDLER(Server, HandleClientConnect));
	SubscribeToEvent(E_NETWORKMESSAGE, HANDLER(Server, HandleNetworkMessage));
	SubscribeToEvent(E_EXCLUSIVENETBROADCAST, HANDLER(Server, HandleExclusiveNetBroadcast));
}

Server::~Server()
{
	msg_.Clear();
	network_->GetServerConnection()->SendMessage(MSG_POPSERVER, true, true, msg_);
}

void Server::Start()
{
	XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>("Objects/serverInfo.xml");
	Node* serverInfo = main_->scene_->InstantiateXML(xmlFile->GetRoot(), Vector3::ZERO, Quaternion(), LOCAL);

	serverName_ = serverInfo->GetVar("serverName").GetString();
	gameMode_ = serverInfo->GetVar("gameMode").GetString();
	masterServerIP_ = serverInfo->GetVar("masterServerIP").GetString();

	network_->Connect(masterServerIP_, 9001, 0);

	LoadGameMode(gameMode_);
}

void Server::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	//LOGERRORF("Server loop");
	elapsedTime_ += timeStep;
}

void Server::HandleServerConnect(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("server connected to server");
	msg_.Clear();
	msg_.WriteString(serverName_.CString());
	msg_.WriteString(gameMode_.CString());
	network_->GetServerConnection()->SendMessage(MSG_PUSHSERVER, true, true, msg_);
}

void Server::HandleServerDisconnect(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("server diconnected from server");
}

void Server::HandleConnectFailed(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("server connection failed");
}

void Server::HandleClientConnect(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("client connected to Server");

	using namespace NetworkMessage;

	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	SharedPtr<Node> rootNode = SharedPtr<Node>( new Node(context_) );
	SharedPtr<Node> sceneNode = SharedPtr<Node>( new Node(context_) );

	main_->rootNodes_.Push(rootNode);
	main_->sceneNodes_.Push(sceneNode);

	main_->scene_->AddChild(rootNode);

	rootNode->AddComponent(new ClientInfo(context_, main_, clientIDCount_, sender), 0, LOCAL);

	msg_.Clear();
	msg_.WriteInt(clientIDCount_);
	sender->SendMessage(MSG_MYCLIENTID, true, true, msg_);

	clientIDCount_++;
}

void Server::HandleClientDisconnect(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("client diconnected from Server");

	using namespace NetworkMessage;

	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	node_->GetComponent<NetPulse>()->RemoveConnection(sender);

	SharedPtr<Node> rootNode = SharedPtr<Node>( main_->GetRootNode(sender) );

	if (rootNode != NULL)
	{
		msg_.Clear();
		msg_.WriteInt(rootNode->GetComponent<ClientInfo>()->clientID_);
		network_->BroadcastMessage(MSG_CLIENTDISCO, true, true, msg_);
		main_->RemoveRootNode(rootNode);
	}
}

void Server::HandleNetworkMessage(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("network message");
	using namespace NetworkMessage;

	int msgID = eventData[P_MESSAGEID].GetInt();

	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	if (msgID == MSG_GOTMYCLIENTID)
	{
		msg_.Clear();
		msg_.WriteString(gameMode_);
		sender->SendMessage(MSG_LOADGAMEMODE, true, true, msg_);
	}
	else if (msgID == MSG_LOADEDGAMEMODE)
	{
		const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
		MemoryBuffer msg(data);
		int clientID = msg.ReadInt();

		VariantMap vm;
		vm[ClientSync::P_CONNECTION] = sender;
		vm[ClientSync::P_CLIENTID] = clientID;
		SendEvent(E_CLIENTSYNC, vm);

		VariantMap vm0;
		vm0[NewClientID::P_CLIENTID] = clientID;
		SendEvent(E_NEWCLIENTID, vm0);
	}
	else if (msgID == MSG_LCMSG)
	{
		const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
		MemoryBuffer msg(data);

		VariantMap vm;
		vm[LcMsg::P_DATA] = data;
		SendEvent(E_LCMSG, vm);
	}
}

void Server::LoadGameMode(String gameMode)
{
	gameMode_ = gameMode;

	if (gameMode_ == "DotsNetCrits")//Todo find better way of loading game modes
	{
		main_->myRootNode_->AddComponent(new DotsNetCrits(context_, main_, true), 0, LOCAL);
	}
}

void Server::HandleExclusiveNetBroadcast(StringHash eventType, VariantMap& eventData)
{
	Connection* conn_ = (Connection*)(eventData[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION].GetPtr());
	const PODVector<unsigned char>& data = eventData[ExclusiveNetBroadcast::P_MSG].GetBuffer();
	VectorBuffer msg(data);

	for (int x = 0; x < main_->rootNodes_.Size(); x++)
	{
		if (main_->rootNodes_[x]->GetComponent<ClientInfo>())
		{
			if (main_->rootNodes_[x]->GetComponent<ClientInfo>()->connection_ != conn_)
			{
				main_->rootNodes_[x]->GetComponent<ClientInfo>()->connection_->
						SendMessage(MSG_LCMSG, true, true, msg);
			}
		}
	}
}
