/*
 * NetPulse.cpp
 *
 *  Created on: Jul 8, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>

#include "NetPulse.h"
#include "NetworkConstants.h"
#include "../Constants.h"

NetPulse::NetPulse(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	elapsedTime_ = 0.0f;
	pulseInterval_ = 1.0f;
	network_ = GetSubsystem<Network>();
}

NetPulse::~NetPulse()
{
	ClearConnections();
}

void NetPulse::Start()
{
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(NetPulse, HandleUpdate));
	SubscribeToEvent(E_NETWORKMESSAGE, URHO3D_HANDLER(NetPulse, HandleNetworkMessage));
	SubscribeToEvent(E_GETLAGTIME, URHO3D_HANDLER(NetPulse, HandleGetLagTime));
}

void NetPulse::ClearConnections()
{
	for (int x = 0; x < pulseConns_.Size(); x++)
	{
		delete pulseConns_[x];
	}
	pulseConns_.Clear();
}

void NetPulse::RemoveConnection(Connection* conn)
{
	for (int x = 0; x < pulseConns_.Size(); x++)
	{
		if (pulseConns_[x]->connection_ == conn)
		{
			PulseConnections* pulseConn = pulseConns_[x];
			pulseConns_.Remove(pulseConns_[x]);
			delete pulseConn;
			return;
		}
	}
}

void NetPulse::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

	elapsedTime_ += timeStep;

	if (elapsedTime_ >= pulseInterval_)//Pulse every x seconds.
	{
		elapsedTime_ = 0.0f;

		msg_.Clear();

		msg_.WriteFloat(GetSubsystem<Time>()->GetElapsedTime());

		if (network_->IsServerRunning())
		{
			network_->BroadcastMessage(MSG_NETPULSE, false, false, msg_);
		}
		else if (network_->GetServerConnection())
		{
			network_->GetServerConnection()->SendMessage(MSG_NETPULSE, false, false, msg_);
		}
	}
}

void NetPulse::HandleNetworkMessage(StringHash eventType, VariantMap& eventData)
{
	using namespace NetworkMessage;

	int msgID = eventData[P_MESSAGEID].GetInt();

    if (msgID == MSG_NETPULSE)
    {
    	Connection* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

        const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
        MemoryBuffer msg(data);
        float time = msg.ReadFloat();

        for (int x = 0; x < pulseConns_.Size(); x++)
        {
        	PulseConnections* conn = pulseConns_[x];
        	if (conn->connection_ == sender)
        	{
        		conn->lagTime_ = Abs( (time - conn->lastPulseTime_) - pulseInterval_ );
        		conn->lastPulseTime_ = time;
        		return;
        	}
        }

        PulseConnections* conn = new PulseConnections;
        conn->connection_ = sender;
        conn->lastPulseTime_ = time;
        conn->lagTime_ = 0;

        pulseConns_.Push(conn);
    }
}

void NetPulse::HandleGetLagTime(StringHash eventType, VariantMap& eventData)
{
	Connection* sender = (Connection*)(eventData[GetLagTime::P_CONNECTION].GetPtr());

	for (int x = 0; x < pulseConns_.Size(); x++)
	{
		PulseConnections* conn = pulseConns_[x];
		if (conn->connection_ == sender)
		{
			VariantMap vm;
			vm[SetLagTime::P_CONNECTION] = sender;
			vm[SetLagTime::P_LAGTIME] = conn->lagTime_;
			SendEvent(E_SETLAGTIME, vm);
			return;
		}
	}
}

float NetPulse::GetLagTime(Connection* conn)
{
	for (int x = 0; x < pulseConns_.Size(); x++)
	{
		if (pulseConns_[x]->connection_ == conn)
		{
			return pulseConns_[x]->lagTime_;
		}
	}

	return 0.0f;
}
