//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Urho3D.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/LuaScript/LuaScript.h>
#include <Urho3D/Core/Main.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "Urho3DPlayer.h"
#include "network/MasterServer.h"
#include "network/Server.h"
#include "network/NetworkConstants.h"
#include "network/ClientInfo.h"
#include "network/NetPulse.h"
#include "network/Client.h"
#include "Constants.h"
#include "GameMenu.h"

#include "gameModes/dotsnetcritsonline/logicComponents/NodeInfo.h"//todo change to a higher directory

URHO3D_DEFINE_APPLICATION_MAIN(Urho3DPlayer);

Urho3DPlayer::Urho3DPlayer(Context* context) :
    		Application(context)
{
}

void Urho3DPlayer::Setup()
{
	engineParameters_["WindowWidth"] = 800;
	engineParameters_["WindowHeight"] = 480;
	engineParameters_["WindowResizable"] = true;
	engineParameters_["WindowTitle"] = "Dots Net Crits Online";
	engineParameters_["FullScreen"] = false;
	engineParameters_["VSync"] = true;
}

void Urho3DPlayer::Start()
{
	SetRandomSeed(GetSubsystem<Time>()->GetTimeSinceEpoch());

	context_->RegisterSubsystem(new LuaScript(context_));

	input_ = GetSubsystem<Input>();
	input_->SetMouseVisible(true);
	input_->SetTouchEmulation(true);
	graphics_ = GetSubsystem<Graphics>();
	cache_ = GetSubsystem<ResourceCache>();
	filesystem_ = GetSubsystem<FileSystem>();
	renderer_ = GetSubsystem<Renderer>();
	network_ = GetSubsystem<Network>();
	ui_ = GetSubsystem<UI>();
	engine_ = GetSubsystem<Engine>();
	audio_ = GetSubsystem<Audio>();
	viewport_ = new Viewport(context_);

	scene_ = new Scene(context_);

	XMLFile* xmlFile = cache_->GetResource<XMLFile>("Objects/serverType.xml");
	Node* serverType = scene_->InstantiateXML(xmlFile->GetRoot(), Vector3::ZERO, Quaternion(), LOCAL);

	int serverTypeValue = serverType->GetVar("serverType").GetInt();

	myRootNode_ = SharedPtr<Node>( new Node(context_) );

	rootNodes_.Push(myRootNode_);

	scene_->AddChild(myRootNode_);

	if (serverTypeValue == 0)//Master Server
	{
		myRootNode_->AddComponent(new MasterServer(context_, this), 0, LOCAL);
	}
	else if (serverTypeValue == 1)//Server
	{
		if (!engine_->IsHeadless())
		{
			SubscribeToEvent(E_RESIZED, URHO3D_HANDLER(Urho3DPlayer, HandleElementResize));
			myRootNode_->AddComponent(new GameMenu(context_, this), 0, LOCAL);

			VariantMap vm;
			vm[GameMenuDisplay::P_STATE] = true;
			SendEvent(E_GAMEMENUDISPLAY, vm);
		}

		myRootNode_->AddComponent(new Server(context_, this), 0, LOCAL);
	}
	else//Client
	{
		SubscribeToEvent(E_RESIZED, URHO3D_HANDLER(Urho3DPlayer, HandleElementResize));

		myRootNode_->AddComponent(new GameMenu(context_, this), 0, LOCAL);

		VariantMap vm;
		vm[GameMenuDisplay::P_STATE] = true;
		SendEvent(E_GAMEMENUDISPLAY, vm);

		myRootNode_->AddComponent(new Client(context_, this), 0, LOCAL);
	}

	scene_->RemoveChild(serverType);

	SubscribeToEvents();
}

void Urho3DPlayer::Stop()
{
	//audio_->Stop();
}

void Urho3DPlayer::SubscribeToEvents()
{
	// Subscribe HandleUpdate() function for processing update events
	//SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Urho3DPlayer, HandleUpdate));

	//SubscribeToEvent(E_NETWORKMESSAGE, URHO3D_HANDLER(Urho3DPlayer, HandleNetworkMessage));
	SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Urho3DPlayer, HandleKeyDown));
}

void Urho3DPlayer::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;

	timeStep_ = eventData[P_TIMESTEP].GetFloat();

}

void Urho3DPlayer::HandleElementResize(StringHash eventType, VariantMap& eventData)
{
	using namespace Resized;

	UIElement* ele = static_cast<UIElement*>(eventData[ElementAdded::P_ELEMENT].GetPtr());

	ElementRecursiveResize(ele);
}

void Urho3DPlayer::ElementRecursiveResize(UIElement* ele)
{
	Vector3 targetRes, targetSize, targetPos;

	targetRes = ele->GetVar("targetRes").GetVector3();
	targetSize = ele->GetVar("targetSize").GetVector3();
	targetPos = ele->GetVar("targetPos").GetVector3();

	if (targetRes != Vector3::ZERO)
	{

		IntVector2 rootExtent;

		rootExtent.x_ = graphics_->GetWidth();
		rootExtent.y_ = graphics_->GetHeight();

		IntVector2 scaledExtent;

		scaledExtent.x_ = ( targetSize.x_ *  rootExtent.x_ ) / targetRes.x_;
		scaledExtent.y_ = ( targetSize.y_ *  rootExtent.y_ ) / targetRes.y_;

		ele->SetSize(scaledExtent);

		IntVector2 scaledPosition = IntVector2(
				( targetPos.x_ *  rootExtent.x_ ) / targetRes.x_,
				( targetPos.y_ *  rootExtent.y_ ) / targetRes.y_);

		ele->SetPosition(scaledPosition);

	}

	for (int x = 0; x < ele->GetNumChildren(); x++)
	{
		ElementRecursiveResize(ele->GetChild(x));
	}

}

void Urho3DPlayer::HandleNetworkMessage(StringHash eventType, VariantMap& eventData)
{
	//LOGERRORF("network message");
	using namespace NetworkMessage;

	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

	int msgID = eventData[P_MESSAGEID].GetInt();

}

Node* Urho3DPlayer::GetRootNode(Connection* conn)
{
	for (int x = 0; x < rootNodes_.Size(); x++)
	{
		if (rootNodes_[x]->HasComponent<ClientInfo>())
		{
			if (rootNodes_[x]->GetComponent<ClientInfo>()->connection_ == conn)
			{
				return rootNodes_[x];
			}
		}
	}
	return NULL;
}

Node* Urho3DPlayer::GetRootNode(int clientID)
{
	for (int x = 0; x < rootNodes_.Size(); x++)
	{
		if (rootNodes_[x]->HasComponent<ClientInfo>())
		{
			if (rootNodes_[x]->GetComponent<ClientInfo>()->clientID_ == clientID)
			{
				return rootNodes_[x];
			}
		}
	}
	return NULL;
}

void Urho3DPlayer::RemoveRootNode(Node* rootNode)
{
	int index = -1;

	for (int x = 0; x < rootNodes_.Size(); x++)
	{
		if (rootNodes_[x] == rootNode)
		{
			index = x;
			break;
		}
	}

	rootNodes_.Remove(SharedPtr<Node>(rootNode));
	rootNode->RemoveAllChildren();
	rootNode->RemoveAllComponents();
	rootNode->Remove();

	SharedPtr<Node> sceneNode = sceneNodes_[index];
	sceneNodes_.Remove(sceneNode);
	sceneNode->RemoveAllChildren();
	sceneNode->RemoveAllComponents();
	sceneNode->Remove();
}

void Urho3DPlayer::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;

	int key = eventData[P_KEY].GetInt();

	if (key == SDLK_ESCAPE)
	{
		engine_->Exit();
	}
}

Node* Urho3DPlayer::GetSceneNode(int clientID)
{
	for (int x = 0; x < rootNodes_.Size(); x++)
	{
		if (rootNodes_[x]->HasComponent<ClientInfo>())
		{
			if (rootNodes_[x]->GetComponent<ClientInfo>()->clientID_ == clientID)
			{
				if (x < sceneNodes_.Size())
				{
					return sceneNodes_[x];
				}
				else
				{
					break;
				}
			}
		}
	}
	return NULL;
}

Node* Urho3DPlayer::GetRootNode(Node* sceneNode)
{
	for (int x = 0; x < sceneNodes_.Size(); x++)
	{
		if (sceneNodes_[x] == sceneNode)
		{
			return rootNodes_[x];
		}
	}
	return NULL;
}

Node* Urho3DPlayer::GetSceneNode(Node* rootNode)
{
	for (int x = 0; x < rootNodes_.Size(); x++)
	{
		if (rootNodes_[x] == rootNode)
		{
			return sceneNodes_[x];
		}
	}
	return NULL;
}

bool Urho3DPlayer::IsLocalClient(Node* sceneNode)
{
	if (mySceneNode_ == sceneNode)
	{
		return true;
	}
	return false;
}

void Urho3DPlayer::RecursiveAddGuiTargets(UIElement* ele)
{
	Vector3 targetRes, targetSize, targetPos;
	IntVector2 v2;

	targetRes = Vector3(800, 480, 0);
	ele->SetVar("targetRes", targetRes);

	v2 = ele->GetSize();

	if (v2 == IntVector2::ZERO)
	{
		v2 = IntVector2(800, 480);
	}

	targetSize = Vector3(v2.x_, v2.y_, 0.0f);
	ele->SetVar("targetSize", targetSize);

	v2 = ele->GetPosition();
	targetPos = Vector3(v2.x_, v2.y_, 0.0f);
	ele->SetVar("targetPos", targetPos);

	for (int x = 0; x < ele->GetNumChildren(); x++)
	{
		RecursiveAddGuiTargets(ele->GetChild(x));
	}
}

void Urho3DPlayer::ClearRootNodes()
{
	ClearSceneNodes();

	SharedPtr<Node> myRootNode = myRootNode_;

	for (int x = 0; x < rootNodes_.Size(); x++)
	{
		if (rootNodes_[x] != myRootNode_)
		{
			rootNodes_[x]->RemoveAllChildren();
			rootNodes_[x]->RemoveAllComponents();
			rootNodes_[x]->Remove();
		}

	}
	rootNodes_.Clear();

	rootNodes_.Push(myRootNode);
}

void Urho3DPlayer::ClearSceneNodes()
{
	for (int x = 0; x < sceneNodes_.Size(); x++)
	{
		sceneNodes_[x]->RemoveAllChildren();
		sceneNodes_[x]->RemoveAllComponents();
		sceneNodes_[x]->Remove();
	}
	sceneNodes_.Clear();
	identifiedNodes_.Clear();
}

int Urho3DPlayer::GetClientID(Node* sceneNode)
{
	for (int x = 0; x < sceneNodes_.Size(); x++)
	{
		if (sceneNodes_[x] == sceneNode)
		{
			return rootNodes_[x]->GetComponent<ClientInfo>()->clientID_;
		}
	}
	return -1;
}

Connection* Urho3DPlayer::GetConn(Node* sceneNode)
{
	for (int x = 0; x < sceneNodes_.Size(); x++)
	{
		if (sceneNodes_[x] == sceneNode)
		{
			return rootNodes_[x]->GetComponent<ClientInfo>()->connection_;
		}
	}
	return NULL;
}

Connection* Urho3DPlayer::GetConnByClientID(int clientID)
{
	for (int x = 0; x < rootNodes_.Size(); x++)
	{
		if (rootNodes_[x]->GetComponent<ClientInfo>()->clientID_ == clientID)
		{
			return rootNodes_[x]->GetComponent<ClientInfo>()->connection_;
		}
	}
	return NULL;
}

Node* Urho3DPlayer::GetIdentifiedNode(String lc, int clientID, int nodeID)
{
	for (int x = 0; x < identifiedNodes_.Size(); x++)
	{
		NodeInfo* ni = identifiedNodes_[x]->GetComponent<NodeInfo>();

		if (ni->LC_ == lc)
		{
			if (ni->clientID_ == clientID)
			{
				if (ni->nodeID_ == nodeID)
				{
					return identifiedNodes_[x];
				}
			}
		}
	}
	return NULL;
}
