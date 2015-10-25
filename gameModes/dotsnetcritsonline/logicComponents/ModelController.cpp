/*
 * ModelController.cpp
 *
 *  Created on: Oct 12, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/UI/ListView.h>
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
#include <Urho3D/UI/Text.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "ModelController.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

#include "Speed.h"
#include "NodeInfo.h"

ModelController::ModelController(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
}

ModelController::~ModelController()
{
	if (modelNode_ != NULL)
	{
		modelNode_->RemoveAllChildren();
		modelNode_->RemoveAllComponents();
		modelNode_->Remove();
	}

	main_->ui_->GetRoot()->RemoveChild(menuButt_);
	main_->ui_->GetRoot()->RemoveChild(menu_);
}

void ModelController::Start()
{
	modelNode_ = NULL;
	LoadDefaultPlayer();
	//SubscribeToEvent(E_UPDATE, HANDLER(ModelController, HandleUpdate));
	SubscribeToEvent(E_ANIMATESCENENODE, HANDLER(ModelController, HandleAnimateSceneNode));
	SubscribeToEvent(E_LCMSG, HANDLER(ModelController, HandleLCMSG));
	SubscribeToEvent(E_GETLC, HANDLER(ModelController, HandleGetLc));

	if (main_->IsLocalClient(node_) && !main_->engine_->IsHeadless())
	{
		menuButt_ = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/modelControllerButt.xml"));
		main_->ui_->GetRoot()->AddChild(menuButt_);
		main_->RecursiveAddGuiTargets(menuButt_);
		main_->ElementRecursiveResize(menuButt_);

		SubscribeToEvent(menuButt_, E_RELEASED, HANDLER(ModelController, HandleButtonRelease));

		menu_ = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/modelControllerMenu.xml"));
		main_->ui_->GetRoot()->AddChild(menu_);
		main_->RecursiveAddGuiTargets(menu_);
		main_->ElementRecursiveResize(menu_);
		menu_->SetEnabled(false);
		menu_->SetVisible(false);

		filenameList_ = menu_->GetChild("fileNames", true);
		aniList_ = menu_->GetChild("aniNames", true);
		keyLineEdit_ = menu_->GetChild("key", true);
		bindButt_ = menu_->GetChild("bind", true);
		loadButt_ = menu_->GetChild("load", true);
		playButt_ = menu_->GetChild("play", true);

		SubscribeToEvent(bindButt_, E_RELEASED, HANDLER(ModelController, HandleButtonRelease));
		SubscribeToEvent(loadButt_, E_RELEASED, HANDLER(ModelController, HandleButtonRelease));
		SubscribeToEvent(playButt_, E_RELEASED, HANDLER(ModelController, HandleButtonRelease));

		SubscribeToEvent(filenameList_, E_ITEMSELECTED, HANDLER(ModelController, HandleItemSelected));
		SubscribeToEvent(filenameList_, E_ITEMDESELECTED, HANDLER(ModelController, HandleItemDeselected));
		SubscribeToEvent(aniList_, E_ITEMSELECTED, HANDLER(ModelController, HandleItemSelected));
		SubscribeToEvent(aniList_, E_ITEMDESELECTED, HANDLER(ModelController, HandleItemDeselected));

		SubscribeToEvent(keyLineEdit_, E_TEXTFINISHED, HANDLER(ModelController, HandleTextFinished));

		SubscribeToEvent(E_KEYDOWN, HANDLER(ModelController, HandleKeyDown));

		XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>("Objects/boundKeys.xml");
		boundKeys_ = main_->scene_->InstantiateXML(xmlFile->GetRoot(), Vector3::ZERO, Quaternion(), LOCAL);

		VariantMap vm = boundKeys_->GetVars();
		Vector<StringHash> keys = vm.Keys();

		for (int x = 0; x < keys.Size(); x++)
		{
			boundKey_.Push(main_->input_->GetKeyName(keys[x].Value()).ToLower());
			boundAni_.Push(boundKeys_->GetVar(keys[x]).GetString());
		}
	}
}

void ModelController::HandleButtonRelease(StringHash eventType, VariantMap& eventData)
{
	using namespace Released;

	UIElement* ele = static_cast<UIElement*>(eventData[Released::P_ELEMENT].GetPtr());

	if (ele == menuButt_)
	{
		menu_->SetEnabled(!menu_->IsEnabled());
		menu_->SetVisible(!menu_->IsVisible());

		if (menu_->IsVisible())
		{
			PopulateModelList();
		}
		else
		{
			selectedFilename_ = "";
			selectedAni_ = "";
		}
	}
	else if (ele == bindButt_)
	{
		if (selectedAni_ != "")
		{
			for (int x = 0; x < boundKey_.Size(); x++)
			{
				if (boundKey_[x] == bindKey_)
				{
					boundAni_[x] = selectedAni_;
					return;
				}
			}

			boundKey_.Push(bindKey_);
			boundAni_.Push(selectedAni_);

			boundKeys_->SetVar(bindKey_, selectedAni_);

			File saveFile(context_, main_->filesystem_->GetProgramDir() + "Data/Objects/boundKeys.xml", FILE_WRITE);
			boundKeys_->SaveXML(saveFile);
		}
	}
	else if (ele == loadButt_)
	{
		if (modelNode_->GetVar("fileName").GetString() != selectedFilename_)
		{
			LoadPlayer(main_->filesystem_->GetProgramDir()
			+ "Data/Objects/player/" + selectedFilename_ + ".xml", true);
		}
	}
	else if (ele == playButt_)
	{
		if (modelNode_->GetVar("fileName").GetString() == selectedFilename_)
		{
			VariantMap vm;
			vm[AnimateSceneNode::P_NODE] = node_;
			vm[AnimateSceneNode::P_ANIMATION] = selectedAni_;
			vm[AnimateSceneNode::P_LOOP] = true;
			vm[AnimateSceneNode::P_LAYER] = 0;
			SendEvent(E_ANIMATESCENENODE, vm);

			NetSendAnimation(selectedAni_);
		}
	}
}

void ModelController::PopulateModelList()
{
	((ListView*)filenameList_)->RemoveAllItems();

	main_->filesystem_->ScanDir(fileNames_,
			main_->filesystem_->GetProgramDir() + "Data/Objects/player/",
			"*.xml", SCAN_FILES, false);

	for (int x = 0; x < fileNames_.Size(); x++)
	{
		SharedPtr<UIElement> filename = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/text.xml"));
		((Text*)((UIElement*)filename))->SetText(fileNames_[x].Substring(0, fileNames_[x].Find(".")).CString());

		((ListView*)filenameList_)->AddItem(filename);
	}
}

void ModelController::HandleItemSelected(StringHash eventType, VariantMap& eventData)
{
	using namespace ItemSelected;

	UIElement* list = static_cast<UIElement*>(eventData[P_ELEMENT].GetPtr());
	int index = eventData[P_SELECTION].GetInt();

	UIElement* item = ((ListView*)list)->GetItem(index);
	((Text*)item)->SetColor(Color(0.0f, 0.5f, 0.5f, 1.0f));

	if (list == filenameList_)
	{
		if (((ListView*)(filenameList_))->GetSelection() != index)
		{
			((ListView*)(filenameList_))->SetSelection(index);
		}

		selectedFilename_ = ((Text*)item)->GetText();

		((ListView*)aniList_)->RemoveAllItems();

		main_->filesystem_->ScanDir(fileNames_,
				main_->filesystem_->GetProgramDir() + "Data/Models/" + selectedFilename_ + "/",
				"*.ani", SCAN_FILES, false);

		for (int x = 0; x < fileNames_.Size(); x++)
		{
			SharedPtr<UIElement> filename = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/text.xml"));
			((Text*)((UIElement*)filename))->SetText(fileNames_[x].Substring(0, fileNames_[x].Find(".")).CString());

			((ListView*)aniList_)->AddItem(filename);
		}
	}
	else if (list == aniList_)
	{
		selectedAni_ = ((Text*)item)->GetText();
	}
}

void ModelController::HandleItemDeselected(StringHash eventType, VariantMap& eventData)
{
	using namespace ItemDeselected;

	UIElement* list = static_cast<UIElement*>(eventData[P_ELEMENT].GetPtr());
	int index = eventData[P_SELECTION].GetInt();

	UIElement* item = ((ListView*)list)->GetItem(index);
	((Text*)item)->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
}

void ModelController::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;

	int key = eventData[P_KEY].GetInt();

	for (int x = 0; x < boundKey_.Size(); x++)
	{
		if (main_->input_->GetKeyName(key).ToLower() == boundKey_[x].ToLower())
		{
			VariantMap vm;
			vm[AnimateSceneNode::P_NODE] = node_;
			vm[AnimateSceneNode::P_ANIMATION] = boundAni_[x];
			vm[AnimateSceneNode::P_LOOP] = true;
			vm[AnimateSceneNode::P_LAYER] = 0;
			SendEvent(E_ANIMATESCENENODE, vm);

			NetSendAnimation(boundAni_[x]);
			break;
		}
	}
}

void ModelController::HandleTextFinished(StringHash eventType, VariantMap& eventData)
{
	using namespace TextFinished;

	bindKey_ = eventData[P_TEXT].GetString();
}

void ModelController::LoadDefaultPlayer()
{
	XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>("Objects/playerInfo.xml");
	Node* playerInfo = main_->scene_->InstantiateXML(xmlFile->GetRoot(), Vector3::ZERO, Quaternion(), LOCAL);

	String fileName = playerInfo->GetVar("fileName").GetString();

	main_->scene_->RemoveChild(playerInfo);

	LoadPlayer(main_->filesystem_->GetProgramDir()
			+ "Data/Objects/player/" + fileName, false);//todo endless loop possibilities :d fix by using counter
}

void ModelController::RemoveModelNode()
{
	if (modelNode_ != NULL)
	{
		modelNode_->RemoveAllChildren();
		modelNode_->RemoveAllComponents();
		modelNode_->Remove();
	}
}

void ModelController::LoadPlayer(String modelPath, bool sendToServer)
{
	if (!main_->cache_->Exists(modelPath))
	{
		LoadDefaultPlayer();
		return;
	}

	RemoveModelNode();
    XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>(modelPath);
    modelNode_ = node_->GetScene()->InstantiateXML(xmlFile->GetRoot(),
    		Vector3::ZERO, Quaternion::IDENTITY, LOCAL);

    node_->AddChild(modelNode_);
    modelNode_->SetName("modelNode");

    modelPath_ = modelPath;

    String fileName = xmlFile->GetName();

    fileName = fileName.Substring(fileName.FindLast("/") + 1);

    fileName = fileName.Substring(0, fileName.Find("."));

    modelNode_->SetVar("fileName",fileName);

    if (sendToServer)
    {
    	VectorBuffer msg;
    	msg.WriteInt(node_->GetComponent<NodeInfo>()->clientID_);
    	msg.WriteString("ModelController");
    	msg.WriteInt(node_->GetComponent<NodeInfo>()->nodeID_);

    	msg.WriteInt(0);//Load player.
    	msg.WriteString(fileName + ".xml");

    	if (!main_->network_->IsServerRunning())
    	{
    		main_->network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg);
    	}
    	else//non-headless host.
    	{
    		main_->network_->BroadcastMessage(MSG_LCMSG, true, true, msg);
    	}
    }
}

void ModelController::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
}

void ModelController::HandleAnimateSceneNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[AnimateSceneNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		String ani = eventData[AnimateSceneNode::P_ANIMATION].GetString();
		bool loop = eventData[AnimateSceneNode::P_LOOP].GetBool();
		unsigned char layer = (unsigned char)(eventData[AnimateSceneNode::P_LAYER].GetUInt());
		RecursiveSetAnimation(modelNode_, ani, loop, layer);
	}
}

void ModelController::RecursiveSetAnimation(Node* noed, String ani, bool loop, unsigned char layer)//todo check if animation exists, if not set default.
{
	if (noed->HasComponent<AnimationController>())
	{
		String fileName = noed->GetVar("fileName").GetString();

		String aniPath = "Models/" + fileName + "/" + ani + ".ani";

		if (main_->cache_->Exists(aniPath))
		{//something's bugged somewhere and stuff might hang/crash/display wrong animations.
			if (( (noed->GetComponent<AnimationController>()->IsPlaying(aniPath) == false)
					&& (noed->GetComponent<AnimationController>()->IsFadingIn(aniPath) == false) )
					|| (noed->GetComponent<AnimationController>()->IsFadingOut(aniPath) == true)
					|| (noed->GetComponent<AnimationController>()->IsAtEnd(aniPath) == true) )
			//if (noed->GetComponent<AnimationController>()->IsPlaying(aniPath) == false)
			{
				//LOGERRORF("playing %s @ %d",aniPath.CString(), layer);
				//noed->GetComponent<AnimationController>()->FadeOthers(aniPath, 0.0f, 0.0f);
				//noed->GetComponent<AnimationController>()->StopAll(0.0f);
				//noed->GetComponent<AnimationController>()->StopLayer(layer, 0.0f);
				//noed->GetComponent<AnimationController>()->Play(aniPath, layer, loop, 0.0f);
				noed->GetComponent<AnimationController>()->StopLayer(0, 0.0f);
				noed->GetComponent<AnimationController>()->PlayExclusive(aniPath, layer, loop, 0.0f);
				//noed->GetComponent<AnimationController>()->FadeOthers(aniPath, 0.0f, 0.25f);
				//noed->GetComponent<AnimationController>()->SetStartBone(aniPath, "Bone");
				//noed->GetComponent<AnimationController>()->SetRemoveOnCompletion(aniPath, true);
				//noed->GetComponent<AnimationController>()->SetTime(aniPath, 0.0f);
				//noed->GetComponent<AnimationController>()->SetWeight(aniPath, 1.0f);
				//noed->GetComponent<AnimationController>()->Update(0.0f);

				float speed = node_->GetComponent<Speed>()->speed_;
				noed->GetComponent<AnimationController>()->SetSpeed(aniPath, (speed * 0.35f) *
						(noed->GetComponent<AnimationController>()->GetSpeed(aniPath)));

				if (!loop)
				{
					noed->GetComponent<AnimationController>()->SetAutoFade(aniPath, 0.25f);
				}
			}
			else
			{
				return;
			}
		}
	}

	for (int x = 0; x < noed->GetNumChildren(); x++)
	{
		RecursiveSetAnimation(noed->GetChild(x), ani, loop, layer);
	}
}

void ModelController::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode != node_)
	{
		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		VectorBuffer msg;
		msg.WriteInt(node_->GetComponent<NodeInfo>()->clientID_);
		msg.WriteString("ModelController");
    	msg.WriteInt(node_->GetComponent<NodeInfo>()->nodeID_);

		msg.WriteInt(0);//Load player.
		String fileName = modelNode_->GetVar("fileName").GetString();
		msg.WriteString(fileName + ".xml");
		conn->SendMessage(MSG_LCMSG, true, true, msg);
	}
}

void ModelController::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "ModelController")
	{
		int nodeID = msg.ReadInt();

		int myclientID = node_->GetComponent<NodeInfo>()->clientID_;
		int mynodeID = node_->GetComponent<NodeInfo>()->nodeID_;
		Connection* myconn = main_->GetConnByClientID(myclientID);

		if (myclientID == clientID && mynodeID == nodeID)
		{
			int operation = msg.ReadInt();

			String fileName;

			if (operation == 0)//Load player.
			{
				fileName = msg.ReadString();
				LoadPlayer(main_->filesystem_->GetProgramDir()
				+ "Data/Objects/player/" + fileName, false);
			}
			else if (operation == 1)
			{
				fileName = msg.ReadString();

				VariantMap vm;
				vm[AnimateSceneNode::P_NODE] = node_;
				vm[AnimateSceneNode::P_ANIMATION] = fileName;
				vm[AnimateSceneNode::P_LOOP] = true;
				vm[AnimateSceneNode::P_LAYER] = 0;
				SendEvent(E_ANIMATESCENENODE, vm);
			}

			if (main_->network_->IsServerRunning())
			{
				VectorBuffer msg;
				msg.WriteInt(myclientID);
				msg.WriteString("ModelController");
				msg.WriteInt(mynodeID);

				msg.WriteInt(operation);

				if (operation == 0)//Load player.
				{
					msg.WriteString(fileName);
				}
				else if (operation == 1)//animation
				{
					msg.WriteString(fileName);
				}

				VariantMap vm;
				vm[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = myconn;
				vm[ExclusiveNetBroadcast::P_MSG] = msg.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm);
			}
		}
	}
}

void ModelController::NetSendAnimation(String ani)
{
	VectorBuffer msg;
	msg.WriteInt(node_->GetComponent<NodeInfo>()->clientID_);
	msg.WriteString("ModelController");
	msg.WriteInt(node_->GetComponent<NodeInfo>()->nodeID_);

	msg.WriteInt(1);//animate player.
	msg.WriteString(ani);

	if (!main_->network_->IsServerRunning())
	{
		main_->network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg);
	}
	else//non-headless host.
	{
		main_->network_->BroadcastMessage(MSG_LCMSG, true, true, msg);
	}
}
