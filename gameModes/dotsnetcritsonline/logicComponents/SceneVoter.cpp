/*
 * SceneVoter.cpp
 *
 *  Created on: Oct 20, 2015
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

#include "SceneVoter.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

SceneVoter::SceneVoter(Context* context, Urho3DPlayer* main) :
LogicComponent(context)
{
	main_ = main;
	selectedFilename_ = "cyberpunk";
}

SceneVoter::~SceneVoter()
{
	main_->ui_->GetRoot()->RemoveChild(menuButt_);
	main_->ui_->GetRoot()->RemoveChild(menu_);
}

void SceneVoter::Start()
{
	if (main_->IsLocalClient(node_) && !main_->engine_->IsHeadless())
	{
		menuButt_ = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/SceneVoteButt.xml"));
		main_->ui_->GetRoot()->AddChild(menuButt_);
		main_->RecursiveAddGuiTargets(menuButt_);
		main_->ElementRecursiveResize(menuButt_);

		SubscribeToEvent(menuButt_, E_RELEASED, HANDLER(SceneVoter, HandleButtonRelease));

		menu_ = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/SceneVoteMenu.xml"));
		main_->ui_->GetRoot()->AddChild(menu_);
		main_->RecursiveAddGuiTargets(menu_);
		main_->ElementRecursiveResize(menu_);
		menu_->SetEnabled(false);
		menu_->SetVisible(false);

		filenameList_ = menu_->GetChild("fileNames", true);
		voteButt_ = menu_->GetChild("Vote", true);

		SubscribeToEvent(voteButt_, E_RELEASED, HANDLER(SceneVoter, HandleButtonRelease));

		SubscribeToEvent(filenameList_, E_ITEMSELECTED, HANDLER(SceneVoter, HandleItemSelected));
		SubscribeToEvent(filenameList_, E_ITEMDESELECTED, HANDLER(SceneVoter, HandleItemDeselected));
	}
}

void SceneVoter::HandleButtonRelease(StringHash eventType, VariantMap& eventData)
{
	using namespace Released;

	UIElement* ele = static_cast<UIElement*>(eventData[Released::P_ELEMENT].GetPtr());

	if (ele == menuButt_)
	{
		menu_->SetEnabled(!menu_->IsEnabled());
		menu_->SetVisible(!menu_->IsVisible());

		if (menu_->IsVisible())
		{
			PopulateSceneList();
		}
	}
	else if (ele == voteButt_)
	{
		VectorBuffer msg;
		msg.WriteInt(GAMEMODEMSG_SCENEVOTE);
		msg.WriteInt(main_->GetClientID(node_));
		msg.WriteString(selectedFilename_);

		if (!main_->network_->IsServerRunning())
		{
			main_->network_->GetServerConnection()->SendMessage(MSG_GAMEMODEMSG, true, true, msg);
		}
		else//non-headless host.
		{
			main_->network_->BroadcastMessage(MSG_GAMEMODEMSG, true, true, msg);

			VariantMap vm;
			vm[SetSceneVote::P_SCENENODE] = node_;
			vm[SetSceneVote::P_SCENENAME] = selectedFilename_;
			SendEvent(E_SETSCENEVOTE, vm);
		}
	}
}

void SceneVoter::PopulateSceneList()
{
	((ListView*)filenameList_)->RemoveAllItems();

	main_->filesystem_->ScanDir(fileNames_,
			main_->filesystem_->GetProgramDir() + "Data/Scenes/",
			"*.xml", SCAN_FILES, false);

	for (int x = 0; x < fileNames_.Size(); x++)
	{
		SharedPtr<UIElement> filename = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/text.xml"));
		((Text*)((UIElement*)filename))->SetText(fileNames_[x].Substring(0, fileNames_[x].Find(".")).CString());

		((ListView*)filenameList_)->AddItem(filename);
	}
}

void SceneVoter::HandleItemSelected(StringHash eventType, VariantMap& eventData)
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
	}
}

void SceneVoter::HandleItemDeselected(StringHash eventType, VariantMap& eventData)
{
	using namespace ItemDeselected;

	UIElement* list = static_cast<UIElement*>(eventData[P_ELEMENT].GetPtr());
	int index = eventData[P_SELECTION].GetInt();

	UIElement* item = ((ListView*)list)->GetItem(index);
	((Text*)item)->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
}
