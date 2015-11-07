/*
 * Constants.h
 *
 *  Created on: Jul 6, 2015
 *      Author: practicing01
 */
#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Object.h>

#include "gameModes/dotsnetcritsonline/DotsNetCritsOnline.h"

using namespace Urho3D;

URHO3D_EVENT(E_GAMEMENUDISPLAY, GameMenuDisplay)
{
	URHO3D_PARAM(P_STATE, State);// bool
}

URHO3D_EVENT(E_NEWCLIENTID, NewClientID)
{
	URHO3D_PARAM(P_CLIENTID, ClientID);// int
}

URHO3D_EVENT(E_GAMEMODEREMOVED, GameModeRemoved)
{
}

URHO3D_EVENT(E_GETCLIENTCAMERA, GetClientCamera)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETCLIENTCAMERA, SetClientCamera)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_CAMERANODE, CameraNode);// node pointer
}

URHO3D_EVENT(E_TOUCHSUBSCRIBE, TouchSubscribe)
{
}

URHO3D_EVENT(E_TOUCHUNSUBSCRIBE, TouchUnSubscribe)
{
}

URHO3D_EVENT(E_GETCLIENTSPEED, GetClientSpeed)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETCLIENTSPEED, SetClientSpeed)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_SPEED, Speed);// float
}

URHO3D_EVENT(E_GETCLIENTGRAVITY, GetClientGravity)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETCLIENTGRAVITY, SetClientGravity)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_GRAVITY, Gravity);// float
}

URHO3D_EVENT(E_GETCLIENTMODELNODE, GetClientModelNode)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETCLIENTMODELNODE, SetClientModelNode)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_MODELNODE, ModelNode);// node pointer
}

URHO3D_EVENT(E_RESPAWNSCENENODE, RespawnSceneNode)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_POSITION, Position);// Vector3
	URHO3D_PARAM(P_ROTATION, Rotation);// Quaternion
}

URHO3D_EVENT(E_ROTATEMODELNODE, RotateModelNode)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_ROTATION, Rotation);// Quaternion
	URHO3D_PARAM(P_SPEED, Speed);// float
	URHO3D_PARAM(P_SPEEDRAMP, SpeedRamp);// float
	URHO3D_PARAM(P_STOPONCOMPLETION, StopOnCompletion);// bool
}

URHO3D_EVENT(E_ANIMATESCENENODE, AnimateSceneNode)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_ANIMATION, Animation);// string
	URHO3D_PARAM(P_LOOP, Loop);// bool
	URHO3D_PARAM(P_LAYER, Layer);// unsigned char
}

URHO3D_EVENT(E_GETCLIENTID, GetClientID)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETCLIENTID, SetClientID)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_CLIENTID, ClientID);// int
}

URHO3D_EVENT(E_LCMSG, LcMsg)
{
	URHO3D_PARAM(P_DATA, Data);// Buffer (PODVector)
}

URHO3D_EVENT(E_GETLAGTIME, GetLagTime)
{
	URHO3D_PARAM(P_CONNECTION, Connection);// Connection pointer
}

URHO3D_EVENT(E_SETLAGTIME, SetLagTime)
{
	URHO3D_PARAM(P_CONNECTION, Connection);// Connection pointer
	URHO3D_PARAM(P_LAGTIME, LagTime);// float
}

URHO3D_EVENT(E_GETCONNECTION, GetConnection)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETCONNECTION, SetConnection)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_CONNECTION, Connection);// connection pointer
}

URHO3D_EVENT(E_GETISSERVER, GetIsServer)
{
}

URHO3D_EVENT(E_SETISSERVER, SetIsServer)
{
	URHO3D_PARAM(P_ISSERVER, IsServer);// bool
}

URHO3D_EVENT(E_EXCLUSIVENETBROADCAST, ExclusiveNetBroadcast)
{
	URHO3D_PARAM(P_EXCLUDEDCONNECTION, ExcludedConnection);// connection pointer
	URHO3D_PARAM(P_MSG, Msg);// Buffer
}

URHO3D_EVENT(E_GETLC, GetLc)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_CONNECTION, Connection);// connection pointer
}

URHO3D_EVENT(E_GETSCENENODEBYMODELNODE, GetSceneNodeByModelNode)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETSCENENODEBYMODELNODE, SetSceneNodeByModelNode)
{
	URHO3D_PARAM(P_MODELNODE, ModelNode);// node pointer
	URHO3D_PARAM(P_SCENENODE, SceneNode);// node pointer
}

URHO3D_EVENT(E_GETSCENENODECLIENTID, GetSceneNodeClientID)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETSCENENODECLIENTID, SetSceneNodeClientID)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_CLIENTID, ClientID);// int
}

URHO3D_EVENT(E_GETMODELNODEBYSCENENODE, GetModelNodeBySceneNode)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETMODELNODEBYSCENENODE, SetModelNodeBySceneNode)
{
	URHO3D_PARAM(P_SCENENODE, SceneNode);// node pointer
	URHO3D_PARAM(P_MODELNODE, ModelNode);// node pointer
}

URHO3D_EVENT(E_MODIFYCLIENTSPEED, ModifyClientSpeed)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_SPEED, Speed);// float
	URHO3D_PARAM(P_OPERATION, Operation);// char
	URHO3D_PARAM(P_SENDTOSERVER, SendToServer);// bool
}

URHO3D_EVENT(E_GETCLIENTHEALTH, GetClientHealth)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
}

URHO3D_EVENT(E_SETCLIENTHEALTH, SetClientHealth)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_HEALTH, Health);// float
}

URHO3D_EVENT(E_MODIFYCLIENTHEALTH, ModifyClientHealth)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_HEALTH, Health);// int
	URHO3D_PARAM(P_OPERATION, Operation);// char
	URHO3D_PARAM(P_SENDTOSERVER, SendToServer);// bool
}

URHO3D_EVENT(E_CLIENTHEALTHSET, ClientHealthSet)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_HEALTH, Health);// int
}

URHO3D_EVENT(E_MOVEMODELNODE, MoveModelNode)
{
	URHO3D_PARAM(P_NODE, Node);// node pointer
	URHO3D_PARAM(P_DEST, Dest);// Vector3
	URHO3D_PARAM(P_SPEED, Speed);// float
	URHO3D_PARAM(P_SPEEDRAMP, SpeedRamp);// float
	URHO3D_PARAM(P_GRAVITY, Gravity);// float
	URHO3D_PARAM(P_GRAVITYRAMP, GravityRamp);// float
	URHO3D_PARAM(P_STOPONCOMPLETION, StopOnCompletion);// bool
	URHO3D_PARAM(P_ROTATE, Rotate);// bool
	URHO3D_PARAM(P_SENDTOSERVER, SendToServer);// bool
}

URHO3D_EVENT(E_SOUNDREQUEST, SoundRequest)
{
   URHO3D_PARAM(P_NODE, Node);
   URHO3D_PARAM(P_SOUNDTYPE, SoundType);
}

URHO3D_EVENT(E_SCENEOBJECTMOVETOCOMPLETE, SceneObjectMoveToComplete)
{
   URHO3D_PARAM(P_NODE, Node);
}

URHO3D_EVENT(E_GETSCENENODEBYINFO, GetSceneNodeByInfo)
{
	URHO3D_PARAM(P_LC, LC);// LC string
	URHO3D_PARAM(P_CLIENTID, ClientID);// int
	URHO3D_PARAM(P_NODEID, NodeID);// int
}

URHO3D_EVENT(E_SETSCENENODEBYINFO, SetSceneNodeByInfo)
{
	URHO3D_PARAM(P_LC, LC);// LC string
	URHO3D_PARAM(P_CLIENTID, ClientID);// int
	URHO3D_PARAM(P_NODEID, NodeID);// int
	URHO3D_PARAM(P_SCENENODE, SceneNode);// node pointer
}

URHO3D_EVENT(E_GETSCENENAME, GetSceneName)
{
}

URHO3D_EVENT(E_SETSCENENAME, SetSceneName)
{
	URHO3D_PARAM(P_SCENENAME, SceneName);//string
}

URHO3D_EVENT(E_SETSCENEVOTE, SetSceneVote)
{
	URHO3D_PARAM(P_SCENENODE, SceneNode);// node pointer
	URHO3D_PARAM(P_SCENENAME, SceneName);//string
}

extern const int GAMEMODEMSG_RESPAWNNODE;
extern const int GAMEMODEMSG_GETLC;
extern const int GAMEMODEMSG_SCENEVOTE;
extern const int SOUNDTYPE_CAST;
extern const int SOUNDTYPE_MELEE;
extern const int SOUNDTYPE_HURT;
