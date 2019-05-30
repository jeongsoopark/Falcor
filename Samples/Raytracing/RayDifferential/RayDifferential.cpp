


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
#include "RayDifferential.h"
using namespace Falcor;

static const glm::vec4 kClearColor(0.38f, 0.52f, 0.10f, 1);
static const std::string kDefaultScene = "Arcade/Arcade.fscene";

void RayDifferential::loadScene(const std::string& filename, const Fbo* pTargetFbo)
{
    mpScene = RtScene::loadFromFile(filename, RtBuildFlags::None, Model::LoadFlags::RemoveInstancing);
    Model::SharedPtr pModel = mpScene->getModel(0);
    float ModelRadius = pModel->getRadius();
    mpCamera = mpScene->getActiveCamera();
	//if no camera in the scene
    assert(mpCamera);
    mCamController.attachCamera(mpCamera);

	//
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    Sampler::SharedPtr pSampler = Sampler::create(samplerDesc);
    pModel->bindSamplerToMaterials(pSampler);

	//Update camera controllers
    mCamController.setCameraSpeed(ModelRadius*0.25f);
    float nearZ = std::max(0.1f, pModel->getRadius() / 750.f);
    float farZ = ModelRadius * 10.f;
    mpCamera->setDepthRange(nearZ, farZ);
    mpCamera->setAspectRatio((float)pTargetFbo->getWidth() / (float)pTargetFbo->getHeight());
    mpRtVars = RtProgramVars::create(mpRaytraceProgram, mpScene);
    mpRtRenderer = RtSceneRenderer::create(mpScene);
}

void RayDifferential::onLoad(SampleCallbacks* pCallbacks, RenderContext* pRenderContext)
{
    //TODO: checking hardware that is capable of ray-tracing 

    //Loading shaders
    RtProgram::Desc rtProgramDesc;
    rtProgramDesc.addShaderLibrary("RayDifferential.rt.hlsl");
    rtProgramDesc.setRayGen("rayGen");
    rtProgramDesc.addHitGroup(0, "primaryClosestHit", "");
    rtProgramDesc.addHitGroup(1, "", "shadowAnyHit");
    rtProgramDesc.addMiss(0, "primaryMiss");
    rtProgramDesc.addMiss(1, "shadowMiss");
    mpRaytraceProgram = RtProgram::create(rtProgramDesc);

    loadScene(kDefaultScene, pCallbacks->getCurrentFbo().get());
    //Init RtState 
    mpRtState = RtState::create();
    mpRtState->setProgram(mpRaytraceProgram);
    mpRtState->setMaxTraceRecursionDepth(3);
}

void RayDifferential::renderRT(RenderContext* pContext, const Fbo* pTargetFbo)
{
    PROFILE("renderRT");
    setPerFrameVars(pTargetFbo);

    pContext->clearUAV(mpRtOut->getUAV().get(), kClearColor);
    mpRtVars->getRayGenVars()->setTexture("gOutput", mpRtOut);

    mpRtRenderer->renderScene(pContext, mpRtVars, mpRtState, uvec3(pTargetFbo->getWidth(), pTargetFbo->getHeight(), 1), mpCamera.get());
    pContext->blit(mpRtOut->getSRV(), pTargetFbo->getRenderTargetView(0));
}


void RayDifferential::setPerFrameVars(const Fbo* pTargetFbo)
{
    PROFILE("setPerFrameVars");
    GraphicsVars* pVars = mpRtVars->getGlobalVars().get();
    ConstantBuffer::SharedPtr pCB = pVars->getConstantBuffer("PerFrameCB");
    pCB["invView"] = glm::inverse(mpCamera->getViewMatrix());
    pCB["viewportDims"] = vec2(pTargetFbo->getWidth(), pTargetFbo->getHeight());
    float fovY = focalLengthToFovY(mpCamera->getFocalLength(), Camera::kDefaultFrameHeight);
    pCB["tanHalfFovY"] = tanf(fovY * 0.5f);
}

void RayDifferential::onGuiRender(SampleCallbacks* pCallbacks, Gui* pGui)
{
    if (pGui->addButton("Load Scene"))
    {
        std::string filename;
        if (openFileDialog(Scene::kFileExtensionFilters, filename))
        {
            loadScene(filename, pCallbacks->getCurrentFbo().get());
        }
    }
    for (uint32_t i = 0; i < mpScene->getLightCount(); i++)
    {
        std::string group = "Point Light" + std::to_string(i);
        mpScene->getLight(i)->renderUI(pGui, group.c_str());
    }
}

void RayDifferential::onFrameRender(SampleCallbacks* pCallbacks, RenderContext* pRenderContext, const std::shared_ptr<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(pTargetFbo.get(), kClearColor, 1.0f, 0, FboAttachmentType::All);

    if (mpScene)
    {
        mCamController.update();
        renderRT(pRenderContext, pTargetFbo.get());
    }
}

bool RayDifferential::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
	if (mCamController.onKeyEvent(keyEvent))
	{
		return true;
	}
	return false;
}

bool RayDifferential::onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent)
{
    return mCamController.onMouseEvent(mouseEvent);
}

void RayDifferential::onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height)
{
    float h = (float)height;
    float w = (float)width;

    mpCamera->setFocalLength(18);
    float aspectRatio = (w / h);
    mpCamera->setAspectRatio(aspectRatio);

    mpRtOut = Texture::create2D(width, height, ResourceFormat::RGBA16Float, 1, 1, nullptr, Resource::BindFlags::UnorderedAccess | Resource::BindFlags::ShaderResource);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    RayDifferential::UniquePtr pRenderer = std::make_unique<RayDifferential>();
    SampleConfig config;
    config.windowDesc.title = "RayDifferential";
    config.windowDesc.resizableWindow = true;

    Sample::run(config, pRenderer);
}

