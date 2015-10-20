/*
 * SceneVoter.h
 *
 *  Created on: Oct 20, 2015
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

class SceneVoter : public LogicComponent
{
	OBJECT(SceneVoter, LogicComponent);
public:
	SceneVoter(Context* context, Urho3DPlayer* main);
	~SceneVoter();
	virtual void Start();
	void HandleButtonRelease(StringHash eventType, VariantMap& eventData);
	void PopulateSceneList();
    void HandleItemSelected(StringHash eventType, VariantMap& eventData);
    void HandleItemDeselected(StringHash eventType, VariantMap& eventData);

	Urho3DPlayer* main_;

	SharedPtr<UIElement> menuButt_;
	SharedPtr<UIElement> menu_;
	UIElement* filenameList_;
	SharedPtr<UIElement> voteButt_;
	Vector<String> fileNames_;

	String selectedFilename_;
};
