/*
 * Client.cpp
 *
 *  Created on: Jul 8, 2015
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

#include "Client.h"
#include "NetworkConstants.h"
#include "ClientInfo.h"
#include "NetPulse.h"
#include "../GameMenu.h"
#include "../Constants.h"

Client::Client(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	network_ = GetSubsystem<Network>();
}

Client::~Client()
{
}

void Client::Start()
{
	main_->myRootNode_->AddComponent(new GameMenu(context_, main_), 0, LOCAL);

	VariantMap vm;
	vm[GameMenuDisplay::P_STATE] = true;
	SendEvent(E_GAMEMENUDISPLAY, vm);

	SubscribeToEvent(E_NETWORKMESSAGE, HANDLER(Client, HandleNetworkMessage));
}

void Client::HandleNetworkMessage(StringHash eventType, VariantMap& eventData)
{
	using namespace NetworkMessage;

	int msgID = eventData[P_MESSAGEID].GetInt();

	if (msgID == MSG_MYCLIENTID)
	{
		const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
		MemoryBuffer msg(data);
		int clientID = msg.ReadInt();

		if (main_->myRootNode_->HasComponent<ClientInfo>())
		{
			main_->myRootNode_->RemoveComponent(
					main_->myRootNode_->GetComponent<ClientInfo>());
		}
		main_->myRootNode_->AddComponent(new ClientInfo(context_, main_, clientID, NULL), 0, LOCAL);

		if (main_->myRootNode_->HasComponent<NetPulse>())
		{
			main_->myRootNode_->RemoveComponent(
					main_->myRootNode_->GetComponent<NetPulse>());
		}
		main_->myRootNode_->AddComponent(new NetPulse(context_, main_), 0, LOCAL);

		msg_.Clear();
		network_->GetServerConnection()->SendMessage(MSG_GOTMYCLIENTID, true, true, msg_);
	}
	else if (msgID == MSG_CLIENTID)
	{
		const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
		MemoryBuffer msg(data);
		int clientID = msg.ReadInt();

		SharedPtr<Node> rootNode = SharedPtr<Node>( new Node(context_) );
		SharedPtr<Node> sceneNode = SharedPtr<Node>( new Node(context_) );

		main_->rootNodes_.Push(rootNode);
		main_->sceneNodes_.Push(sceneNode);

		main_->scene_->AddChild(rootNode);

		rootNode->AddComponent(new ClientInfo(context_, main_, clientID, NULL), 0, LOCAL);

		VariantMap vm;
		vm[NewClientID::P_CLIENTID] = clientID;
		SendEvent(E_NEWCLIENTID, vm);
	}
	else if (msgID == MSG_CLIENTDISCO)
	{
		const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
		MemoryBuffer msg(data);
		int clientID = msg.ReadInt();

		SharedPtr<Node> rootNode = SharedPtr<Node>( main_->GetRootNode(clientID) );

		if (rootNode != NULL)
		{
			main_->RemoveRootNode(rootNode);
		}
	}
	else if (msgID == MSG_LOADGAMEMODE)
	{
		const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
		MemoryBuffer msg(data);
		String gameMode = msg.ReadString();

		LoadGameMode(gameMode);

		msg_.Clear();
		msg_.WriteInt(main_->myRootNode_->GetComponent<ClientInfo>()->clientID_);
		network_->GetServerConnection()->SendMessage(MSG_LOADEDGAMEMODE, true, true, msg_);
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

void Client::LoadGameMode(String gameMode)
{
	if (gameMode == "DotsNetCrits")
	{
		main_->myRootNode_->AddComponent(new DotsNetCrits(context_, main_, false), 0, LOCAL);
	}
}
