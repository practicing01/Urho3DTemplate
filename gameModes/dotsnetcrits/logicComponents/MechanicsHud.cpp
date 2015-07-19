/*
 * MechanicsHud.cpp
 *
 *  Created on: Jul 10, 2015
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
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "../../../Urho3DPlayer.h"
#include "MechanicsHud.h"
#include "../../../Constants.h"

MechanicsHud::MechanicsHud(Context* context, Urho3DPlayer* main) :
	LogicComponent(context)
{
	main_ = main;
	network_ = GetSubsystem<Network>();
}

MechanicsHud::~MechanicsHud()
{
	main_->ui_->GetRoot()->RemoveChild(mechanicsHUD_);
}

void MechanicsHud::Start()
{
	mechanicsHUD_ = main_->ui_->LoadLayout(main_->cache_->GetResource<XMLFile>("UI/mechanicsHUD.xml"));
	main_->RecursiveAddGuiTargets(mechanicsHUD_);
	main_->ui_->GetRoot()->AddChild(mechanicsHUD_);
	main_->ElementRecursiveResize(mechanicsHUD_);

    for (int x = 0; x < mechanicsHUD_->GetNumChildren(); x++)
    {
    	SubscribeToEvent(mechanicsHUD_->GetChild(x), E_RELEASED, HANDLER(MechanicsHud, HandleRelease));
    }

	SubscribeToEvent(E_GAMEMODEREMOVED, HANDLER(MechanicsHud, HandleGameModeRemoved));
}

void MechanicsHud::HandleRelease(StringHash eventType, VariantMap& eventData)
{
	using namespace Released;

	UIElement* ele = static_cast<UIElement*>(eventData[Released::P_ELEMENT].GetPtr());

	String mechanicID = ele->GetVar("mechanicID").GetString();

	VariantMap vm;
	vm[MechanicRequest::P_MECHANICID] = mechanicID;
	SendEvent(E_MECHANICREQUEST, vm);
}

void MechanicsHud::HandleGameModeRemoved(StringHash eventType, VariantMap& eventData)
{
	Remove();
}
