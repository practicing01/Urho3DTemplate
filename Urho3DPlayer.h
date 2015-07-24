//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Graphics/Viewport.h>

using namespace Urho3D;

/// Urho3DPlayer application runs a script specified on the command line.
class Urho3DPlayer : public Application
{
    OBJECT(Urho3DPlayer);

public:
    /// Construct.
    Urho3DPlayer(Context* context);

    /// Setup before engine initialization. Verify that a script file has been specified.
    virtual void Setup();
    /// Setup after engine initialization. Load the script and execute its start function.
    virtual void Start();
    /// Cleanup after the main loop. Run the script's stop function if it exists.
    virtual void Stop();
    void HandleElementResize(StringHash eventType, VariantMap& eventData);
    void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    void ElementRecursiveResize(UIElement* ele);
    Node* GetRootNode(Connection* conn);
    Node* GetRootNode(int clientID);
    Node* GetRootNode(Node* sceneNode);
    void RemoveRootNode(SharedPtr<Node>);
    Node* GetSceneNode(int clientID);//todo replace these with events system
    Node* GetSceneNode(Node* rootNode);
    int GetClientID(Node* sceneNode);
    bool IsLocalClient(Node* sceneNode);
    void RecursiveAddGuiTargets(UIElement* ele);
    void ClearRootNodes();

    float timeStep_;
    Input* input_;
    SharedPtr<Viewport> viewport_;
    SharedPtr<Graphics> graphics_;
    SharedPtr<ResourceCache> cache_;
    SharedPtr<FileSystem> filesystem_;
    SharedPtr<Renderer> renderer_;
    SharedPtr<Network> network_;
    SharedPtr<UI> ui_;
    SharedPtr<Engine> engine_;
    SharedPtr<Audio> audio_;

    SharedPtr<Scene> scene_;
    SharedPtr<Node> myRootNode_;
    SharedPtr<Node> mySceneNode_;
    Vector< SharedPtr<Node> > rootNodes_;//Parallel array with sceneNodes_.
    Vector< SharedPtr<Node> > sceneNodes_;

	VectorBuffer msg_;

private:
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
};
