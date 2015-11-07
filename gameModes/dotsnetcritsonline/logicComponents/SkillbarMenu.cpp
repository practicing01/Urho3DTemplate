/*
 * SkillbarMenu.cpp
 *
 *  Created on: Nov 5, 2015
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
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "SkillbarMenu.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

#include "Speed.h"
#include "NodeInfo.h"

SkillbarMenu::SkillbarMenu(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
}

SkillbarMenu::~SkillbarMenu()
{
	if (menuButt_)
	{
		main_->ui_->GetRoot()->RemoveChild(menuButt_);
	}

	if (menu_)
	{
		main_->ui_->GetRoot()->RemoveChild(menu_);
	}

	for (int x = 0; x < skillbars_.Size(); x++)
	{
		if (skillbars_[x])
		{
			skillbars_[x]->Remove();
		}
	}

	if (skillbar_)
	{
		skillbar_->Remove();
	}
}

void SkillbarMenu::Start()
{
	//SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(SkillbarMenu, HandleUpdate));

	if (main_->IsLocalClient(node_) && !main_->engine_->IsHeadless())
	{
		menuButt_ = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/skillbarMenuButt.xml"));
		main_->ui_->GetRoot()->AddChild(menuButt_);
		main_->RecursiveAddGuiTargets(menuButt_);
		main_->ElementRecursiveResize(menuButt_);

		SubscribeToEvent(menuButt_, E_RELEASED, URHO3D_HANDLER(SkillbarMenu, HandleButtonRelease));

		menu_ = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/skillbarMenu.xml"));
		main_->ui_->GetRoot()->AddChild(menu_);
		main_->RecursiveAddGuiTargets(menu_);
		main_->ElementRecursiveResize(menu_);
		menu_->SetEnabled(false);
		menu_->SetVisible(false);

		skillsList_ = menu_->GetChild("skills", true);
		skillbarList_ = menu_->GetChild("skillbars", true);
		addButt_ = menu_->GetChild("add", true);
		removeButt_ = menu_->GetChild("remove", true);
		skill0Butt_ = menu_->GetChild("skill0", true);
		skill1Butt_ = menu_->GetChild("skill1", true);
		skill2Butt_ = menu_->GetChild("skill2", true);
		skill3Butt_ = menu_->GetChild("skill3", true);
		skill4Butt_ = menu_->GetChild("skill4", true);
		skill5Butt_ = menu_->GetChild("skill5", true);

		XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>("Objects/skillbar.xml");
		skillbar_ = node_->GetScene()->InstantiateXML(xmlFile->GetRoot(), Vector3::ZERO, Quaternion(), LOCAL);

		SubscribeToEvent(addButt_, E_RELEASED, URHO3D_HANDLER(SkillbarMenu, HandleButtonRelease));
		SubscribeToEvent(removeButt_, E_RELEASED, URHO3D_HANDLER(SkillbarMenu, HandleButtonRelease));
		SubscribeToEvent(skill0Butt_, E_RELEASED, URHO3D_HANDLER(SkillbarMenu, HandleButtonRelease));
		SubscribeToEvent(skill1Butt_, E_RELEASED, URHO3D_HANDLER(SkillbarMenu, HandleButtonRelease));
		SubscribeToEvent(skill2Butt_, E_RELEASED, URHO3D_HANDLER(SkillbarMenu, HandleButtonRelease));
		SubscribeToEvent(skill3Butt_, E_RELEASED, URHO3D_HANDLER(SkillbarMenu, HandleButtonRelease));
		SubscribeToEvent(skill4Butt_, E_RELEASED, URHO3D_HANDLER(SkillbarMenu, HandleButtonRelease));
		SubscribeToEvent(skill5Butt_, E_RELEASED, URHO3D_HANDLER(SkillbarMenu, HandleButtonRelease));

		SubscribeToEvent(skillsList_, E_ITEMSELECTED, URHO3D_HANDLER(SkillbarMenu, HandleItemSelected));
		SubscribeToEvent(skillsList_, E_ITEMDESELECTED, URHO3D_HANDLER(SkillbarMenu, HandleItemDeselected));
		SubscribeToEvent(skillbarList_, E_ITEMSELECTED, URHO3D_HANDLER(SkillbarMenu, HandleItemSelected));
		SubscribeToEvent(skillbarList_, E_ITEMDESELECTED, URHO3D_HANDLER(SkillbarMenu, HandleItemDeselected));
	}
}

void SkillbarMenu::HandleButtonRelease(StringHash eventType, VariantMap& eventData)
{
	using namespace Released;

	UIElement* ele = static_cast<UIElement*>(eventData[Released::P_ELEMENT].GetPtr());

	if (ele == menuButt_)
	{
		menu_->SetEnabled(!menu_->IsEnabled());
		menu_->SetVisible(!menu_->IsVisible());

		if (menu_->IsVisible())
		{
			PopulateLists();
		}
		else
		{
			selectedSkillFilename_ = "";
			selectedSkillbarFilename_ = "";

			((Sprite*)(skill0Butt_->GetChild(0)))->SetTexture(NULL);
			((Sprite*)(skill0Butt_->GetChild(0)))->SetOpacity(0.0f);
			((Sprite*)(skill1Butt_->GetChild(0)))->SetTexture(NULL);
			((Sprite*)(skill1Butt_->GetChild(0)))->SetOpacity(0.0f);
			((Sprite*)(skill2Butt_->GetChild(0)))->SetTexture(NULL);
			((Sprite*)(skill2Butt_->GetChild(0)))->SetOpacity(0.0f);
			((Sprite*)(skill3Butt_->GetChild(0)))->SetTexture(NULL);
			((Sprite*)(skill3Butt_->GetChild(0)))->SetOpacity(0.0f);
			((Sprite*)(skill4Butt_->GetChild(0)))->SetTexture(NULL);
			((Sprite*)(skill4Butt_->GetChild(0)))->SetOpacity(0.0f);
			((Sprite*)(skill5Butt_->GetChild(0)))->SetTexture(NULL);
			((Sprite*)(skill5Butt_->GetChild(0)))->SetOpacity(0.0f);

			if (skillbar_)
			{
				skillbar_->Remove();
			}
		}
	}
	else if (ele == skill0Butt_)
	{
		if (selectedSkillFilename_ != "" && selectedSkillbarFilename_ != "")
		{
			((Sprite*)(skill0Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + selectedSkillFilename_ + "/icon.png"));

			((Sprite*)(skill0Butt_->GetChild(0)))->SetOpacity(1.0f);

			skillbar_->SetVar("skill0", selectedSkillFilename_);
			File saveFile(context_, main_->filesystem_->GetProgramDir() +
					"Data/Objects/skillbars/" + selectedSkillbarFilename_ + ".xml", FILE_WRITE);
			skillbar_->SaveXML(saveFile);
			saveFile.Flush();
			saveFile.Close();
		}
	}
	else if (ele == skill1Butt_)
	{
		if (selectedSkillFilename_ != "" && selectedSkillbarFilename_ != "")
		{
			((Sprite*)(skill1Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + selectedSkillFilename_ + "/icon.png"));

			((Sprite*)(skill1Butt_->GetChild(0)))->SetOpacity(1.0f);

			skillbar_->SetVar("skill1", selectedSkillFilename_);
			File saveFile(context_, main_->filesystem_->GetProgramDir() +
					"Data/Objects/skillbars/" + selectedSkillbarFilename_ + ".xml", FILE_WRITE);
			skillbar_->SaveXML(saveFile);
			saveFile.Flush();
			saveFile.Close();
		}
	}
	else if (ele == skill2Butt_)
	{
		if (selectedSkillFilename_ != "" && selectedSkillbarFilename_ != "")
		{
			((Sprite*)(skill2Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + selectedSkillFilename_ + "/icon.png"));

			((Sprite*)(skill2Butt_->GetChild(0)))->SetOpacity(1.0f);

			skillbar_->SetVar("skill2", selectedSkillFilename_);
			File saveFile(context_, main_->filesystem_->GetProgramDir() +
					"Data/Objects/skillbars/" + selectedSkillbarFilename_ + ".xml", FILE_WRITE);
			skillbar_->SaveXML(saveFile);
			saveFile.Flush();
			saveFile.Close();
		}
	}
	else if (ele == skill3Butt_)
	{
		if (selectedSkillFilename_ != "" && selectedSkillbarFilename_ != "")
		{
			((Sprite*)(skill3Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + selectedSkillFilename_ + "/icon.png"));

			((Sprite*)(skill3Butt_->GetChild(0)))->SetOpacity(1.0f);

			skillbar_->SetVar("skill3", selectedSkillFilename_);
			File saveFile(context_, main_->filesystem_->GetProgramDir() +
					"Data/Objects/skillbars/" + selectedSkillbarFilename_ + ".xml", FILE_WRITE);
			skillbar_->SaveXML(saveFile);
			saveFile.Flush();
			saveFile.Close();
		}
	}
	else if (ele == skill4Butt_)
	{
		if (selectedSkillFilename_ != "" && selectedSkillbarFilename_ != "")
		{
			((Sprite*)(skill4Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + selectedSkillFilename_ + "/icon.png"));

			((Sprite*)(skill4Butt_->GetChild(0)))->SetOpacity(1.0f);

			skillbar_->SetVar("skill4", selectedSkillFilename_);
			File saveFile(context_, main_->filesystem_->GetProgramDir() +
					"Data/Objects/skillbars/" + selectedSkillbarFilename_ + ".xml", FILE_WRITE);
			skillbar_->SaveXML(saveFile);
			saveFile.Flush();
			saveFile.Close();
		}
	}
	else if (ele == skill5Butt_)
	{
		if (selectedSkillFilename_ != "" && selectedSkillbarFilename_ != "")
		{
			((Sprite*)(skill5Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + selectedSkillFilename_ + "/icon.png"));

			((Sprite*)(skill5Butt_->GetChild(0)))->SetOpacity(1.0f);

			skillbar_->SetVar("skill5", selectedSkillFilename_);
			File saveFile(context_, main_->filesystem_->GetProgramDir() +
					"Data/Objects/skillbars/" + selectedSkillbarFilename_ + ".xml", FILE_WRITE);
			skillbar_->SaveXML(saveFile);
			saveFile.Flush();
			saveFile.Close();
		}
	}
	else if (ele == addButt_)
	{
		XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>("Objects/skillbar.xml");
		Node* skillbar = node_->GetScene()->InstantiateXML(xmlFile->GetRoot(), Vector3::ZERO, Quaternion(), LOCAL);

		File saveFile(context_, main_->filesystem_->GetProgramDir() +
				"Data/Objects/skillbars/" + String(skillbarfileNames_.Size()) + ".xml", FILE_WRITE);
		skillbar->SaveXML(saveFile);
		saveFile.Flush();
		saveFile.Close();

		PopulateLists();
	}
	else if (ele == removeButt_)
	{
		main_->filesystem_->Delete(main_->filesystem_->GetProgramDir() +
				"Data/Objects/skillbars/" + String(skillbarfileNames_.Size() - 1) + ".xml");

		PopulateLists();
	}
}

void SkillbarMenu::PopulateLists()
{
	((ListView*)skillsList_)->RemoveAllItems();
	((ListView*)skillbarList_)->RemoveAllItems();

	main_->filesystem_->ScanDir(skillfileNames_,
			main_->filesystem_->GetProgramDir() + "Data/LuaScripts/",
			"*.xml", SCAN_DIRS, false);

	for (int x = 0; x < skillfileNames_.Size(); x++)
	{
		if (skillfileNames_[x] == "."
				|| skillfileNames_[x] == ".."
						||skillfileNames_[x] == "onInit")
		{
			continue;
		}

		SharedPtr<UIElement> filename = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/text.xml"));
		((Text*)((UIElement*)filename))->SetText(skillfileNames_[x].CString());

		((ListView*)skillsList_)->AddItem(filename);
	}

	main_->filesystem_->ScanDir(skillbarfileNames_,
			main_->filesystem_->GetProgramDir() + "Data/Objects/skillbars/",
			"*.xml", SCAN_FILES, false);

	for (int x = 0; x < skillbarfileNames_.Size(); x++)
	{
		SharedPtr<UIElement> filename = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/text.xml"));
		((Text*)((UIElement*)filename))->SetText(skillbarfileNames_[x].Substring(0, skillbarfileNames_[x].Find(".")).CString());

		((ListView*)skillbarList_)->AddItem(filename);
	}
}

void SkillbarMenu::HandleItemSelected(StringHash eventType, VariantMap& eventData)
{
	using namespace ItemSelected;

	UIElement* list = static_cast<UIElement*>(eventData[P_ELEMENT].GetPtr());
	int index = eventData[P_SELECTION].GetInt();

	UIElement* item = ((ListView*)list)->GetItem(index);
	((Text*)item)->SetColor(Color(0.0f, 0.5f, 0.5f, 1.0f));

	if (list == skillsList_)
	{
		if (((ListView*)(skillsList_))->GetSelection() != index)
		{
			((ListView*)(skillsList_))->SetSelection(index);
		}

		selectedSkillFilename_ = ((Text*)item)->GetText();

		//
	}
	else if (list == skillbarList_)
	{
		if (((ListView*)(skillbarList_))->GetSelection() != index)
		{
			((ListView*)(skillbarList_))->SetSelection(index);
		}

		selectedSkillbarFilename_ = ((Text*)item)->GetText();

		Scene* scene = node_->GetScene();

		if (skillbar_)
		{
			skillbar_->Remove();
		}

		XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>("Objects/skillbars/" +
				selectedSkillbarFilename_ + ".xml");
		skillbar_ = scene->InstantiateXML(xmlFile->GetRoot(), Vector3::ZERO, Quaternion(), LOCAL);

		String skill0 = skillbar_->GetVar("skill0").GetString();
		String skill1 = skillbar_->GetVar("skill1").GetString();
		String skill2 = skillbar_->GetVar("skill2").GetString();
		String skill3 = skillbar_->GetVar("skill3").GetString();
		String skill4 = skillbar_->GetVar("skill4").GetString();
		String skill5 = skillbar_->GetVar("skill5").GetString();

		if (skill0 != "")
		{
			((Sprite*)(skill0Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + skill0 + "/icon.png"));

			((Sprite*)(skill0Butt_->GetChild(0)))->SetOpacity(1.0f);
		}
		else
		{
			((Sprite*)(skill0Butt_->GetChild(0)))->SetTexture(NULL);

			((Sprite*)(skill0Butt_->GetChild(0)))->SetOpacity(0.0f);
		}

		if (skill1 != "")
		{
			((Sprite*)(skill1Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + skill1 + "/icon.png"));

			((Sprite*)(skill1Butt_->GetChild(0)))->SetOpacity(1.0f);
		}
		else
		{
			((Sprite*)(skill1Butt_->GetChild(0)))->SetTexture(NULL);

			((Sprite*)(skill1Butt_->GetChild(0)))->SetOpacity(0.0f);
		}

		if (skill2 != "")
		{
			((Sprite*)(skill2Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + skill2 + "/icon.png"));

			((Sprite*)(skill2Butt_->GetChild(0)))->SetOpacity(1.0f);
		}
		else
		{
			((Sprite*)(skill2Butt_->GetChild(0)))->SetTexture(NULL);

			((Sprite*)(skill2Butt_->GetChild(0)))->SetOpacity(0.0f);
		}

		if (skill3 != "")
		{
			((Sprite*)(skill3Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + skill3 + "/icon.png"));

			((Sprite*)(skill3Butt_->GetChild(0)))->SetOpacity(1.0f);
		}
		else
		{
			((Sprite*)(skill3Butt_->GetChild(0)))->SetTexture(NULL);

			((Sprite*)(skill3Butt_->GetChild(0)))->SetOpacity(0.0f);
		}

		if (skill4 != "")
		{
			((Sprite*)(skill4Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + skill4 + "/icon.png"));

			((Sprite*)(skill4Butt_->GetChild(0)))->SetOpacity(1.0f);
		}
		else
		{
			((Sprite*)(skill4Butt_->GetChild(0)))->SetTexture(NULL);

			((Sprite*)(skill4Butt_->GetChild(0)))->SetOpacity(0.0f);
		}

		if (skill5 != "")
		{
			((Sprite*)(skill5Butt_->GetChild(0)))->SetTexture(
					main_->cache_->GetResource<Texture2D>("Textures/" + skill5 + "/icon.png"));

			((Sprite*)(skill5Butt_->GetChild(0)))->SetOpacity(1.0f);
		}
		else
		{
			((Sprite*)(skill5Butt_->GetChild(0)))->SetTexture(NULL);

			((Sprite*)(skill5Butt_->GetChild(0)))->SetOpacity(0.0f);
		}

		main_->cache_->ReleaseResource(XMLFile::GetTypeStatic(), "Objects/skillbars/" +
				selectedSkillbarFilename_ + ".xml");
	}
}

void SkillbarMenu::HandleItemDeselected(StringHash eventType, VariantMap& eventData)
{
	using namespace ItemDeselected;

	UIElement* list = static_cast<UIElement*>(eventData[P_ELEMENT].GetPtr());
	int index = eventData[P_SELECTION].GetInt();

	UIElement* item = ((ListView*)list)->GetItem(index);
	((Text*)item)->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
}

void SkillbarMenu::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
}
