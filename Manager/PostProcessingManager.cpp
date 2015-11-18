/******************************************************************************/
/*!
\file   PostProcessingManager.cpp
\par    email: y.li\@digipen.edu
\par    name:  Sebastian Li Ye Wei
\par    Project: CS370 
\date   02/08/2013
\brief  
Class that performs post processing effects for the game

All content (c) 2012 DigiPen Institute of Technology Singapore, all rights reserved.
*/
/******************************************************************************/

/*****************************************************************************/
/*!
Includes
*/
/*****************************************************************************/
#include "Precompiled.h" //Precompiled header
#include "RenderBuffer.h"
#include "Window.h"
#include "GraphicsManager.h"
#include "ShaderManager.h"
#include "PostProEffect.h"

#include "PostProcessingManager.h" //Own header

/*****************************************************************************/
/*!
Use the engine namespace, for convenience
*/
/*****************************************************************************/
using namespace wfe;

PostProEffectFactoryContainer PostProcessingManager::mPostProEffectFactoryContainer;
TwBar* PostProcessingManager::sStackBar = 0;
TwBar* PostProcessingManager::sStackManagerBar = 0;

PostProcessingManager::PostProcessingManager() : mDrawDepthTexture(false)
{
  mSourceBuffer = new RenderBuffer(WFE_WINDOW->GetResoWidth(), WFE_WINDOW->GetResoHeight(), false);
  mDestBuffer = new RenderBuffer(WFE_WINDOW->GetResoWidth(), WFE_WINDOW->GetResoHeight(), false);

  //Init texture handle
  glGenTextures(1, &mOriginalTextureHandle);
  //Setup texture params
  glBindTexture(GL_TEXTURE_2D, mOriginalTextureHandle);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
  mData = new u8[4 * WFE_WINDOW->GetResoWidth() * WFE_WINDOW->GetResoHeight()];
}

PostProcessingManager::~PostProcessingManager()
{
  //Delete effects
  ClearPostProEffects();

  //Delete buffers
  SafeDelete(&mSourceBuffer);
  SafeDelete(&mDestBuffer);
  delete[] mData;
}

void PostProcessingManager::ApplyPostProEffects()
{
  WFE_GRAPHICS->ApplyAO();

  //Clear settings before we start
  WFE_GRAPHICS->SwitchShader(0);
  glActiveTexture(GL_TEXTURE0);

  //Profile how long our post processing takes
  WFE_FRC->StartTimingWindow("PostPro");

  s32 sizeX = WFE_WINDOW->GetResoWidth();
  s32 sizeY = WFE_WINDOW->GetResoHeight();
  Shader* shader = 0;

  Matrix4 oldProjViewMtx = WFE_GRAPHICS->GetProjViewMatrix();

  //////////////////////////////////////////////////////////////////////////
  //Set the projection matrix for the graphics
  WFE_GRAPHICS->SetProjViewMtx(Matrix4());

  //////////////////////////////////////////////////////////////////////////
  //Clear out the buffers
  mSourceBuffer->Bind();
  mSourceBuffer->Clear();

  //////////////////////////////////////////////////////////////////////////
  //Set some states
  glDisable(GL_DEPTH_TEST);
  glDepthMask(false);

  //////////////////////////////////////////////////////////////////////////
  //Copy the default frame buffer drawn to my own frame buffer
  mSourceBuffer->Bind();
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, sizeX, sizeY,
    0, 0, sizeX, sizeY,
    GL_COLOR_BUFFER_BIT,
    GL_NEAREST);

  // Assume we are having a clean image here
  glReadPixels(0, 0, sizeX, sizeY, GL_RGBA, GL_UNSIGNED_BYTE, mData);
  //Setup texture params
  glBindTexture(GL_TEXTURE_2D, mOriginalTextureHandle);
  //Allocate the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sizeX, sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, mData);
  glBindTexture(GL_TEXTURE_2D, 0);

  PostProEffectContainerIt ite = mPostProEffects.begin();
  while (ite != mPostProEffects.end())
  {
    (*ite)->Apply(mSourceBuffer, mDestBuffer);

    // swap around to save memory, no reason to create tons of frame buffer 
    // NOTE: limitation to this implementation is that every frame buffer are of the same size
    //       this can still be solved, inform me (ES) if you need such functionality (i was lazy to implement it)
    std::swap(mSourceBuffer, mDestBuffer);

    ++ite;
  }

  //////////////////////////////////////////////////////////////////////////
  //End image processing special effects
  ResetRenderTarget();

  //Draw from last used buffer back to screen
  shader = &WFE_SHADER_MANAGER->GetResource("SimpleAttribs.xml");
  WFE_GRAPHICS->SwitchShader(shader);
  shader->EnableTexture(mSourceBuffer->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  WFE_GRAPHICS->DrawOverScreen();

  //Draw out the depth texture
  if (mDrawDepthTexture)
  {
    glViewport(0, 0, 256, 256);
    shader = &WFE_SHADER_MANAGER->GetResource("SimpleAttribs.xml");
    WFE_GRAPHICS->SwitchShader(shader);
    shader->EnableTexture(WFE_GRAPHICS->GetShadowMapBuffer()->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);
    WFE_GRAPHICS->DrawOverScreen();
    ResetRenderTarget();
  }

  //////////////////////////////////////////////////////////////////////////
  //Reset states
  WFE_GRAPHICS->SwitchBlendingMode(WFE_BM_NORMAL);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(true);

  WFE_GRAPHICS->SetProjViewMtx(oldProjViewMtx);

  GraphicsManager::CheckGLError();
  WFE_GRAPHICS->SwitchShader(0);

  WFE_FRC->EndTimingWindow("PostPro");
}

void PostProcessingManager::ClearPostProEffects()
{
  while(!mPostProEffects.empty())
  {
    PopPostProEffect();
  }
}

void PostProcessingManager::PopPostProEffect()
{
  if (!mPostProEffects.empty())
  {
    FactoryFree(mPostProEffectFactoryContainer, mPostProEffects.back()->GetType(), mPostProEffects.back());
    mPostProEffects.pop_back();
  }
}

b8 PostProcessingManager::RemovePostProEffect(u32 index)
{
  if(index < mPostProEffects.size())
  {
    FactoryFree(mPostProEffectFactoryContainer, mPostProEffects[index]->GetType(), mPostProEffects[index]);

    mPostProEffects.erase(mPostProEffects.begin() + index);

    return true;
  }

  return false;
}

void PostProcessingManager::PushPostProEffect( PostProEffect* effect )
{
  effect->CreateATBMain(mPostProEffects.size());

  mPostProEffects.push_back(effect);
}


