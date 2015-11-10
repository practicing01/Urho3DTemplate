/*
 * MoveByTouch.cpp
 *
 *  Created on: Jul 12, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
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
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>

#include "MoveByTouch.h"
#include "../../../network/NetworkConstants.h"
#include "../../../network/NetPulse.h"
#include "../../../Constants.h"

#include "Gravity.h"
#include "Speed.h"
#include "NodeInfo.h"

MoveByTouch::MoveByTouch(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	touchSubscriberCount_ = 0;
	isMoving_ = false;
}

MoveByTouch::~MoveByTouch()
{
}

void MoveByTouch::Start()
{
/*
	debugNode_ = node_->GetScene()->CreateChild("moveToDebugNode",LOCAL);
	debugNode_->AddChild(node_->GetChild("modelNode")->Clone());
	debugNode_->GetChild("modelNode")->RemoveComponent(
			debugNode_->GetChild("modelNode")->GetComponent<CollisionShape>());
*/
	isMoving_ = false;
	speed_ = node_->GetComponent<Speed>()->speed_;
	speedRamp_ = speed_;
	gravity_ = node_->GetComponent<Gravity>()->gravity_;
	gravityRamp_ = gravity_;

	SubscribeToEvent(E_TOUCHSUBSCRIBE, URHO3D_HANDLER(MoveByTouch, HandleTouchSubscribe));
	SubscribeToEvent(E_TOUCHUNSUBSCRIBE, URHO3D_HANDLER(MoveByTouch, HandleTouchUnSubscribe));
	SubscribeToEvent(E_MOVEMODELNODE, URHO3D_HANDLER(MoveByTouch, HandleMoveModelNode));
	SubscribeToEvent(node_->GetChild("modelNode"), E_NODECOLLISION, URHO3D_HANDLER(MoveByTouch, HandleNodeCollision));
	SubscribeToEvent(E_LCMSG, URHO3D_HANDLER(MoveByTouch, HandleLCMSG));
	SubscribeToEvent(E_GETLC, URHO3D_HANDLER(MoveByTouch, HandleGetLc));

	if (main_->IsLocalClient(node_))
	{
		SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(MoveByTouch, HandleTouchEnd));
	}

	SetUpdateEventMask(USE_FIXEDUPDATE);
}

void MoveByTouch::HandleTouchSubscribe(StringHash eventType, VariantMap& eventData)
{
	touchSubscriberCount_++;//Hazardous if a component gets deleted and it doesn't unsubscribe.
}

void MoveByTouch::HandleTouchUnSubscribe(StringHash eventType, VariantMap& eventData)
{
	touchSubscriberCount_--;
	if (touchSubscriberCount_ < 0)
	{
		touchSubscriberCount_ = 0;
	}
}

void MoveByTouch::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
{
	if (main_->ui_->GetFocusElement() || touchSubscriberCount_)
	{
		return;
	}

	if (!node_->GetScene())
	{
		return;
	}

	if (!node_->GetChild("cameraNode"))
	{
		return;
	}

	if (!node_->GetScene()->GetComponent<PhysicsWorld>())
	{
		return;
	}

	using namespace TouchEnd;

	Ray cameraRay = node_->GetChild("cameraNode")->GetComponent<Camera>()->GetScreenRay(
			(float) eventData[P_X].GetInt() / main_->graphics_->GetWidth(),
			(float) eventData[P_Y].GetInt() / main_->graphics_->GetHeight());

	PhysicsRaycastResult raeResult;

	node_->GetScene()->GetComponent<PhysicsWorld>()->RaycastSingle(raeResult, cameraRay, 10000.0f, 1);//todo define masks.

	float speed = node_->GetComponent<Speed>()->speed_;
	float gravity = node_->GetComponent<Gravity>()->gravity_;

	if (raeResult.body_)
	{
		MoveTo(raeResult.position_, speed, speed, gravity, gravity, true, true);
	}
	else
	{
		return;
	}

}

void MoveByTouch::MoveTo(Vector3 dest, float speed, float speedRamp, float gravity, float gravityRamp, bool rotate, bool sendToServer)
{
	loc_ = node_->GetPosition();

	if (loc_ == dest)
	{
		return;
	}

	VariantMap vm;
	vm[AnimateSceneNode::P_NODE] = node_;
	vm[AnimateSceneNode::P_ANIMATION] = "run";
	vm[AnimateSceneNode::P_LOOP] = true;
	vm[AnimateSceneNode::P_LAYER] = 0;
	SendEvent(E_ANIMATESCENENODE, vm);

	speed_ = speed;
	speedRamp_ = speedRamp;
	gravity_ = gravity;
	gravityRamp_ = gravityRamp;
	dest_ = dest;
	rotate_ = rotate;
	if (rotate)
	{
		dest_.y_ = loc_.y_;
	}
	dir_ = dest - loc_;
	dir_.Normalize();
	moveToTravelTime_ = (dest_ - loc_).Length() / speed;
	moveToElapsedTime_ = 0;
	Node* modelNode = node_->GetChild("modelNode");
	radius_ = modelNode->GetComponent<CollisionShape>()->GetSize().x_;
	radius_ *= modelNode->GetWorldScale().x_;
	isMoving_ = true;

	if (speedRamp_ < speed_)
	{
		speedRamp_ = speed_;
	}

	if (gravityRamp_ < gravity_)
	{
		gravityRamp_ = gravity_;
	}

	//RotateTo
	Quaternion quarterOnion_ = node_->GetRotation();//Temp storage.
	node_->LookAt(dest_);//Hack rotation calculation :d
	Quaternion quarterPounder_ = node_->GetRotation();
	node_->SetRotation(quarterOnion_);

	if (rotate)
	{
		vm.Clear();
		vm[RotateModelNode::P_NODE] = node_;
		vm[RotateModelNode::P_ROTATION] = quarterPounder_;
		vm[RotateModelNode::P_SPEED] = 4.0f;
		vm[RotateModelNode::P_SPEEDRAMP] = 4.0f;
		vm[RotateModelNode::P_STOPONCOMPLETION] = true;
		SendEvent(E_ROTATEMODELNODE, vm);
	}

	if (sendToServer)
	{
		VectorBuffer msg;
		msg.Clear();
		msg.WriteInt(node_->GetComponent<NodeInfo>()->clientID_);
		msg.WriteString("MoveByTouch");
		msg.WriteInt(node_->GetComponent<NodeInfo>()->nodeID_);
		msg.WriteVector3(loc_);
		msg.WriteVector3(dest_);
		msg.WriteFloat(speed_);
		msg.WriteFloat(speedRamp_);
		msg.WriteFloat(gravity_);
		msg.WriteFloat(gravityRamp_);
		if (!main_->network_->IsServerRunning())
		{
			main_->network_->GetServerConnection()->SendMessage(MSG_LCMSG, true, true, msg);
		}
		else//non-headless host.
		{
			main_->network_->BroadcastMessage(MSG_LCMSG, true, true, msg);
		}
	}
}

void MoveByTouch::FixedUpdate(float timeStep)
{
	if (!node_->GetScene())
	{
		return;
	}

	PhysicsWorld* pw = node_->GetScene()->GetComponent<PhysicsWorld>();

	Node* modelNode = node_->GetChild("modelNode");

	if (!modelNode)
	{
		return;
	}

	CollisionShape* colshape = modelNode->GetComponent<CollisionShape>();

	if (isMoving_ == true)
	{
		if (speedRamp_ > speed_)
		{
			speedRamp_ -= timeStep;
			if (speedRamp_ < speed_)
			{
				speedRamp_ = speed_;
			}
		}

		if (gravityRamp_ > gravity_)
		{
			gravityRamp_ -= timeStep;
			if (gravityRamp_ < gravity_)
			{
				gravityRamp_ = gravity_;
			}
		}

		inderp_ = speedRamp_ * timeStep;
		loc_ =  node_->GetPosition();
		if (rotate_)
		{
			dest_.y_ = loc_.y_;
		}
		remainingDist_ = (loc_ - dest_).Length();

		Vector3 victoria = loc_.Lerp(dest_, inderp_ / remainingDist_);

		node_->SetPosition(victoria);

		if (remainingDist_ <= 0.1f)
		{
			isMoving_ = false;
			OnMoveToComplete();
		}
	}

	//Apply gravity.
	loc_ = node_->GetPosition();
	radius_ = colshape->GetSize().x_;
	radius_ *= modelNode->GetWorldScale().x_;

	Vector3 gravity = Vector3(0.0f, node_->GetComponent<Gravity>()->gravity_, 0.0f);

	PhysicsRaycastResult raeResult;
	Ray rae;
	rae.origin_ = loc_;
	rae.origin_.y_ += radius_ * 0.5f;
	rae.direction_ = gravity.Normalized();

	float gravityStepDist = gravity.Length() * timeStep;

	pw->SphereCast(raeResult, rae, radius_, gravityStepDist, 1);//todo define masks.

	if (!raeResult.body_)
	{
		node_->SetPosition(loc_ + ( gravity * timeStep ) );
	}
	else
	{
		if (Abs(raeResult.position_.y_ - loc_.y_) > 0.1f)
		{
			loc_.y_ = raeResult.position_.y_;
			node_->SetPosition(loc_);
		}
		//debugNode_->SetPosition(raeResult.position_);
	}
}

void MoveByTouch::OnMoveToComplete()
{
	VariantMap vm;
	vm[SceneObjectMoveToComplete::P_NODE] = node_;
	SendEvent(E_SCENEOBJECTMOVETOCOMPLETE,vm);
	//todo implement stopOnCompletion = false
	//calculate direction based on previous location and keep going

	vm.Clear();
	vm[AnimateSceneNode::P_NODE] = node_;
	vm[AnimateSceneNode::P_ANIMATION] = "idle";
	vm[AnimateSceneNode::P_LOOP] = true;
	vm[AnimateSceneNode::P_LAYER] = 0;
	SendEvent(E_ANIMATESCENENODE, vm);
}

void MoveByTouch::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
	using namespace NodeCollision;

    MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());
    SharedPtr<Node> otherNode = SharedPtr<Node>(static_cast<Node*>(eventData[P_OTHERNODE].GetPtr()));

    Vector3 vectoria_ = node_->GetPosition();
    Vector3 highestContact = vectoria_;

    while (!contacts.IsEof())
    {
        Vector3 contactPosition = contacts.ReadVector3();
        Vector3 contactNormal = contacts.ReadVector3();
        float contactDistance = contacts.ReadFloat();
        float contactImpulse = contacts.ReadFloat();

        float level = Abs(contactNormal.y_);

        //debug
        //debugNode_->SetPosition(contactPosition);
        //end debug

        if (level > 0.1f)//todo change to be gravity independant
        {
        	if (contactPosition.y_ > highestContact.y_)
        	{
        		highestContact = contactPosition;
        	}
        }
        else//wall
        {
        	vectoria_ += contactNormal * (contactDistance);
        	node_->SetPosition(vectoria_);
        	isMoving_ = false;
        	//OnMoveToComplete();//todo add stopOnCollide param
        }
    }

    vectoria_.y_ = highestContact.y_;
    node_->SetPosition(vectoria_);
}

void MoveByTouch::HandleSetLagTime(StringHash eventType, VariantMap& eventData)
{

	Connection* sender = (Connection*)(eventData[SetLagTime::P_CONNECTION].GetPtr());
	if (sender == main_->GetConnByClientID(node_->GetComponent<NodeInfo>()->clientID_))
	{
		lagTime_ = eventData[SetLagTime::P_LAGTIME].GetFloat();

		UnsubscribeFromEvent(E_SETLAGTIME);
	}
}

void MoveByTouch::HandleLCMSG(StringHash eventType, VariantMap& eventData)
{
	const PODVector<unsigned char>& data = eventData[LcMsg::P_DATA].GetBuffer();
	MemoryBuffer msg(data);
	int clientID = msg.ReadInt();
	String lc = msg.ReadString();

	if (lc == "MoveByTouch")
	{
		int nodeID = msg.ReadInt();

		int myclientID = node_->GetComponent<NodeInfo>()->clientID_;
		int mynodeID = node_->GetComponent<NodeInfo>()->nodeID_;
		Connection* myconn = main_->GetConnByClientID(myclientID);

		if (myclientID == clientID && mynodeID == nodeID)
		{
			Vector3 loc = msg.ReadVector3();
			Vector3 dest = msg.ReadVector3();
			float speed = msg.ReadFloat();
			float speedRamp = msg.ReadFloat();
			float gravity = msg.ReadFloat();
			float gravityRamp = msg.ReadFloat();

			Node* rootNode = main_->GetRootNode(node_);

			if (rootNode->GetComponent<NetPulse>())
			{
				lagTime_ = rootNode->GetComponent<NetPulse>()->GetLagTime(myconn);
			}
			else
			{
				lagTime_ = 0.0f;
			}

			//todo change to be asynchronous
			speedRamp += speed * lagTime_;
			gravityRamp += gravity * lagTime_;

			node_->SetPosition(loc);

			if (loc != dest)
			{
				MoveTo(dest, speed, speedRamp, gravity, gravityRamp, true, false);
			}

			if (main_->network_->IsServerRunning())
			{
				//todo asynchronous messaging
				VectorBuffer msg;
				msg.WriteInt(myclientID);
				msg.WriteString("MoveByTouch");
				msg.WriteInt(mynodeID);
				msg.WriteVector3(loc);
				msg.WriteVector3(dest);
				msg.WriteFloat(speed_);
				msg.WriteFloat(speedRamp);
				msg.WriteFloat(gravity_);
				msg.WriteFloat(gravityRamp);

				VariantMap vm;
				vm[ExclusiveNetBroadcast::P_EXCLUDEDCONNECTION] = myconn;
				vm[ExclusiveNetBroadcast::P_MSG] = msg.GetBuffer();
				SendEvent(E_EXCLUSIVENETBROADCAST, vm);
			}
		}
	}
}

void MoveByTouch::HandleGetLc(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[GetLc::P_NODE].GetPtr());

	if (clientNode != node_)
	{
		Connection* conn = (Connection*)(eventData[GetLc::P_CONNECTION].GetPtr());

		Node* rootNode = main_->GetRootNode(node_);

		if (rootNode->GetComponent<NetPulse>())//todo figure out why this is needed & if netpulse actually works.
		{
			lagTime_ = rootNode->GetComponent<NetPulse>()->GetLagTime(conn);
		}
		else
		{
			lagTime_ = 0.0f;
		}

		//todo change to be asynchronous
		float speedRamp = speedRamp_ + (speed_ * lagTime_);
		float gravityRamp = gravityRamp_ + (gravity_ * lagTime_);

		VectorBuffer msg;
		msg.WriteInt(node_->GetComponent<NodeInfo>()->clientID_);
		msg.WriteString("MoveByTouch");
		msg.WriteInt(node_->GetComponent<NodeInfo>()->nodeID_);

		if (!isMoving_)
		{
			msg.WriteVector3(node_->GetPosition());
			msg.WriteVector3(node_->GetPosition());
		}
		else
		{
			msg.WriteVector3(node_->GetPosition());
			msg.WriteVector3(dest_);
		}
		msg.WriteFloat(speed_);
		msg.WriteFloat(speedRamp);
		msg.WriteFloat(gravity_);
		msg.WriteFloat(gravityRamp);
		conn->SendMessage(MSG_LCMSG, true, true, msg);
	}
}

void MoveByTouch::HandleMoveModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* node = (Node*)(eventData[MoveModelNode::P_NODE].GetPtr());

	if (node == node_)
	{
		Vector3 dest = eventData[MoveModelNode::P_DEST].GetVector3();
		float speed = eventData[MoveModelNode::P_SPEED].GetFloat();
		float speedRamp = eventData[MoveModelNode::P_SPEEDRAMP].GetFloat();
		float gravity = eventData[MoveModelNode::P_GRAVITY].GetFloat();
		float gravityRamp = eventData[MoveModelNode::P_GRAVITYRAMP].GetFloat();
		bool rotate = eventData[MoveModelNode::P_ROTATE].GetBool();
		bool sendToServer = eventData[MoveModelNode::P_SENDTOSERVER].GetBool();

		MoveTo(dest, speed, speedRamp, gravity, gravityRamp, rotate, sendToServer);
	}
}
