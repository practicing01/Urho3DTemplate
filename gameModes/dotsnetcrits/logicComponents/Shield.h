/*
 * Shield.h
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
#include <Urho3D/Graphics/ParticleEmitter.h>

#include "LC.h"
#include "LCTarget.h"

using namespace Urho3D;

class Shield : public LogicComponent
{
	OBJECT(Shield);
public:
	Shield(Context* context, Urho3DPlayer* main);
	~Shield();
	virtual void Start();
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandleMechanicRequest(StringHash eventType, VariantMap& eventData);
	void HandleSetEnabled(StringHash eventType, VariantMap& eventData);
	void HandleLCMSG(StringHash eventType, VariantMap& eventData);
	void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
    void HandleCleanseStatus(StringHash eventType, VariantMap& eventData);
	void HandleGetLc(StringHash eventType, VariantMap& eventData);
	void Exec(int clientID, float timeRamp, bool sendToServer);

	LC* lc_;
	Vector<LCTarget*> targets_;

	int shield_;
};
