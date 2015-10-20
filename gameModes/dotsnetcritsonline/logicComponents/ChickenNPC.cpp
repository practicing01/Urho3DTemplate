/*
 * ChickenNPC.cpp
 *
 *  Created on: Oct 19, 2015
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

#include "ChickenNPC.h"
#include "../../../network/NetworkConstants.h"
#include "../../../Constants.h"

ChickenNPC::ChickenNPC(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
}

ChickenNPC::~ChickenNPC()
{
}

void ChickenNPC::Start()
{

	XMLFile* xmlFile = main_->cache_->GetResource<XMLFile>(
			main_->filesystem_->GetProgramDir()	+ "Data/Objects/npc/chicken.xml");
	Node* chicken = node_->GetScene()->InstantiateXML(xmlFile->GetRoot(),
			node_->GetPosition(), node_->GetRotation(), LOCAL);

	node_->AddChild(chicken);
}
