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
	cameraNode_ = NULL;
}

ThirdPersonCamera::~ThirdPersonCamera()
{
}

void ThirdPersonCamera::Start()
{
	if (!main_->IsLocalClient(node_) || main_->engine_->IsHeadless())
	{
		return;
	}

	cameraNode_ = new Node(context_);
	cameraNode_ = node_->GetScene()->GetChild("camera")->Clone(LOCAL);

	originNode_ = node_->GetScene()->CreateChild("cameraOriginNode", LOCAL);

	if (main_->viewport_ != NULL)
	{
		main_->viewport_->SetCamera(cameraNode_->GetComponent<Camera>());
	}

	Node* modelNode = node_->GetChild("modelNode");

	node_->AddChild(cameraNode_);
	cameraNode_->SetName("cameraNode");

	node_->AddChild(originNode_);
	/*originNode_->AddChild(node_->GetChild("modelNode")->Clone());
	originNode_->GetChild("modelNode")->RemoveComponent(
			originNode_->GetChild("modelNode")->GetComponent<CollisionShape>());
*/
	BoundingBox beeBox = modelNode->GetComponent<AnimatedModel>()->GetWorldBoundingBox();

	cameraNode_->SetWorldPosition(node_->GetWorldPosition() + Vector3(0.0f, beeBox.Size().y_ * 1.5f, beeBox.Size().z_ * -15.0f));
	cameraNode_->SetWorldRotation(node_->GetWorldRotation());

	cameraNode_->LookAt(node_->GetWorldPosition() + Vector3(0.0f, beeBox.Size().y_, 0.0f));

	originNode_->SetWorldPosition(cameraNode_->GetWorldPosition());
	originNode_->SetWorldRotation(cameraNode_->GetWorldRotation());

	distanceDampener_ = 0.1f;

	//SubscribeToEvent(E_POSTUPDATE, HANDLER(ThirdPersonCamera, HandlePostUpdate));
}

void ThirdPersonCamera::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	if (!node_)
	{
		return;
	}
	else if (!node_->GetScene())
	{
		return;
	}

	Node* modelNode = node_->GetChild("modelNode");

	if (!modelNode)
	{
		return;
	}

	if (!cameraNode_)
	{
		return;
	}

	float timeStep = eventData[PostUpdate::P_TIMESTEP].GetFloat();

	BoundingBox beeBox = modelNode->GetComponent<AnimatedModel>()->GetWorldBoundingBox();

	Vector3 vectoria = beeBox.Size();

	Vector3 camOrigin = originNode_->GetPosition();

	float rayDistance = originNode_->GetPosition().Length();

	Ray cameraRay;

	cameraRay.origin_ = originNode_->GetWorldPosition();

	cameraRay.direction_ = originNode_->GetWorldDirection();

	Vector3 victoria_ = cameraNode_->GetPosition();
	float remainingDist = (victoria_ - originNode_->GetPosition()).Length();
	float inderp = (remainingDist * timeStep);

	if (remainingDist > distanceDampener_)
	{
		cameraNode_->SetPosition(victoria_.Lerp(camOrigin, inderp / remainingDist));
		if ( (cameraNode_->GetPosition() - camOrigin).Length() < distanceDampener_)
		{
			cameraNode_->SetPosition(camOrigin);
		}
	}

	PODVector<RayQueryResult> results;

	RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, rayDistance,
			DRAWABLE_GEOMETRY);

	node_->GetScene()->GetComponent<Octree>()->Raycast(query);

	if (results.Size())
	{
		int index = -1;
		for (int x = 0; x < results.Size(); x++)
		{
			if (results[x].drawable_->GetViewMask()!=1)//todo define viewmasks
			{
				if (index > -1)
				{
					if (results[x].distance_ > results[index].distance_)
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
			/*if (results[index].position_.y_ < cameraRay.origin_.y_)
			{
				victoria_ = results[index].position_;
				victoria_.y_ += vectoria.y_;
				cameraNode_->SetWorldPosition(victoria_);
			}
			else*/
			{
				cameraNode_->SetWorldPosition(results[index].position_);
				/*cameraNode_->SetWorldPosition(results[index].position_ +
						(results[index].normal_ * (rayDistance - results[index].distance_)));*/
			}
		}
	}

	cameraNode_->LookAt(node_->GetWorldPosition() + Vector3(0.0f, beeBox.Size().y_, 0.0f));
}
