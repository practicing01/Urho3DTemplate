/*
 * Armor.h
 *
 *  Created on: Jul 22, 2015
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

class Armor : public LogicComponent
{
	OBJECT(Armor);
public:
	Armor(Context* context, Urho3DPlayer* main);
	~Armor();
	virtual void Start();
	void HandleGetArmor(StringHash eventType, VariantMap& eventData);
	void HandleSetIsServer(StringHash eventType, VariantMap& eventData);
	void HandleSetClientID(StringHash eventType, VariantMap& eventData);
	void HandleSetConnection(StringHash eventType, VariantMap& eventData);
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void HandleModifyArmor(StringHash eventType, VariantMap& eventData);
	void ModifyArmor(int armorMod, int operation, bool sendToServer);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	bool isServer_;
	int clientID_;
	Connection* conn_;

	int armor_;
};
