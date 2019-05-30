/***************************************************************************
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

class RayDifferential : public Renderer
{
public:
        /** Called once right after context creation.
        */
		void onLoad(SampleCallbacks* pCallbacks, RenderContext* pRenderContext) override;

        /** Called on each frame render.
        */
        void onFrameRender(SampleCallbacks* pCallbacks, RenderContext* pRenderContext, const std::shared_ptr<Fbo>& pTargetFbo) override;

        /** Called every time the swap-chain is resized. You can query the default FBO for the new size and sample count of the window.
        */
        void onResizeSwapChain(SampleCallbacks* pCallbacks, uint32_t width, uint32_t height) override;

        /** Called every time a key event occurred.
        \param[in] keyEvent The keyboard event
        \return true if the event was consumed by the callback, otherwise false
        */
        bool onKeyEvent(SampleCallbacks* pCallbacks, const KeyboardEvent& keyEvent) override;

        /** Called every time a mouse event occurred.
        \param[in] mouseEvent The mouse event
        \return true if the event was consumed by the callback, otherwise false
        */
        bool onMouseEvent(SampleCallbacks* pCallbacks, const MouseEvent& mouseEvent)  override;

        /** Called after onFrameRender().
        It is highly recommended to use onGuiRender() exclusively for GUI handling. onGuiRender() will not be called when the GUI is hidden, which should help reduce CPU overhead.
        You could also ignore this and render the GUI directly in your onFrameRender() function, but that is discouraged.
        */
        void onGuiRender(SampleCallbacks* pCallbacks, Gui* pGui) override;



private:
    RtScene::SharedPtr mpScene;
    Camera::SharedPtr mpCamera;
    FirstPersonCameraController mCamController;

    //Ray tracing based renderer stuff
    RtProgram::SharedPtr mpRaytraceProgram = nullptr;
    RtProgramVars::SharedPtr mpRtVars;
    RtState::SharedPtr mpRtState;
    RtSceneRenderer::SharedPtr mpRtRenderer;
    Texture::SharedPtr mpRtOut;

	//private function used internally

    void renderRT(RenderContext* pContext, const Fbo* pTargetFbo);
	void loadScene(const std::string& filename, const Fbo* pTargetFbo);
    void setPerFrameVars(const Fbo* pTargetFbo);
};