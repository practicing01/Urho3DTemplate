/*
 * MasterServer.cpp
 *
 *  Created on: Jun 29, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>

#include "MasterServer.h"
#include "NetworkConstants.h"
#include "ServerQuery.h"

MasterServer::MasterServer(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	elapsedTime_ = 0.0f;

	main_->network_->StartServer(9001);

	network_ = GetSubsystem<Network>();

	SubscribeToEvent(E_CLIENTDISCONNECTED, HANDLER(MasterServer, HandleClientDisconnect));
	SubscribeToEvent(E_CLIENTCONNECTED, HANDLER(MasterServer, HandleClientConnect));
	SubscribeToEvent(E_NETWORKMESSAGE, HANDLER(MasterServer, HandleNetworkMessage));
	SubscribeToEvent(E_UPDATE, HANDLER(MasterServer, HandleUpdate));
}

MasterServer::~MasterServer()
{
	for (int x = 0; x < servers_.Size(); x++)
	{
		delete servers_[x];
	}

	for (int x = 0; x < serverQueries_.Size(); x++)
	{
		delete serverQueries_[x];
	}
}

void MasterServer::Start()
{
	//LOGERRORF("master server attached");
}

void MasterServer::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	//LOGERRORF("MasterServer loop");
	elapsedTime_ += timeStep;

	for (int x = 0; x < serverQueries_.Size(); x++)
	{
		if (servers_.Empty())
		{
			msg_.Clear();
			serverQueries_[x]->connection_->SendMessage(MSG_SERVERSSENT, true, true, msg_);
		}
		else
		{
			msg_.Clear();
			msg_.WriteString(servers_[serverQueries_[x]->index_]->serverName_.CString());
			msg_.WriteString(servers_[serverQueries_[x]->index_]->gameMode_.CString());
			msg_.WriteString(servers_[serverQueries_[x]->index_]->connection_->GetAddress().CString());
			serverQueries_[x]->connection_->SendMessage(MSG_SERVERINFO, true, true, msg_);

			serverQueries_[x]->index_++;
		}
	}

	for (int x = 0; x < serverQueries_.Size(); x++)//Prune.
	{
		if (serverQueries_[x]->index_ == servers_.Size())
		{
			msg_.Clear();
			serverQueries_[x]->connection_->SendMessage(MSG_SERVERSSENT, true, true, msg_);

			ServerQuery* sq = serverQueries_[x];
			serverQueries_.Remove(sq);
			delete sq;
			x = 0;
		}
	}
}

void MasterServer::HandleClientConnect(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("client connected to MasterServer");

	using namespace NetworkMessage;

	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
}

void MasterServer::HandleClientDisconnect(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("client diconnected from MasterServer");

	using namespace NetworkMessage;

	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	for (int x = 0; x < serverQueries_.Size(); x++)
	{
		if (serverQueries_[x]->connection_ == sender)
		{
			ServerQuery* sq = serverQueries_[x];
			serverQueries_.Remove(sq);
			delete sq;
			break;
		}
	}

	for (int x = 0; x < servers_.Size(); x++)
	{
		if (servers_[x]->connection_ == sender)
		{
			ServerInfo* si = servers_[x];
			servers_.Remove(si);
			delete si;
			break;
		}
	}
}

void MasterServer::HandleNetworkMessage(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("network message");
	using namespace NetworkMessage;

	int msgID = eventData[P_MESSAGEID].GetInt();

	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	if (msgID == MSG_PUSHSERVER)
	{
		const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
		MemoryBuffer msg(data);
		String serverName = msg.ReadString();
		String gameMode = msg.ReadString();

		ServerInfo* si = new ServerInfo();
		si->serverName_ = serverName;
		si->gameMode_ = gameMode;
		si->connection_ = sender;

		servers_.Push(si);
	}
	else if (msgID == MSG_POPSERVER)
	{
		for (int x = 0; x < servers_.Size(); x++)
		{
			if (servers_[x]->connection_ == sender)
			{
				ServerInfo* server = servers_[x];
				servers_.Remove(server);
				delete server;
				break;
			}
		}
	}
	else if (msgID == MSG_GETSERVERS)
	{
		ServerQuery* sq = new ServerQuery();
		sq->connection_ = sender;

		serverQueries_.Push(sq);
	}
}
