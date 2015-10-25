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
#include "../../network/ClientInfo.h"
#include "../../Constants.h"

#include "logicComponents/NodeInfo.h"
#include "logicComponents/ModelController.h"
#include "logicComponents/ThirdPersonCamera.h"
#include "logicComponents/Speed.h"
#include "logicComponents/Gravity.h"
#include "logicComponents/RotateTo.h"
#include "logicComponents/MoveByTouch.h"
#include "logicComponents/ChickenNPC.h"
#include "logicComponents/SceneVoter.h"

DotsNetCritsOnline::DotsNetCritsOnline(Context* context, Urho3DPlayer* main, bool isServer, String defaultScene) :
	LogicComponent(context)
{
	main_ = main;
	network_ = GetSubsystem<Network>();
	isServer_ = isServer;

	nodeIDCounter_ = 0;

	lagTime_ = 0.0f;

	sceneVoteCount_ = 0;

	sceneFileName_ = defaultScene;

	GAMEMODEMSG_SPAWNCHICKEN = 0;
	GAMEMODEMSG_LOADSCENE = 1;

	//SubscribeToEvent(E_POSTRENDERUPDATE, HANDLER(DotsNetCritsOnline, HandlePostRenderUpdate));
	SubscribeToEvent(E_GETISSERVER, HANDLER(DotsNetCritsOnline, HandleGetIsServer));
}

DotsNetCritsOnline::~DotsNetCritsOnline()
{
	main_->ClearRootNodes();
	scene_->RemoveAllChildren();
	//scene_->RemoveAllComponents();
	scene_->Remove();
}

void DotsNetCritsOnline::Start()
{
	main_->ClearRootNodes();

	LoadScene(sceneFileName_);

	SubscribeToEvent(E_NETWORKMESSAGE, HANDLER(DotsNetCritsOnline, HandleNetworkMessage));
	SubscribeToEvent(E_GAMEMENUDISPLAY, HANDLER(DotsNetCritsOnline, HandleDisplayMenu));
	SubscribeToEvent(E_NEWCLIENTID, HANDLER(DotsNetCritsOnline, HandleNewClientID));
	SubscribeToEvent(E_CLIENTHEALTHSET, HANDLER(DotsNetCritsOnline, HandleClientHealthSet));
	SubscribeToEvent(E_LCMSG, HANDLER(DotsNetCritsOnline, HandleLCMSG));
	SubscribeToEvent(E_GETLC, HANDLER(DotsNetCritsOnline, HandleGetLc));
	SubscribeToEvent(E_GETSCENENAME, HANDLER(DotsNetCritsOnline, HandleGetSceneName));
	SubscribeToEvent(E_SETSCENEVOTE, HANDLER(DotsNetCritsOnline, HandleSetSceneVote));
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
        	int nodeID = msg.ReadInt();
        	int index = msg.ReadInt();
        	bool newClient = msg.ReadBool();

        	Node* sceneNode = main_->GetSceneNode(clientID);

        	if (sceneNode != NULL)
        	{
        		RespawnNode(SharedPtr<Node>(sceneNode), index);

        		if (newClient)
        		{
    				AttachLogicComponents(SharedPtr<Node>(sceneNode), nodeID);

        			if (main_->GetClientID(main_->mySceneNode_) == clientID)
        			{
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
        else if (gmMSG == GAMEMODEMSG_SCENEVOTE)
        {
        	int clientID = msg.ReadInt();
        	SharedPtr<Node> sceneNode = SharedPtr<Node>( main_->GetSceneNode(clientID) );

        	String selectedFilename = msg.ReadString();

        	VariantMap vm;
			vm[SetSceneVote::P_SCENENODE] = sceneNode;
			vm[SetSceneVote::P_SCENENAME] = selectedFilename;
			SendEvent(E_SETSCENEVOTE, vm);
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

	if (sceneNode != NULL)
	{
		if (sceneNode->GetScene() != scene_)
		{
			scene_->AddChild(sceneNode);
		}

		if (isServer_)
		{
			Connection* conn = main_->GetConn(sceneNode);

			for (int x = 0; x < main_->sceneNodes_.Size(); x++)
			{
				SharedPtr<Node> oldSceneNode = main_->sceneNodes_[x];

				if (oldSceneNode == sceneNode)
				{
					continue;
				}

				msg_.Clear();
				msg_.WriteInt(GAMEMODEMSG_RESPAWNNODE);
				msg_.WriteInt(oldSceneNode->GetComponent<NodeInfo>()->clientID_);
				msg_.WriteInt(oldSceneNode->GetComponent<NodeInfo>()->nodeID_);
				msg_.WriteInt(-1);
				msg_.WriteBool(true);//new client.
				conn->SendMessage(MSG_GAMEMODEMSG, true, true, msg_);
			}

			int index = Random( 0, spawnPoints_.Size() );
			RespawnNode(sceneNode, index);

			AttachLogicComponents(sceneNode, -1);

			msg_.Clear();
			msg_.WriteInt(GAMEMODEMSG_RESPAWNNODE);
			msg_.WriteInt(sceneNode->GetComponent<NodeInfo>()->clientID_);
			msg_.WriteInt(sceneNode->GetComponent<NodeInfo>()->nodeID_);
			msg_.WriteInt(index);
			msg_.WriteBool(true);//new client
			network_->BroadcastMessage(MSG_GAMEMODEMSG, true, true, msg_);
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

void DotsNetCritsOnline::AttachLogicComponents(SharedPtr<Node> sceneNode, int nodeID)
{
	int noedID = nodeID;

	if (nodeID == -1)
	{
		noedID = nodeIDCounter_;
		nodeIDCounter_++;
	}

	sceneNode->AddComponent(new NodeInfo(context_, main_,
			"DotsNetCritsOnline",
			main_->GetClientID(sceneNode),
			noedID), 0, LOCAL);

	main_->identifiedNodes_.Push(sceneNode);

	sceneNode->AddComponent(new ModelController(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new ThirdPersonCamera(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new Speed(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new Gravity(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new RotateTo(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new MoveByTouch(context_, main_), 0, LOCAL);
	sceneNode->AddComponent(new SceneVoter(context_, main_), 0, LOCAL);
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

	NodeInfo* ni = targetSceneNode_->GetComponent<NodeInfo>();

	if (health <= 0)
	{
		int index = Random( 0, spawnPoints_.Size() );
		RespawnNode(SharedPtr<Node>(targetSceneNode_), index);

		msg_.Clear();
		msg_.WriteInt(GAMEMODEMSG_RESPAWNNODE);
		msg_.WriteInt(ni->clientID_);
		msg_.WriteInt(ni->nodeID_);
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
		msg_.WriteInt(ni->clientID_);
		msg_.WriteString("Health");
		msg_.WriteInt(ni->nodeID_);
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

void DotsNetCritsOnline::SpawnChicken(int clientID, int nodeID)
{
	Node* chickenNode = scene_->CreateChild("chicken",LOCAL);

	chickenNode->AddComponent(new NodeInfo(context_, main_,
			"DotsNetCritsOnline",
			clientID,
			nodeIDCounter_), 0, LOCAL);

	main_->identifiedNodes_.Push(chickenNode);

	chickenNode->AddComponent(new ChickenNPC(context_, main_), 0, LOCAL);

	if (main_->network_->IsServerRunning())
	{
		VectorBuffer msg;
		msg.WriteInt(clientID);
		msg.WriteString("DotsNetCritsOnline");
		msg.WriteInt(GAMEMODEMSG_SPAWNCHICKEN);
		msg.WriteInt(nodeIDCounter_);

		main_->network_->BroadcastMessage(MSG_LCMSG, true, true, msg);
	}
}

void DotsNetCritsOnline::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "DotsNetCritsOnline")
	{
		int gmMSG = msg.ReadInt();

		Connection* myconn = main_->GetConn(node_);
		SubscribeToEvent(E_SETLAGTIME, HANDLER(DotsNetCritsOnline, HandleSetLagTime));

		VariantMap vm;
		vm[GetLagTime::P_CONNECTION] = myconn;
		SendEvent(E_GETLAGTIME, vm);

		if (gmMSG == GAMEMODEMSG_SPAWNCHICKEN)
		{
			int nodeID = msg.ReadInt();
			SpawnChicken(clientID, nodeID);
		}
		else if (gmMSG == GAMEMODEMSG_LOADSCENE)
		{
			sceneFileName_ = msg.ReadString();
			LoadScene(sceneFileName_ + ".xml");
		}
	}
}

void DotsNetCritsOnline::HandleSetLagTime(StringHash eventType, VariantMap& eventData)
{
	Connection* sender = (Connection*)(eventData[SetLagTime::P_CONNECTION].GetPtr());
	if (sender == main_->GetConnByClientID(node_->GetComponent<NodeInfo>()->clientID_))
	{
		lagTime_ = eventData[SetLagTime::P_LAGTIME].GetFloat();

		UnsubscribeFromEvent(E_SETLAGTIME);
	}
}

void DotsNetCritsOnline::HandleGetLc(StringHash eventType, VariantMap& eventData)
{return;
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode != node_)
	{
		Node* chicken;

		for (int x = 0; x < main_->identifiedNodes_.Size(); x++)
		{
			if (main_->identifiedNodes_[x]->GetName() == "chicken")
			{
				chicken = main_->identifiedNodes_[x];
				break;
			}
		}

		if (!chicken)
		{
			return;
		}

		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		SubscribeToEvent(E_SETLAGTIME, HANDLER(DotsNetCritsOnline, HandleSetLagTime));

		VariantMap vm;
		vm[GetLagTime::P_CONNECTION] = conn;
		SendEvent(E_GETLAGTIME, vm);

		VectorBuffer msg;
		msg.WriteInt(node_->GetComponent<ClientInfo>()->clientID_);
		msg.WriteString("DotsNetCritsOnline");
		msg.WriteInt(GAMEMODEMSG_SPAWNCHICKEN);
		msg.WriteInt(chicken->GetComponent<NodeInfo>()->nodeID_);
		conn->SendMessage(MSG_LCMSG, true, true, msg);
	}
}

void DotsNetCritsOnline::LoadScene(String fileName)
{
	main_->ClearSceneNodes();
	spawnPoints_.Clear();

	if (scene_)
	{
		scene_->RemoveAllChildren();
		//scene_->RemoveAllComponents();
		scene_->Remove();
	}

	scene_ = new Scene(context_);

	sceneFileName_ = fileName;

	String filePath = main_->filesystem_->GetProgramDir() + "Data/Scenes/" + sceneFileName_;

	if (!main_->cache_->Exists(filePath))
	{
		return;
	}

	File loadFile(context_, filePath , FILE_READ);
	scene_->LoadXML(loadFile);

	for (int x = 0; x < main_->rootNodes_.Size(); x++)
	{
		SharedPtr<Node> sceneNode = SharedPtr<Node>( new Node(context_) );
		main_->sceneNodes_.Push(sceneNode);
		scene_->AddChild(sceneNode);

		if (main_->rootNodes_[x] == main_->myRootNode_)
		{
			main_->mySceneNode_ = sceneNode;
		}
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
	{
		nodeIDCounter_ = 0;

		for (int x = 0; x < main_->sceneNodes_.Size(); x++)
		{
			SharedPtr<Node> sceneNode = main_->sceneNodes_[x];

			int index = Random( 0, spawnPoints_.Size() );
			RespawnNode(sceneNode, index);

			AttachLogicComponents(sceneNode, -1);

			msg_.Clear();
			msg_.WriteInt(GAMEMODEMSG_RESPAWNNODE);
			msg_.WriteInt(sceneNode->GetComponent<NodeInfo>()->clientID_);
			msg_.WriteInt(sceneNode->GetComponent<NodeInfo>()->nodeID_);
			msg_.WriteInt(index);
			msg_.WriteBool(true);//new client.
			network_->BroadcastMessage(MSG_GAMEMODEMSG, true, true, msg_);
		}

		//SpawnChicken(node_->GetComponent<ClientInfo>()->clientID_, nodeIDCounter_);
		//nodeIDCounter_++;
	}
}

void DotsNetCritsOnline::HandleGetSceneName(StringHash eventType, VariantMap& eventData)
{
	VariantMap vm;
	vm[SetSceneName::P_SCENENAME] = sceneFileName_;
	SendEvent(E_SETSCENENAME, vm);
}

void DotsNetCritsOnline::HandleSetSceneVote(StringHash eventType, VariantMap& eventData)
{
	Node* sceneNode = (Node*)(eventData[SetSceneVote::P_SCENENODE].GetPtr());
	String sceneName = eventData[SetSceneVote::P_SCENENAME].GetString();

	sceneNode->GetComponent<SceneVoter>()->selectedFilename_ = sceneName;

	if (main_->network_->IsServerRunning())
	{
		sceneVoteCount_++;

		if (sceneVoteCount_ >= main_->sceneNodes_.Size())
		{
			sceneVoteCount_ = 0;

			sceneCandidates_.Clear();

			for (int x = 0; x < main_->sceneNodes_.Size(); x++)
			{
				String vote = main_->sceneNodes_[x]->GetComponent<SceneVoter>()->selectedFilename_;
				if (!sceneCandidates_.Contains(vote))
				{
					sceneCandidates_[vote] = 1;
				}
				else
				{
					sceneCandidates_[vote]++;
				}
			}

			String topVote = sceneCandidates_.Begin()->first_;
			int topVoteTally = sceneCandidates_[topVote];

			for (HashMap<String, int>::ConstIterator x = sceneCandidates_.Begin(); x != sceneCandidates_.End(); x++)
			{
				if (x->second_ > topVoteTally)
				{
					topVote = x->first_;
					topVoteTally = x->second_;
				}
			}

			VectorBuffer msg;
			msg.WriteInt(node_->GetComponent<ClientInfo>()->clientID_);
			msg.WriteString("DotsNetCritsOnline");
			msg.WriteInt(GAMEMODEMSG_LOADSCENE);
			msg.WriteString(topVote);

			main_->network_->BroadcastMessage(MSG_LCMSG, true, true, msg);

			sceneFileName_ = topVote;
			LoadScene(sceneFileName_ + ".xml");
		}
		else if (main_->sceneNodes_.Size() <= 2)//server and 1 player or just server.
		{
			sceneVoteCount_ = 0;

			int randy = Random(0, main_->sceneNodes_.Size());

			String vote = main_->sceneNodes_[randy]->GetComponent<SceneVoter>()->selectedFilename_;

			VectorBuffer msg;
			msg.WriteInt(node_->GetComponent<ClientInfo>()->clientID_);
			msg.WriteString("DotsNetCritsOnline");
			msg.WriteInt(GAMEMODEMSG_LOADSCENE);
			msg.WriteString(vote);

			main_->network_->BroadcastMessage(MSG_LCMSG, true, true, msg);

			sceneFileName_ = vote;
			LoadScene(sceneFileName_ + ".xml");
		}
	}
}
