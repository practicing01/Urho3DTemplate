/*
 * ModelController.h
 *
 *  Created on: Oct 12, 2015
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

class ModelController : public LogicComponent
{
	OBJECT(ModelController, LogicComponent);
public:
	ModelController(Context* context, Urho3DPlayer* main);
	~ModelController();
	virtual void Start();
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void LoadDefaultPlayer();
	void LoadPlayer(String modelFilename, bool sendToServer);
	void RemoveModelNode();
	void HandleAnimateSceneNode(StringHash eventType, VariantMap& eventData);
	void RecursiveSetAnimation(Node* noed, String ani, bool loop, unsigned char layer);
	void HandleButtonRelease(StringHash eventType, VariantMap& eventData);
	void PopulateModelList();
    void HandleItemSelected(StringHash eventType, VariantMap& eventData);
    void HandleItemDeselected(StringHash eventType, VariantMap& eventData);
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
	void HandleTextFinished(StringHash eventType, VariantMap& eventData);
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void NetSendAnimation(String ani);

	Urho3DPlayer* main_;
	SharedPtr<Node> modelNode_;
	String modelPath_;
	SharedPtr<UIElement> menuButt_;
	SharedPtr<UIElement> menu_;
	UIElement* filenameList_;
	UIElement* aniList_;
	SharedPtr<UIElement> keyLineEdit_;
	SharedPtr<UIElement> bindButt_;
	SharedPtr<UIElement> loadButt_;
	SharedPtr<UIElement> playButt_;
	Vector<String> fileNames_;
	String selectedFilename_;
	String selectedAni_;
	String bindKey_;

	Vector<String> boundKey_;//Parallel vectors.
	Vector<String> boundAni_;

	SharedPtr<Node> boundKeys_;
};
