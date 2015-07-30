/*
 * ThirdPersonCamera.cpp
 *
 *  Created on: Jul 11, 2015
 *      Author: practicing01
 */

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Resource/XMLFile.h>

#include "ThirdPersonCamera.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

ThirdPersonCamera::ThirdPersonCamera(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	isEnabled_ = true;
	scene_ = NULL;
	cameraNode_ = NULL;
	modelNode_ = NULL;
}

ThirdPersonCamera::~ThirdPersonCamera()
{
}

void ThirdPersonCamera::Start()
{
	if (!main_->IsLocalClient(node_))
	{
		return;
	}

	scene_ = node_->GetScene();

	SubscribeToEvent(E_SETCLIENTCAMERA, HANDLER(ThirdPersonCamera, HandleSetCamera));
	SubscribeToEvent(E_SETCLIENTMODELNODE, HANDLER(ThirdPersonCamera, HandleSetClientModelNode));
	SubscribeToEvent(E_MECHANICREQUEST, HANDLER(ThirdPersonCamera, HandleMechanicRequest));

	VariantMap vm0;
	vm0[GetClientModelNode::P_NODE] = node_;
	SendEvent(E_GETCLIENTMODELNODE, vm0);

	VariantMap vm;
	vm[GetClientCamera::P_NODE] = node_;
	SendEvent(E_GETCLIENTCAMERA, vm);
}

void ThirdPersonCamera::HandleSetCamera(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientCamera::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		cameraNode_ = (Node*)(eventData[SetClientCamera::P_CAMERANODE].GetPtr());

		if (main_->viewport_ != NULL)
		{
			main_->viewport_->SetCamera(cameraNode_->GetComponent<Camera>());
		}

		camOrigin_ = cameraNode_->GetPosition();
		rayDistance_ = (cameraNode_->GetWorldPosition() - modelNode_->GetPosition()).Length();

		if (modelNode_ != NULL)
		{
			SubscribeToEvent(E_POSTUPDATE, HANDLER(ThirdPersonCamera, HandlePostUpdate));
		}

		UnsubscribeFromEvent(E_SETCLIENTCAMERA);
	}
}

void ThirdPersonCamera::HandleSetClientModelNode(StringHash eventType, VariantMap& eventData)
{
	Node* clientNode = (Node*)(eventData[SetClientModelNode::P_NODE].GetPtr());

	if (clientNode == node_)
	{
		modelNode_ = (Node*)(eventData[SetClientModelNode::P_MODELNODE].GetPtr());
		beeBox_ = modelNode_->GetComponent<AnimatedModel>()->GetWorldBoundingBox();

		if (cameraNode_ != NULL)
		{
			SubscribeToEvent(E_POSTUPDATE, HANDLER(ThirdPersonCamera, HandlePostUpdate));
		}

		UnsubscribeFromEvent(E_SETCLIENTMODELNODE);
	}
}

void ThirdPersonCamera::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	if (!scene_)
	{
		return;
	}
	else if (!scene_->GetComponent<Octree>())
	{
		return;
	}

	if (!modelNode_)
	{
		return;
	}

	if (!cameraNode_)
	{
		return;
	}

	float timeStep = eventData[PostUpdate::P_TIMESTEP].GetFloat();

	vectoria_ = beeBox_.Size();

	cameraRay_.origin_ = modelNode_->GetPosition();

	cameraRay_.origin_.y_ += vectoria_.y_;

	cameraRay_.direction_ = (cameraNode_->GetWorldPosition() - cameraRay_.origin_).Normalized();

	victoria_ = cameraNode_->GetPosition();
	remainingDist_ = (victoria_ - camOrigin_).Length();
	inderp_ = (remainingDist_ * timeStep);

	if (remainingDist_ > 1.0f)
	{
		cameraNode_->SetPosition(victoria_.Lerp(camOrigin_, inderp_ / remainingDist_));
		if ( (cameraNode_->GetPosition() - camOrigin_).Length() < 1.0f)
		{
			cameraNode_->SetPosition(camOrigin_);
		}
	}

	PODVector<RayQueryResult> results;

	RayOctreeQuery query(results, cameraRay_, RAY_TRIANGLE, rayDistance_,
			DRAWABLE_GEOMETRY);

	scene_->GetComponent<Octree>()->Raycast(query);

	if (results.Size())
	{
		int index = -1;
		for (int x = 0; x < results.Size(); x++)
		{
			if (results[x].drawable_->GetViewMask()!=1)//todo define viewmasks
			{
				if (index > -1)
				{
					if (results[x].distance_ < results[index].distance_)
					{
						index = x;
					}
				}
				else
				{
					index = x;
				}
			}
		}

		if (index > -1)
		{
			if (results[index].position_.y_ < cameraRay_.origin_.y_)
			{
				victoria_ = results[index].position_;
				victoria_.y_ += vectoria_.y_;
				cameraNode_->SetWorldPosition(victoria_);
			}
			else
			{
				cameraNode_->SetWorldPosition(results[index].position_);
			}
		}
	}

	cameraNode_->LookAt(cameraRay_.origin_);
}

void ThirdPersonCamera::HandleMechanicRequest(StringHash eventType, VariantMap& eventData)
{
	String mechanicID = eventData[MechanicRequest::P_MECHANICID].GetString();
	if (mechanicID == "CameraToggle")
	{
		if (!isEnabled_)
		{
			isEnabled_ = true;

			cameraNode_->SetPosition(camOrigin_);
			SubscribeToEvent(E_POSTUPDATE, HANDLER(ThirdPersonCamera, HandlePostUpdate));
		}
		else
		{
			isEnabled_ = false;

			UnsubscribeFromEvent(E_POSTUPDATE);
		}
	}
}
