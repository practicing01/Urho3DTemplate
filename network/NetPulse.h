/*
 * NetPulse.h
 *
 *  Created on: Jul 8, 2015
 *      Author: practicing01
 */

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/LogicComponent.h>
#include "../Urho3DPlayer.h"
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/Connection.h>

using namespace Urho3D;

class NetPulse : public LogicComponent
{
	URHO3D_OBJECT(NetPulse, LogicComponent);
public:
	NetPulse(Context* context, Urho3DPlayer* main);
	~NetPulse();
	virtual void Start();
    void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void ClearConnections();
    void RemoveConnection(Connection* conn);
	void HandleGetLagTime(StringHash eventType, VariantMap& eventData);
	float GetLagTime(Connection* conn);

	Urho3DPlayer* main_;

	Network* network_;

	VectorBuffer msg_;

	float elapsedTime_;
	float pulseInterval_;

	typedef struct
	{
		Connection* connection_;
		float lastPulseTime_;
		float lagTime_;
	}PulseConnections;

	Vector<PulseConnections*> pulseConns_;
};
