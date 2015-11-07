/*
 * SkillbarMenu.h
 *
 *  Created on: Nov 5, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../../../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>

using namespace Urho3D;

class SkillbarMenu : public LogicComponent
{
	URHO3D_OBJECT(SkillbarMenu, LogicComponent);
public:
	SkillbarMenu(Context* context, Urho3DPlayer* main);
	~SkillbarMenu();
	virtual void Start();

	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandleButtonRelease(StringHash eventType, VariantMap& eventData);
	void PopulateLists();
    void HandleItemSelected(StringHash eventType, VariantMap& eventData);
    void HandleItemDeselected(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;
	SharedPtr<UIElement> menuButt_;
	SharedPtr<UIElement> menu_;
	UIElement* skillsList_;
	UIElement* skillbarList_;
	UIElement* addButt_;
	UIElement* removeButt_;
	UIElement* skill0Butt_;
	UIElement* skill1Butt_;
	UIElement* skill2Butt_;
	UIElement* skill3Butt_;
	UIElement* skill4Butt_;
	UIElement* skill5Butt_;
	Vector<String> skillfileNames_;
	Vector<String> skillbarfileNames_;
	String selectedSkillFilename_;
	String selectedSkillbarFilename_;

	SharedPtr<Node> skillbar_;
	Vector<Node*> skillbars_;
};