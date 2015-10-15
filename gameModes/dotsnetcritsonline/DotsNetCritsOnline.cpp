/*
 * DotsNetCrits.cpp
 *
 *  Created on: Jul 6, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundListener.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Audio/SoundSource3D.h>
#include <Urho3D/Physics/PhysicsWorld.h>

#include "DotsNetCritsOnline.h"
#include "../../network/NetworkConstants.h"
#include "../../Constants.h"

#include "logicComponents/ModelController.h"
#include "logicComponents/ThirdPersonCamera.h"
#include "logicComponents/Speed.h"
#include "logicComponents/Gravity.h"
#include "logicComponents/RotateTo.h"
#include "logicComponents/MoveByTouch.h"

DotsNetCritsOnline::DotsNetCritsOnline(Context* context, Urho3DPlayer* main, bool isServer) :
	LogicComponent(context)
{
	main_ = main;
	network_ = GetSubsystem<Network>();
	isServer_ = isServer;

	//SubscribeToEvent(E_POSTRENDERUPDATE, HANDLER(DotsNetCritsOnline, HandlePostRenderUpdate));
	SubscribeToEvent(E_GETISSERVER, HANDLER(DotsNetCritsOnline, HandleGetIsServer));
}

DotsNetCritsOnline::~DotsNetCritsOnline()
{
	scene_->RemoveAllChildren();
	//scene_->RemoveAllComponents();
	scene_->Remove();
	main_->ClearRootNodes();
}

void DotsNetCritsOnline::Start()
{
	main_->ClearRootNodes();

	scene_ = new Scene(context_);

	File loadFile(context_,main_->filesystem_->GetProgramDir()
			+ "Data/Scenes/cyberpunk.xml", FILE_READ);
	scene_->LoadXML(loadFile);

	main_->mySceneNode_ = SharedPtr<Node>( new Node(context_) );
	main_->sceneNodes_.Push(main_->mySceneNode_);

	for (int x = 0; x < main_->sceneNodes_.Size(); x++)
	{//LOGERRORF("adding sceneNode to scene_. clientid %d",main_->GetClientID(main_->mySceneNode_));
		scene_->AddChild(main_->sceneNodes_[x]);
	}

	cameraNode_ = new Node(context_);
	cameraNode_ = scene_->GetChild("camera");

	Node* spawns = scene_->GetChild("spawns");

	for (int x = 0; x < spawns->GetNumChildren(false); x++)
	{
		spawnPoints_.Push( SharedPtr<Node>(spawns->GetChild(x) ) );
	}

	if (!main_->engine_->IsHeadless())
	{
		main_->viewport_->SetScene(scene_);
		main_->viewport_->SetCamera(cameraNode_->GetComponent<Camera>());
		main_->renderer_->SetViewport(0, main_->viewport_);

		if (GetPlatform() == "Android")
		{
			//main_->renderer_->SetReuseShadowMaps(false);
			//main_->renderer_->SetShadowQuality(SHADOWQUALITY_LOW_16BIT);
			//main_->renderer_->SetMobileShadowBiasMul(2.0f);
			main_->renderer_->SetMobileShadowBiasAdd(0.001);
		}

		//main_->mySceneNode_->AddComponent(new MechanicsHud(context_, main_), 0, LOCAL);

	}
	else
	{
		//scene_->GetChild("bgm")->GetComponent<SoundSource>()->Stop();
	}

	if (main_->network_->IsServerRunning())
	{//LOGERRORF("spawning server player");
		RespawnNode(main_->mySceneNode_, -1);
		AttachLogicComponents(main_->mySceneNode_);
	}

	SubscribeToEvent(E_NETWORKMESSAGE, HANDLER(DotsNetCritsOnline, HandleNetworkMessage));
	SubscribeToEvent(E_GAMEMENUDISPLAY, HANDLER(DotsNetCritsOnline, HandleDisplayMenu));
	SubscribeToEvent(E_NEWCLIENTID, HANDLER(DotsNetCritsOnline, HandleNewClientID));
	SubscribeToEvent(E_CLIENTHEALTHSET, HANDLER(DotsNetCritsOnline, HandleClientHealthSet));
}

void DotsNetCritsOnline::HandleNetworkMessage(StringHash eventType, VariantMap& eventData)
{
	using namespace NetworkMessage;

	int msgID = eventData[P_MESSAGEID].GetInt();

    if (msgID == MSG_GAMEMODEMSG)
    {
    	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

        const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
        MemoryBuffer msg(data);

        int gmMSG = msg.ReadInt();

        if (gmMSG == GAMEMODEMSG_RESPAWNNODE)
        {
        	int clientID = msg.ReadInt();
        	int index = msg.ReadInt();
        	bool newClient = msg.ReadBool();

        	Node* sceneNode = main_->GetSceneNode(clientID);
//LOGERRORF("got respawn for new clientID %d", clientID);
        	if (sceneNode != NULL)
        	{
        		RespawnNode(SharedPtr<Node>(sceneNode), index);

        		if (newClient)
        		{//LOGERRORF("respawned new clientID %d myclientd: %d", clientID, main_->GetClientID(main_->mySceneNode_));
        			if (main_->GetClientID(main_->mySceneNode_) == clientID)
        			{
        				//LOGERRORF("attaching lc to clientid %d",clientID);
        				AttachLogicComponents(SharedPtr<Node>(sceneNode));
        				msg_.Clear();
            			msg_.WriteInt(GAMEMODEMSG_GETLC);
            			msg_.WriteInt(clientID);
            			network_->GetServerConnection()->SendMessage(MSG_GAMEMODEMSG, true, true, msg_);
        			}
        		}
        	}
        }
        else if (gmMSG == GAMEMODEMSG_GETLC)
        {
        	int clientID = msg.ReadInt();
        	SharedPtr<Node> sceneNode = SharedPtr<Node>( main_->GetSceneNode(clientID) );

        	VariantMap vm;
        	vm[GetLc::P_NODE] = sceneNode;
        	vm[GetLc::P_CONNECTION] = sender;
        	SendEvent(E_GETLC, vm);
        }
    }
}

void DotsNetCritsOnline::HandleDisplayMenu(StringHash eventType, VariantMap& eventData)
{
	bool state = eventData[GameMenuDisplay::P_STATE].GetBool();

	if (state)
	{
		VariantMap vm;
		SendEvent(E_GAMEMODEREMOVED, vm);

		Remove();//buhbye.
	}
}

void DotsNetCritsOnline::HandleNewClientID(StringHash eventType, VariantMap& eventData)
{
	int clientID = eventData[NewClientID::P_CLIENTID].GetInt();

	SharedPtr<Node> sceneNode = SharedPtr<Node>( main_->GetSceneNode(clientID) );
//LOGERRORF("handling new clientID %d", clientID);
	if (sceneNode != NULL)
	{
		if (sceneNode->GetScene() != scene_)
		{//LOGERRORF("added new clientID sceneNode to scene %d", clientID);
		scene_->AddChild(sceneNode);
		}

		if (isServer_)
		{//LOGERRORF("telling new clientID %d to respawn", clientID);
			int index = Random( 0, spawnPoints_.Size() );
			RespawnNode(sceneNode, index);

			AttachLogicComponents(sceneNode);

			msg_.Clear();
			msg_.WriteInt(GAMEMODEMSG_RESPAWNNODE);
			msg_.WriteInt(clientID);
			msg_.WriteInt(index);
			msg_.WriteBool(true);//new client
			network_->BroadcastMessage(MSG_GAMEMODEMSG, true, true, msg_);
		}
		else
		{
			int index = Random( 0, spawnPoints_.Size() );
			RespawnNode(sceneNode, index);
			AttachLogicComponents(sceneNode);
			//LOGERRORF("attaching lc to clientid %d",clientID);
			/*msg_.Clear();
			msg_.WriteInt(GAMEMODEMSG_GETLC);
			msg_.WriteInt(clientID);
			network_->GetServerConnection()->SendMessage(MSG_GAMEMODEMSG, true, true, msg_);*/
		}
	}
}

void DotsNetCritsOnline::RespawnNode(SharedPtr<Node> sceneNode, int index)
{
	if (spawnPoints_.Size() == 0)
	{
		return;
	}

	if (index == -1)
	{
		index = Random( 0, spawnPoints_.Size() );
	}

	Vector3 victoria = spawnPoints_[index]->GetPosition();
	Quaternion quarterOnion = spawnPoints_[index]->GetRotation();

	sceneNode->SetPosition(victoria);
	sceneNode->SetRotation(quarterOnion);

	VariantMap vm;
	vm[RespawnSceneNode::P_NODE] = sceneNode;
	vm[RespawnSceneNode::P_POSITION] = victoria;
	vm[RespawnSceneNode::P_ROTATION] = quarterOnion;
	SendEvent(E_RESPAWNSCENENODE, vm);
}

void DotsNetCritsOnline::AttachLogicComponents(SharedPtr<Node> sceneNode)
{
	sceneNode->AddComponent(new ModelController(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new ThirdPersonCamera(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new Speed(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new Gravity(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new RotateTo(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new MoveByTouch(context_, main_), 0, LOCAL);
}

void DotsNetCritsOnline::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
	if (!GetSubsystem<Renderer>())
	{
		return;
	}

	GetSubsystem<Renderer>()->DrawDebugGeometry(true);
	if (scene_)
	{
		if (scene_->GetComponent<PhysicsWorld>())
		{
			scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
		}
	}
}

void DotsNetCritsOnline::HandleGetIsServer(StringHash eventType, VariantMap& eventData)
{
	VariantMap vm;
	vm[SetIsServer::P_ISSERVER] = isServer_;
	SendEvent(E_SETISSERVER, vm);
}

void DotsNetCritsOnline::HandleClientHealthSet(StringHash eventType, VariantMap& eventData)
{
	if (!isServer_)
	{
		return;
	}

	targetSceneNode_ = (Node*)(eventData[ModifyClientHealth::P_NODE].GetPtr());
	int health = eventData[ClientHealthSet::P_HEALTH].GetInt();

	if (health <= 0)
	{
		SubscribeToEvent(E_SETSCENENODECLIENTID, HANDLER(DotsNetCritsOnline, HandleSetSceneNodeClientID));

		VariantMap vm0;
		vm0[GetSceneNodeClientID::P_NODE] = targetSceneNode_;
		SendEvent(E_GETSCENENODECLIENTID, vm0);

		UnsubscribeFromEvent(E_SETSCENENODECLIENTID);

		int index = Random( 0, spawnPoints_.Size() );
		RespawnNode(SharedPtr<Node>(targetSceneNode_), index);

		msg_.Clear();
		msg_.WriteInt(GAMEMODEMSG_RESPAWNNODE);
		msg_.WriteInt(targetSceneNodeClientID_);
		msg_.WriteInt(index);
		msg_.WriteBool(false);//old client
		network_->BroadcastMessage(MSG_GAMEMODEMSG, true, true, msg_);

		VariantMap vm1;
		vm1[ModifyClientHealth::P_NODE] = targetSceneNode_;
		vm1[ModifyClientHealth::P_HEALTH] = 100;
		vm1[ModifyClientHealth::P_OPERATION] = 0;
		vm1[ModifyClientHealth::P_SENDTOSERVER] = false;
		SendEvent(E_MODIFYCLIENTHEALTH, vm1);

		msg_.Clear();//todo better way of doing this stuff
		msg_.WriteInt(targetSceneNodeClientID_);
		msg_.WriteString("Health");
		msg_.WriteInt(100);
		msg_.WriteInt(0);
		network_->BroadcastMessage(MSG_LCMSG, true, true, msg_);
	}
}

void DotsNetCritsOnline::HandleSetSceneNodeClientID(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetSceneNodeClientID::P_NODE].GetPtr());

	if (sceneNode == targetSceneNode_)
	{
		targetSceneNodeClientID_ = eventData[SetSceneNodeClientID::P_CLIENTID].GetInt();
	}
}
