/*
 * GameMenu.cpp
 *
 *  Created on: Jul 6, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "Urho3DPlayer.h"
#include "GameMenu.h"
#include "Constants.h"
#include "network/NetworkConstants.h"
#include "network/Server.h"
#include "network/ServerInfo.h"
#include "network/Client.h"

GameMenu::GameMenu(Context* context, Urho3DPlayer* main) :
LogicComponent(context)
{
	main_ = main;
	elapsedTime_ = 0.0f;

	network_ = GetSubsystem<Network>();

	masterServerConnected_ = false;

	ipAddress_ = "127.0.0.1";
}

GameMenu::~GameMenu()
{
}

void GameMenu::Start()
{
	XMLFile* style = main_->cache_->GetResource<XMLFile>("UI/DefaultStyle.xml");
	main_->ui_->GetRoot()->SetDefaultStyle(style);

	gameMenu_ = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/gameMenu.xml"));
	main_->ui_->GetRoot()->AddChild(gameMenu_);
	main_->RecursiveAddGuiTargets(gameMenu_);
	main_->ElementRecursiveResize(gameMenu_);
	gameMenu_->SetEnabled(false);
	gameMenu_->SetVisible(false);

	mainMenuButt_ = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/mainMenuButt.xml"));
	main_->ui_->GetRoot()->AddChild(mainMenuButt_);
	main_->RecursiveAddGuiTargets(mainMenuButt_);
	main_->ElementRecursiveResize(mainMenuButt_);
	mainMenuButt_->SetEnabled(false);
	mainMenuButt_->SetVisible(false);

	serverName_ = gameMenu_->GetChild("serverName", true);
	gameMode_ = gameMenu_->GetChild("gameMode", true);
	address_ = gameMenu_->GetChild("address", true);
	ipLineEdit_ = gameMenu_->GetChild("IPLineEdit", true);

	SubscribeToEvent(serverName_, E_ITEMSELECTED, HANDLER(GameMenu, HandleItemSelected));
	SubscribeToEvent(gameMode_, E_ITEMSELECTED, HANDLER(GameMenu, HandleItemSelected));
	SubscribeToEvent(address_, E_ITEMSELECTED, HANDLER(GameMenu, HandleItemSelected));
	SubscribeToEvent(serverName_, E_ITEMDESELECTED, HANDLER(GameMenu, HandleItemDeselected));
	SubscribeToEvent(gameMode_, E_ITEMDESELECTED, HANDLER(GameMenu, HandleItemDeselected));
	SubscribeToEvent(address_, E_ITEMDESELECTED, HANDLER(GameMenu, HandleItemDeselected));
	SubscribeToEvent(gameMenu_->GetChild("host", true), E_RELEASED, HANDLER(GameMenu, HandleButtonRelease));
	SubscribeToEvent(gameMenu_->GetChild("list", true), E_RELEASED, HANDLER(GameMenu, HandleButtonRelease));
	SubscribeToEvent(gameMenu_->GetChild("join", true), E_RELEASED, HANDLER(GameMenu, HandleButtonRelease));
	SubscribeToEvent(mainMenuButt_, E_RELEASED, HANDLER(GameMenu, HandleButtonRelease));
	SubscribeToEvent(ipLineEdit_, E_TEXTFINISHED, HANDLER(GameMenu, HandleTextFinished));

	SubscribeToEvent(E_GAMEMENUDISPLAY, HANDLER(GameMenu, HandleDisplayMenu));
	SubscribeToEvent(E_SERVERCONNECTED, HANDLER(GameMenu, HandleServerConnect));
	SubscribeToEvent(E_NETWORKMESSAGE, HANDLER(GameMenu, HandleNetworkMessage));

	//gameMenu_->GetChild("host", true)->SetEnabledRecursive(false);
	//gameMenu_->GetChild("host", true)->SetVisible(false);
}

void GameMenu::HandleItemSelected(StringHash eventType, VariantMap& eventData)
{
	using namespace ItemSelected;

	UIElement* list = static_cast<UIElement*>(eventData[P_ELEMENT].GetPtr());
	int index = eventData[P_SELECTION].GetInt();

	UIElement* item = ((ListView*)list)->GetItem(index);
	((Text*)item)->SetColor(Color(0.0f, 0.5f, 0.5f, 1.0f));

	if (((ListView*)(serverName_))->GetSelection() != index)
	{
		((ListView*)(serverName_))->SetSelection(index);
	}
	else if (((ListView*)(gameMode_))->GetSelection() != index)
	{
		((ListView*)(gameMode_))->SetSelection(index);
	}
	else if (((ListView*)(address_))->GetSelection() != index)
	{
		((ListView*)(address_))->SetSelection(index);
	}
}

void GameMenu::HandleItemDeselected(StringHash eventType, VariantMap& eventData)
{
	using namespace ItemDeselected;

	UIElement* list = static_cast<UIElement*>(eventData[P_ELEMENT].GetPtr());
	int index = eventData[P_SELECTION].GetInt();

	UIElement* item = ((ListView*)list)->GetItem(index);
	((Text*)item)->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
}

void GameMenu::HandleButtonRelease(StringHash eventType, VariantMap& eventData)
{
	using namespace Released;

	UIElement* ele = static_cast<UIElement*>(eventData[Released::P_ELEMENT].GetPtr());

	network_->Disconnect();

	if (ele->GetName() == "host")
	{
		VariantMap vm;
		vm[GameMenuDisplay::P_STATE] = false;
		SendEvent(E_GAMEMENUDISPLAY, vm);

		if (main_->myRootNode_->HasComponent<Client>())
		{
			main_->myRootNode_->RemoveComponent(
					main_->myRootNode_->GetComponent<Client>());
		}

		if (main_->myRootNode_->HasComponent<Server>())
		{
			main_->myRootNode_->RemoveComponent(
					main_->myRootNode_->GetComponent<Server>());
		}

		main_->myRootNode_->AddComponent(new Server(context_, main_), 0, LOCAL);
	}
	else if (ele->GetName() == "list")
	{
		QueryMasterServer();
	}
	else if (ele->GetName() == "join")
	{
		VariantMap vm;
		vm[GameMenuDisplay::P_STATE] = false;
		SendEvent(E_GAMEMENUDISPLAY, vm);

		if (main_->myRootNode_->HasComponent<Server>())
		{
			main_->myRootNode_->RemoveComponent(
					main_->myRootNode_->GetComponent<Server>());
		}

		if (!main_->myRootNode_->HasComponent<Client>())
		{
			main_->myRootNode_->AddComponent(new Client(context_, main_), 0, LOCAL);
		}

		int index = ((ListView*)serverName_)->GetSelection();

		if (index < 0)
		{
			network_->Connect(ipAddress_, 9002, 0);
			return;
		}

		UIElement* item = ((ListView*)address_)->GetItem(index);
		String address = ((Text*)item)->GetText();

		network_->Connect(address, 9002, 0);
	}
	else if (ele->GetName() == "mainMenuButt")
	{
		VariantMap vm;
		vm[GameMenuDisplay::P_STATE] = true;
		SendEvent(E_GAMEMENUDISPLAY, vm);
	}
}

void GameMenu::QueryMasterServer()
{
	if (!masterServerConnected_)
	{
		XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>("Objects/serverInfo.xml");
		Node* serverInfo = main_->scene_->InstantiateXML(xmlFile->GetRoot(), Vector3::ZERO, Quaternion(), LOCAL);

		masterServerIP_ = serverInfo->GetVar("masterServerIP").GetString();

		if (masterServerIP_ == "127.0.0.1")
		{
			return;
		}

		masterServerConnected_ = network_->Connect(masterServerIP_, 9001, 0);
	}
}

void GameMenu::HandleServerConnect(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("server connected to server");
	if (masterServerConnected_)
	{
		((ListView*)serverName_)->RemoveAllItems();
		((ListView*)gameMode_)->RemoveAllItems();
		((ListView*)address_)->RemoveAllItems();

		for (int x = 0; x < servers_.Size(); x++)
		{
			delete servers_[x];
		}

		servers_.Clear();

		msg_.Clear();
		network_->GetServerConnection()->SendMessage(MSG_GETSERVERS, true, true, msg_);
		return;
	}
}

void GameMenu::HandleNetworkMessage(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("network message");
	using namespace NetworkMessage;

	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	int msgID = eventData[P_MESSAGEID].GetInt();

	if (msgID == MSG_SERVERINFO)
	{
		const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
		MemoryBuffer msg(data);
		String serverName = msg.ReadString();
		String gameMode = msg.ReadString();
		String address = msg.ReadString();

		ServerInfo* si = new ServerInfo();
		si->serverName_ = serverName;
		si->gameMode_ = gameMode;
		si->address_ = address;

		servers_.Push(si);

		SharedPtr<UIElement> serverNameT = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/serverInfo.xml"));
		((Text*)((UIElement*)serverNameT))->SetText(serverName.CString());

		SharedPtr<UIElement> gameModeT = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/serverInfo.xml"));
		((Text*)((UIElement*)gameModeT))->SetText(gameMode.CString());

		SharedPtr<UIElement> addressT = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/serverInfo.xml"));
		((Text*)((UIElement*)addressT))->SetText(address.CString());

		((ListView*)serverName_)->AddItem(serverNameT);
		((ListView*)gameMode_)->AddItem(gameModeT);
		((ListView*)address_)->AddItem(addressT);
	}
	else if (msgID == MSG_SERVERSSENT)
	{
		network_->GetServerConnection()->Disconnect();
		masterServerConnected_ = false;
	}
}

void GameMenu::HandleDisplayMenu(StringHash eventType, VariantMap& eventData)
{
	bool state = eventData[GameMenuDisplay::P_STATE].GetBool();

	if (state)
	{
		gameMenu_->SetEnabled(true);
		gameMenu_->SetVisible(true);
		mainMenuButt_->SetEnabled(false);
		mainMenuButt_->SetVisible(false);

		LoadScene();
	}
	else
	{
		gameMenu_->SetEnabled(false);
		gameMenu_->SetVisible(false);
		mainMenuButt_->SetEnabled(true);
		mainMenuButt_->SetVisible(true);

		UnloadScene();
	}
}

void GameMenu::LoadScene()
{
	scene_ = new Scene(context_);

	File loadFile(context_,main_->filesystem_->GetProgramDir()
			+ "Data/Scenes/networkMenu.xml", FILE_READ);
	scene_->LoadXML(loadFile);

	cameraNode_ = new Node(context_);
	cameraNode_ = scene_->GetChild("camera");

	main_->viewport_->SetScene(scene_);
	main_->viewport_->SetCamera(cameraNode_->GetComponent<Camera>());
	main_->renderer_->SetViewport(0, main_->viewport_);
}

void GameMenu::UnloadScene()
{
	scene_->RemoveAllChildren();
	//scene_->RemoveAllComponents();
	scene_->Remove();
}

void GameMenu::HandleTextFinished(StringHash eventType, VariantMap& eventData)
{
	using namespace TextFinished;

	ipAddress_ = eventData[P_TEXT].GetString();

	((ListView*)serverName_)->ClearSelection();
	((ListView*)gameMode_)->ClearSelection();
	((ListView*)address_)->ClearSelection();
}
