/*
 * GameMenu.h
 *
 *  Created on: Jul 6, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>

#include "Urho3DPlayer.h"
#include "Constants.h"
#include "network/ServerInfo.h"

using namespace Urho3D;

class GameMenu : public LogicComponent
{
	URHO3D_OBJECT(GameMenu, LogicComponent);
public:
	GameMenu(Context* context, Urho3DPlayer* main);
	~GameMenu();

	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	virtual void Start();
	void HandleServerConnect(StringHash eventType, VariantMap& eventData);
    void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
    void HandleItemSelected(StringHash eventType, VariantMap& eventData);
    void HandleItemDeselected(StringHash eventType, VariantMap& eventData);
	void HandleButtonRelease(StringHash eventType, VariantMap& eventData);
	void HandleDisplayMenu(StringHash eventType, VariantMap& eventData);
	void HandleTextFinished(StringHash eventType, VariantMap& eventData);
	void QueryMasterServer();
	void LoadScene();
	void UnloadScene();

	Urho3DPlayer* main_;
	float elapsedTime_;

	Network* network_;
	VectorBuffer msg_;

	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;

	SharedPtr<UIElement> gameMenu_;
	SharedPtr<UIElement> mainMenuButt_;
	SharedPtr<UIElement> ipLineEdit_;
	UIElement* serverName_;
	UIElement* gameMode_;
	UIElement* address_;
	Vector<ServerInfo*> servers_;

	bool masterServerConnected_;
	String masterServerIP_;
	String ipAddress_;
};
