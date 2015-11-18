/******************************************************************************/
/*!
\file   PostProEffect.cpp
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

#include "PostProEffect.h" //Own header
#include "PostProcessingManager.h"
#include "LevelEditor.h"
#include "GameplayState.h"
#include "GameStateManager.h"
#include "TextureManager.h"
#include "Camera.h"

/*****************************************************************************/
/*!
Use the engine namespace, for convenience
*/
/*****************************************************************************/
using namespace wfe;

void TW_CALL SetCombineModeCB(const void *value, void *clientData)
{ 
  static_cast<PostProEffect*>(clientData)->SetCombineMode(*static_cast<const PostProcessingCombineModes*>(value));
}

void TW_CALL GetCombineModeCB(void *value, void *clientData)
{ 
  *static_cast<s32*>(value) = static_cast<PostProEffect*>(clientData)->GetCombineMode();
}


PostProEffect::PostProEffect(s32 type)
  :  mType(type), mShader(0), mCombineMode(POSTPRO_CM_REPLACE), mKeepInputImage(false)
{  
  //Init texture handle
  glGenTextures(1, &mInputTextureHandle);
  //Setup texture params
  glBindTexture(GL_TEXTURE_2D, mInputTextureHandle);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  mData = new u8[4 * WFE_WINDOW->GetResoWidth() * WFE_WINDOW->GetResoHeight()];
}

PostProEffect::~PostProEffect()
{
  delete[] mData;

  while(!mPrePostProEffect.empty())
  {
    FactoryFree(PostProcessingManager::mPostProEffectFactoryContainer, mPrePostProEffect.back()->GetType(), mPrePostProEffect.back());
    mPrePostProEffect.pop_back();
  }
}

/*****************************************************************************/
/*!
Applies the effect using the shader and combine mode
*/
/*****************************************************************************/
void PostProEffect::Apply(RenderBuffer*& source, RenderBuffer*& dest)
{
  if(mKeepInputImage)
  {
    // always keep image that was fed into this effect
    s32 sizeX = WFE_WINDOW->GetResoWidth();
    s32 sizeY = WFE_WINDOW->GetResoHeight();
    // Assume we are having a clean image here
    glReadPixels(0, 0, sizeX, sizeY, GL_RGBA, GL_UNSIGNED_BYTE, mData);
    //Setup texture params
    glBindTexture(GL_TEXTURE_2D, mInputTextureHandle);
    //Allocate the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sizeX, sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, mData);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  PostProEffectContainerIt ite = mPrePostProEffect.begin();
  while (ite != mPrePostProEffect.end())
  {
    (*ite)->Apply(source, dest);

    // swap around to save memory, no reason to create tons of frame buffer 
    // NOTE: limitation to this implementation is that every frame buffer are of the same size
    //       this can still be solved, inform me (ES) if you need such functionality (i was lazy to implement it)
    std::swap(source, dest);

    ++ite;
  }

  if(mPrePostProEffect.size() % 2)
  {
    std::swap(source, dest);
  }

  PreBindUpdate(source);

  //Dest buffer is not guaranteed to be cleared so we bind and clear it first
  dest->Bind();
  dest->Clear();
  // if null ptr, most likely u forgot to give ur post pro effect the name of the shader file
  ASSERT(mShader);

  //Use the shader for this effect
  WFE_GRAPHICS->SwitchShader(mShader);

  //Enable the color texture for the shader
  //You can enable other types of textures by changing the enum provided in the second argument
  //In your shaders, your samplers are expected to be named one of the following:
  // uColorMap
  // uEmissiveMap
  // uSpecularMap
  // uNormalMap 
  // uDissolveMap
  // uShadowMap
  //If you want to use a texture that is named something else you are going to have
  //to bind the texture unit and uniform manually. The enable texture function will
  //NOT work if you try to enable a texture that the shader does not have a
  //corresponding sampler for (with the expected name).

  EnableUniforms(source);

  //This is how you would pass in a custom uniform
  //glUniform1f(glGetUniformLocation(mShader->GetHandle(), "uMyUniformName"), 1.0f);

  //Draw a fullscreen quad over the screen (assumes that view proj mtx has been set to identity)
  //The view proj mtx should have been set in the PostProcessing class before this
  WFE_GRAPHICS->DrawOverScreen();

  //Any additional steps to apply the effect would go here.
  //For example, for 2 pass blur, you would draw to an intermediate buffer for the 
  //vertical pass, draw to another buffer for the horizontal pass and finally
  //use the code below to draw the results back into the source.

  //////////////////////////////////////////////////////////////////////////
  //Combine the result back into the source buffer.
  //Combine Modes Available:
  // REPLACE: What the shader draws is what the effect outputs
  // ADD: What the shader draws is added to the source image
  // SUB: What the shader draws is subtracted from the source image
  if (POSTPRO_CM_REPLACE != mCombineMode)
  {
    std::swap(source, dest);

    switch(mCombineMode)
    {
    case POSTPRO_CM_NORMAL:
      WFE_GRAPHICS->SwitchBlendingMode(WFE_BM_NORMAL);
      break;
    case POSTPRO_CM_ADD:
      WFE_GRAPHICS->SwitchBlendingMode(WFE_BM_ADD);
      break;
    case POSTPRO_CM_SUB:
      WFE_GRAPHICS->SwitchBlendingMode(WFE_BM_SUB);
      break;
    default:
      ASSERT(false);
    }

    dest->Bind();
    dest->Clear();

    //Draw the results back into the buffer
    WFE_GRAPHICS->SwitchShader(mShader);  //The SimpleAttribs shader simply draws the texture directly
    //mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR); //Tell the shader to use the given texture
    EnableUniforms(source);
    WFE_GRAPHICS->DrawOverScreen();
  }
}

void PostProEffect::EnableUniforms(RenderBuffer* source)
{
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);
}

void PostProEffect::CreateATBMain(u32 index)
{
  std::stringstream ss;

  ss << "Post Processing Effect #" << index;
  mName = ss.str();

  mNameFormatted = " group='" + mName + "' ";

  StringContainer stringContainer;

  stringContainer.push_back("REPLACE");
  stringContainer.push_back("NORMAL");
  stringContainer.push_back("ADD");
  stringContainer.push_back("SUB");

  TwAddSeparator(PostProcessingManager::sStackBar, "", mNameFormatted.c_str());
  AddDropDownVarCB(PostProcessingManager::sStackBar, 
    EDITOR_MODE_PAUSE, 
    "", 
    SetCombineModeCB, 
    GetCombineModeCB, 
    this, 
    ("label='Combine Mode'" + mNameFormatted).c_str(),
    "CombineModeEnum",
    stringContainer,
    false);

  CreateATB();
}

void PostProEffect::AddVarRW( cstr const name, TwType type, void* var, cstr const def )
{
  TwAddVarRW(PostProcessingManager::sStackBar, name, type, var, def);
}

void PostProEffect::PushbackPreEffects( s32 type )
{
  mPrePostProEffect.push_back(FactoryCreate(PostProcessingManager::mPostProEffectFactoryContainer,type));
}

Desaturation::Desaturation() : PostProEffect(sType), mSaturation(.5f)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("Desaturation.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
}

void Desaturation::CreateATB()
{
  AddVarRW("", TW_TYPE_FLOAT, &mSaturation, ("label='Saturation' min=0.0 max=1.0 step=0.01" + GetNameFormatted()).c_str());
}

void Desaturation::EnableUniforms(RenderBuffer* source)
{
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);
  //Pass a saturation value into the shader
  glUniform1f(mShader->GetSaturationHandle(), mSaturation);
}

SepiaTone::SepiaTone() : PostProEffect(sType)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("SepiaTone.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
}

BlurHorizontal::BlurHorizontal() : PostProEffect(sType), mHalfSize(7), mApplyNaiveDOF(false), mInvert(false), mBlurCutoff(0.3f)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("BlurHorizontal.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
  else
  {
    mApplyNaiveDOFHandle = glGetUniformLocation(mShader->GetHandle(), "uNaiveDOF");
    mBlurCutoffHandle = glGetUniformLocation(mShader->GetHandle(), "uBlurCutoff");
    mInvertHandle = glGetUniformLocation(mShader->GetHandle(), "uInvert");
  }
}

void BlurHorizontal::CreateATB()
{
  AddVarRW("", TW_TYPE_INT32, &mHalfSize, ("label='Kernel Half Size' min=1" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_BOOLCPP, &mApplyNaiveDOF, ("label='Naive DOF'" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_FLOAT, &mBlurCutoff, ("label='Blur Cutoff' min=0.0 max=1.0 step=0.01" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_BOOLCPP, &mInvert, ("label='Invert'" + GetNameFormatted()).c_str());
}

void BlurHorizontal::EnableUniforms( wfe::RenderBuffer* source )
{
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);
  mShader->EnableTexture(WFE_GRAPHICS->GetDepthAndNormalBuffer()->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_SHADOW);

  glUniform1f(mShader->GetMapSizeHandle(), 1.0f / (source->GetWidth()));
  glUniform1i(mShader->GetBlurHalfSizeHandle(), mHalfSize);

  glUniform1f(mBlurCutoffHandle, mBlurCutoff);
  glUniform1i(mInvertHandle, mInvert);
  glUniform1i(mApplyNaiveDOFHandle, mApplyNaiveDOF);
}

BlurVertical::BlurVertical() : PostProEffect(sType), mHalfSize(7), mApplyNaiveDOF(false), mInvert(false), mBlurCutoff(0.3f)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("BlurVertical.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
  else
  {
    mApplyNaiveDOFHandle = glGetUniformLocation(mShader->GetHandle(), "uNaiveDOF");
    mBlurCutoffHandle = glGetUniformLocation(mShader->GetHandle(), "uBlurCutoff");
    mInvertHandle = glGetUniformLocation(mShader->GetHandle(), "uInvert");
  }
}

void BlurVertical::CreateATB()
{
  AddVarRW("", TW_TYPE_INT32, &mHalfSize, ("label='Kernel Half Size' min=1" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_BOOLCPP, &mApplyNaiveDOF, ("label='Naive DOF'" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_FLOAT, &mBlurCutoff, ("label='Blur Cutoff' min=0.0 max=1.0 step=0.01" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_BOOLCPP, &mInvert, ("label='Invert'" + GetNameFormatted()).c_str());
}

void BlurVertical::EnableUniforms( wfe::RenderBuffer* source )
{
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);
  mShader->EnableTexture(WFE_GRAPHICS->GetDepthAndNormalBuffer()->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_SHADOW);

  glUniform1f(mShader->GetMapSizeHandle(), 1.0f / (source->GetHeight()));
  glUniform1i(mShader->GetBlurHalfSizeHandle(), mHalfSize);
  glUniform1f(mBlurCutoffHandle, mBlurCutoff);
  glUniform1i(mInvertHandle, mInvert);
  glUniform1i(mApplyNaiveDOFHandle, mApplyNaiveDOF);
}

BlackWhite::BlackWhite() : PostProEffect(sType), mTolerance(.4f)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("BlackWhite.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
}

void BlackWhite::CreateATB()
{
  AddVarRW("", TW_TYPE_FLOAT, &mTolerance, ("label='Tolerance' min=0.0 step=0.001" + GetNameFormatted()).c_str());
}

void BlackWhite::EnableUniforms( wfe::RenderBuffer* source )
{
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  glUniform1f(mShader->GetToleranceHandle(), mTolerance);
}

UnsharpMaskingDepth::UnsharpMaskingDepth() : PostProEffect(sType), mLambda(.1f), mLambdaHandle(-1), mHalfSize(9)
{  
  mKeepInputImage = true;
  mShader = &WFE_SHADER_MANAGER->GetResource("UnsharpMaskingDepth.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }  
  PushbackPreEffects(BlurVerticalDepth::sType);
  PushbackPreEffects(BlurHorizontalDepth::sType);

  mLambdaHandle = glGetUniformLocation(mShader->GetHandle(), "uLambda");
}

void UnsharpMaskingDepth::CreateATB()
{
  AddVarRW("", TW_TYPE_FLOAT, &mLambda, ("label='Lambda' step=0.01" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_INT32, &mHalfSize, ("label='Kernel Half Size' min=1" + GetNameFormatted()).c_str());
}

void UnsharpMaskingDepth::EnableUniforms( wfe::RenderBuffer* source )
{
  static_cast<BlurVerticalDepth*>(mPrePostProEffect[0])->mHalfSize = mHalfSize;
  static_cast<BlurHorizontalDepth*>(mPrePostProEffect[1])->mHalfSize = mHalfSize;

    // Enable the original texture
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  // Enable the original depth texture map
  mShader->EnableTexture(WFE_GRAPHICS->GetDepthAndNormalBuffer()->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_SHADOW);

  // Send output texture from previous effects
  mShader->EnableTexture(mInputTextureHandle, Shader::WFE_SHADER_MAPTYPE_ORIGINAL);
  glUniform1f(mLambdaHandle, mLambda);
}


GaussianBlur::GaussianBlur() : PostProEffect(sType), mRadius(1.f)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("GaussianBlur.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
}

void GaussianBlur::CreateATB()
{
  AddVarRW("", TW_TYPE_FLOAT, &mRadius, ("label='Radius' min=0.0 step=1.0" + GetNameFormatted()).c_str());
}

void GaussianBlur::EnableUniforms( wfe::RenderBuffer* source )
{
  // Enable current texture map
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  // Send uniforms for buffer height and width
  glUniform1f(mShader->GetMapHeightHandle(), static_cast<GLfloat>(source->GetHeight()));
  glUniform1f(mShader->GetMapWidthHandle(), static_cast<GLfloat>(source->GetWidth()));
  glUniform1f(glGetUniformLocation(mShader->GetHandle(), "uRadius"), mRadius);
  //mShader->EnableTexture(WFE_GRAPHICS->GetDepthAndNormalBuffer()->GetDepthTextureHandle(), Shader::WFE_SHADER_MAPTYPE_DEPTH);
}

Laplacian::Laplacian() : PostProEffect(sType)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("Laplacian.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
}

void Laplacian::CreateATB()
{

}

void Laplacian::EnableUniforms( wfe::RenderBuffer* source )
{
  // Enable current texture map
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  // Send uniforms for buffer height and width
  glUniform1f(mShader->GetMapHeightHandle(), static_cast<GLfloat>(source->GetHeight()));
  glUniform1f(mShader->GetMapWidthHandle(), static_cast<GLfloat>(source->GetWidth()));
}

Sobel::Sobel() : PostProEffect(sType)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("Sobel.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
}

void Sobel::CreateATB()
{
}

void Sobel::EnableUniforms( wfe::RenderBuffer* source )
{
  // Enable current texture map
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  // Send uniforms for buffer height and width
  glUniform1f(mShader->GetMapHeightHandle(), static_cast<GLfloat>(source->GetHeight()));
  glUniform1f(mShader->GetMapWidthHandle(), static_cast<GLfloat>(source->GetWidth()));
}

UnsharpMasking::UnsharpMasking() : PostProEffect(sType), mWeightage(1.f), mWeightageHandle(-1)
{  
  mKeepInputImage = true;
  mShader = &WFE_SHADER_MANAGER->GetResource("UnsharpMasking.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
  else
  {
    mWeightageHandle = glGetUniformLocation(mShader->GetHandle(), "uWeightage");
    PushbackPreEffects(BlurHorizontal::sType);
    PushbackPreEffects(BlurVertical::sType);
  } 
}

void UnsharpMasking::CreateATB()
{
  AddVarRW("", TW_TYPE_FLOAT, &mWeightage, ("label='Weightage' min=0.0 step=0.001" + GetNameFormatted()).c_str());
}

void UnsharpMasking::EnableUniforms( wfe::RenderBuffer* source )
{
  // Enable current texture map
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  // Send uniforms for buffer height and width
  glUniform1f(mShader->GetMapHeightHandle(), static_cast<GLfloat>(source->GetHeight()));
  glUniform1f(mShader->GetMapWidthHandle(), static_cast<GLfloat>(source->GetWidth()));
  glUniform1f(mWeightageHandle, mWeightage);

  // Send default original clean image
  mShader->EnableTexture(mInputTextureHandle, Shader::WFE_SHADER_MAPTYPE_ORIGINAL);
}

Negative::Negative() : PostProEffect(sType)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("Negative.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
}

HueChange::HueChange() : PostProEffect(sType), mHue(.0f), mSaturation(.0f), mValue(.0f)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("HueChange.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
  else
  {
    mHueHandle = glGetUniformLocation(mShader->GetHandle(), "uHue");
    mSaturationHandle = glGetUniformLocation(mShader->GetHandle(), "uSaturation");
    mValueHandle = glGetUniformLocation(mShader->GetHandle(), "uValue");
  }
}

void HueChange::CreateATB()
{
  AddVarRW("", TW_TYPE_FLOAT, &mHue, ("label='Hue' min=-1.0 max=1.0 step=0.001" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_FLOAT, &mSaturation, ("label='Saturation' min=-1.0 max=1.0 step=0.001" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_FLOAT, &mValue, ("label='Value' min=-1.0 max=1.0 step=0.001" + GetNameFormatted()).c_str());
}

void HueChange::EnableUniforms( wfe::RenderBuffer* source )
{
  // Enable current texture map
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  // Send uniforms for buffer height and width
  glUniform1f(mHueHandle, mHue);
  glUniform1f(mSaturationHandle, mSaturation);
  glUniform1f(mValueHandle, mValue);
}

RealisticDOF::RealisticDOF() : PostProEffect(sType), mBias(2.5), mInvert(false)
{
  mKeepInputImage = true;
  mShader = &WFE_SHADER_MANAGER->GetResource("RealisticDOF.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
  else
  { 
    PushbackPreEffects(BlurHorizontal::sType);
    PushbackPreEffects(BlurVertical::sType);

    mInvertHandle = glGetUniformLocation(mShader->GetHandle(), "uInvert");
    mBiasHandle = glGetUniformLocation(mShader->GetHandle(), "uBias");
  } 
}

void RealisticDOF::CreateATB()
{
  AddVarRW("", TW_TYPE_FLOAT, &mBias, ("label='Bias' min=-0.0 step=0.1" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_BOOLCPP, &mInvert, ("label='Invert'" + GetNameFormatted()).c_str());
}

void RealisticDOF::EnableUniforms( wfe::RenderBuffer* source )
{
  // Enable current texture map
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  // Send default original clean image
  mShader->EnableTexture(mInputTextureHandle, Shader::WFE_SHADER_MAPTYPE_ORIGINAL);

  mShader->EnableTexture(WFE_GRAPHICS->GetDepthAndNormalBuffer()->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_SHADOW);

  glUniform1f(mBiasHandle, mBias);
  glUniform1i(mInvertHandle, mInvert);
}

BlurHorizontalDepth::BlurHorizontalDepth() : PostProEffect(sType), mHalfSize(5)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("BlurHorizontal.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
  else
  {
    mApplyNaiveDOFHandle = glGetUniformLocation(mShader->GetHandle(), "uNaiveDOF");
  }
}

void BlurHorizontalDepth::EnableUniforms( wfe::RenderBuffer* source )
{
  mShader->EnableTexture(WFE_GRAPHICS->GetDepthAndNormalBuffer()->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  glUniform1f(mShader->GetMapSizeHandle(), 1.0f / (source->GetWidth()));
  glUniform1i(mShader->GetBlurHalfSizeHandle(), mHalfSize);

  glUniform1i(mApplyNaiveDOFHandle, false);
}

BlurVerticalDepth::BlurVerticalDepth() : PostProEffect(sType), mHalfSize(5)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("BlurVertical.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
  else
  {
    mApplyNaiveDOFHandle = glGetUniformLocation(mShader->GetHandle(), "uNaiveDOF");
  }
}

void BlurVerticalDepth::EnableUniforms( wfe::RenderBuffer* source )
{
  mShader->EnableTexture(WFE_GRAPHICS->GetDepthAndNormalBuffer()->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  glUniform1f(mShader->GetMapSizeHandle(), 1.0f / (source->GetHeight()));
  glUniform1i(mShader->GetBlurHalfSizeHandle(), mHalfSize);

  glUniform1i(mApplyNaiveDOFHandle, false);
}

AdditiveNoise::AdditiveNoise() : PostProEffect(sType), mOffsetYHandle(-1), mOffsetXHandle(-1), mBias(0.4f)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("AdditiveNoise.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
  s32 sizeX = WFE_WINDOW->GetResoWidth();
  s32 sizeY = WFE_WINDOW->GetResoHeight();
  s32 size = 4 * sizeX * sizeY;
  u8* data = new u8[size]();

  // Create a new noisy texture
  glGenTextures(1, &mNoisyTextureHandle);
  //Setup texture params
  glBindTexture(GL_TEXTURE_2D, mNoisyTextureHandle);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  f32 standardDeviation = 5.f;
  // Perform the noisiness
  for(s32 i = 0; i < size; ++i)
  {
    //Generate amount to be added
    f32 ran = glm::gaussRand1(0.1f, standardDeviation);      
    data[i] += (u8)ran;

    data[i] = Clamp<u8>(data[i], 0, 255);
  }
  // Upload to GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sizeX, sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glBindTexture(GL_TEXTURE_2D, 0);
  delete [] data;

  mOffsetXHandle = glGetUniformLocation(mShader->GetHandle(), "uOffsetX");
  mOffsetYHandle = glGetUniformLocation(mShader->GetHandle(), "uOffsetY");
  mBiasHandle = glGetUniformLocation(mShader->GetHandle(), "uBias");
}

void AdditiveNoise::CreateATB()
{
  AddVarRW("", TW_TYPE_FLOAT, &mBias, ("label='Value' min=-0.0 max=1.0 step=0.001" + GetNameFormatted()).c_str());
}

void AdditiveNoise::EnableUniforms( wfe::RenderBuffer* source )
{
  // Enable current texture map
  mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

  // Enable noisy texture map
  mShader->EnableTexture(mNoisyTextureHandle, Shader::WFE_SHADER_MAPTYPE_OTHER);

  // Send uniforms
  glUniform1f(mOffsetXHandle, glm::compRand1(0.f, 1.f));
  glUniform1f(mOffsetYHandle, glm::compRand1(0.f, 1.f));
  glUniform1f(mBiasHandle, mBias);
}

// AdditiveNoise::~AdditiveNoise()
// {
//     //delete [] m_data;
// }




BloomCombine::BloomCombine():PostProEffect(sType),
                             m_coefP1(.15f),
                             m_coefP1x(1.0f),
                             m_coefP1y(1.0f),
                             m_coefP1z(1.0f),
                             m_coefP2(1.f)
{
	mShader = &WFE_SHADER_MANAGER->GetResource("Bloom_combine.xml");
  mKeepInputImage = true;

	if(!mShader)
	{
		WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
	}
	else
	{ 
    PushbackPreEffects(LuminanceThreshold::sType);
		PushbackPreEffects(BlurHorizontal::sType);
		PushbackPreEffects(BlurVertical::sType);
    static_cast<BlurHorizontal*>(mPrePostProEffect[1])->mHalfSize = 3;
    static_cast<BlurVertical*>(mPrePostProEffect[2])->mHalfSize = 3;
	}

	s32 size = 1024;

  for(u32 i = 0; i < 6; i +=2)
  {
    mRenderBuffer[i] = new wfe::RenderBuffer(size, size, false);
    mRenderBuffer[i + 1] = new wfe::RenderBuffer(size, size, false);

    size /= 2;
  }

	locC1 = glGetUniformLocation(mShader->GetHandle(),"uCoeft1") ;
	locC2 =	glGetUniformLocation(mShader->GetHandle(),"uCoeft1x");
	locC3 =	glGetUniformLocation(mShader->GetHandle(),"uCoeft1y");
	locC4 =	glGetUniformLocation(mShader->GetHandle(),"uCoeft1z");
	locC5 =	glGetUniformLocation(mShader->GetHandle(),"uCoeft2") ;
}

void BloomCombine::CreateATB()
{
	AddVarRW("", TW_TYPE_FLOAT, &m_coefP1, ("label='t1 Coefficient' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &m_coefP1x, ("label='t1x Coefficient' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &m_coefP1y, ("label='t1y Coefficient' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &m_coefP1z, ("label='t1z Coefficient' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &m_coefP2, ("label='t2 Coefficient' step=0.01" + GetNameFormatted()).c_str());	
}

void BloomCombine::PreBindUpdate(wfe::RenderBuffer* source )
{
  for(u32 i = 0; i < 6; i += 2)
  {
    WFE_GRAPHICS->SwitchShader(&WFE_SHADER_MANAGER->GetResource("BlurVertical.xml"));
    glUniform1f(mShader->GetMapSizeHandle(), 1.0f / mRenderBuffer[i]->GetWidth());
    glUniform1i(mShader->GetBlurHalfSizeHandle(), 3);
    mRenderBuffer[i]->Bind();
    glBindTexture(GL_TEXTURE_2D, source->GetColorTextureHandle());
    WFE_GRAPHICS->DrawOverScreen();

    WFE_GRAPHICS->SwitchShader(&WFE_SHADER_MANAGER->GetResource("BlurHorizontal.xml"));
    glUniform1f(mShader->GetMapSizeHandle(), 1.0f / (mRenderBuffer[i]->GetWidth()));
    glUniform1i(mShader->GetBlurHalfSizeHandle(), 3);
    mRenderBuffer[i + 1]->Bind();
    glBindTexture(GL_TEXTURE_2D, mRenderBuffer[i]->GetColorTextureHandle());
    WFE_GRAPHICS->DrawOverScreen();
  }
}

void BloomCombine::EnableUniforms( wfe::RenderBuffer* source)
{
	mShader->EnableTexture(mInputTextureHandle, Shader::WFE_SHADER_MAPTYPE_COLOR);
  mShader->EnableTexture(source->GetColorTextureHandle(),Shader::WFE_SHADER_MAPTYPE_BLOOMZERO);
  mShader->EnableTexture(mRenderBuffer[1]->GetColorTextureHandle(),Shader::WFE_SHADER_MAPTYPE_BLOOMONE);
  mShader->EnableTexture(mRenderBuffer[3]->GetColorTextureHandle(),Shader::WFE_SHADER_MAPTYPE_BLOOMTWO);
  mShader->EnableTexture(mRenderBuffer[5]->GetColorTextureHandle(),Shader::WFE_SHADER_MAPTYPE_BLOOMTHREE);

  glUniform1f(locC1 , m_coefP1);
  glUniform1f(locC2 , m_coefP1x);
  glUniform1f(locC3 , m_coefP1y);
  glUniform1f(locC4 , m_coefP1z);
  glUniform1f(locC5 , m_coefP2);
}

LuminanceThreshold::LuminanceThreshold() : PostProEffect(sType)
{
  mShader = &WFE_SHADER_MANAGER->GetResource("Luminance.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
}


OldFilm::OldFilm() : PostProEffect(sType), mSepiaVaue(1.0f), mNoiseValue(1.0f),mScratchValue(1.0f)
										 , mInnerVignetting(0.5f), mOuterVignetting(0.9f)
										 , mRandomValue(0.5f), mTimeLapse(1.f),m_multiplier(0.f)	
{  
	mShader = &WFE_SHADER_MANAGER->GetResource("OldFilm.xml");

	if(!mShader)
	{
		WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
	}
	
	mSepiaHandle = glGetUniformLocation(mShader->GetHandle(),"uSpeiaValue");
	mNoiseHandle = glGetUniformLocation(mShader->GetHandle(),"uNoiseValue");
	mScratchHandle = glGetUniformLocation(mShader->GetHandle(),"uScratchValue");
	mInnerVignettingHandle = glGetUniformLocation(mShader->GetHandle(),"uInnerVignetting");
	mOuterVignettingHandle = glGetUniformLocation(mShader->GetHandle(),"uOuterVignetting");
	mRandomValueHandle = glGetUniformLocation(mShader->GetHandle(),"uRandomValue");
	mTimeLapseHandle = glGetUniformLocation(mShader->GetHandle(),"uTimeLapse");

	
}

void OldFilm::CreateATB()
{
	AddVarRW("", TW_TYPE_FLOAT, &mSepiaVaue, ("label='Sepia Value' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &mNoiseValue, ("label='Noise Value' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &mScratchValue, ("label='Scratch Value' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &mInnerVignetting, ("label='Inner Vignetting ' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &mOuterVignetting, ("label='Outer Vignetting' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &mRandomValue, ("label='Random Value' step=0.01" + GetNameFormatted()).c_str());
	AddVarRW("", TW_TYPE_FLOAT, &mTimeLapse, ("label='Time Lapse' step=0.01" + GetNameFormatted()).c_str());
}

void OldFilm::EnableUniforms( wfe::RenderBuffer* source )
{
	// Enable current texture map
	mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

	//0.1-1.1
	m_multiplier =  cos(WFE_FRC->GetLevelTime() / 3.f + 7.f);
	
	// Send uniforms
	glUniform1f(mSepiaHandle,			mSepiaVaue);
	glUniform1f(mNoiseHandle,			.5f + cos(WFE_FRC->GetLevelTime() / 2.f + 3.f ) * mNoiseValue) ;
	glUniform1f(mScratchHandle,		.5f + 	cos(WFE_FRC->GetLevelTime() * 10.f  + 11.f) * mScratchValue);
	glUniform1f(mInnerVignettingHandle, cos(WFE_FRC->GetLevelTime()) * mInnerVignetting + .1f);
	glUniform1f(mOuterVignettingHandle, mOuterVignetting);
	glUniform1f(mRandomValueHandle,		m_multiplier * mRandomValue / 2.f) ;
	glUniform1f(mTimeLapseHandle,	.5f + 	cos(WFE_FRC->GetLevelTime() * 10.f + 13.f) * mTimeLapse);
}

PPSSAO::PPSSAO(): PostProEffect(sType), 
  mAOStrength(0.33f), 
  mAOSampleDistance(8.f),
  mAOScale(20.0f),
  mAOSamples(6), 
  mAOSamples2(2)
{  
  mShader = &WFE_SHADER_MANAGER->GetResource("SimpleAttribs.xml");

  if(!mShader)
  {
    WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
  }
}

PPSSAO::~PPSSAO()
{
  WFE_GRAPHICS->SetAOActive(false);
}

void PPSSAO::CreateATB()
{
  AddVarRW("", TW_TYPE_FLOAT, &WFE_GRAPHICS->mAOStrength, ("label='AO Strength' step=0.01" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_FLOAT, &WFE_GRAPHICS->mAOSampleDistance, ("label='Sample Distance' step=0.01" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_FLOAT, &WFE_GRAPHICS->mAOScale, ("label='AO Scale' step=0.01" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_INT32, &WFE_GRAPHICS->mAOSamples, ("label='AO Samples' min=1" + GetNameFormatted()).c_str());
  AddVarRW("", TW_TYPE_INT32, &WFE_GRAPHICS->mAOSamples2, ("label='AO Samples2' min=1" + GetNameFormatted()).c_str());
}

void PPSSAO::EnableUniforms( wfe::RenderBuffer* source)
{
  PostProEffect::EnableUniforms(source);
  // hacking it in, not sure why it isnt working properly when i add it in stack
  WFE_GRAPHICS->SetAOActive(true);
}


Fog::Fog() : PostProEffect(sType), mCameraViewVecHandle(-1), mCameraEyePosHandle(-1)
{  
    mShader = &WFE_SHADER_MANAGER->GetResource("Fog.xml");

    if(!mShader)
    {
        WFE_LOGGER_POPUP << "Shader file can't be created for post processing effect" << std::endl;
    }

    mCameraViewVecHandle = glGetUniformLocation(mShader->GetHandle(),"uCameraViewVec");
    mCameraEyePosHandle = glGetUniformLocation(mShader->GetHandle(),"uCameraEyePos");  
    mCameraNear = glGetUniformLocation(mShader->GetHandle(),"uCameraNear");  
    mCameraFar = glGetUniformLocation(mShader->GetHandle(),"uCameraFar");
    if(mCameraEyePosHandle == -1)
    {
        assert(0);
    }
    if(mCameraViewVecHandle == -1)
    {
        assert(0);
    }
}

void Fog::CreateATB()
{
//     AddVarRW("", TW_TYPE_FLOAT, &mSepiaVaue, ("label='Sepia Value' step=0.01" + GetNameFormatted()).c_str());
//     AddVarRW("", TW_TYPE_FLOAT, &mNoiseValue, ("label='Noise Value' step=0.01" + GetNameFormatted()).c_str());
//     AddVarRW("", TW_TYPE_FLOAT, &mScratchValue, ("label='Scratch Value' step=0.01" + GetNameFormatted()).c_str());
//     AddVarRW("", TW_TYPE_FLOAT, &mInnerVignetting, ("label='Inner Vignetting ' step=0.01" + GetNameFormatted()).c_str());
//     AddVarRW("", TW_TYPE_FLOAT, &mOuterVignetting, ("label='Outer Vignetting' step=0.01" + GetNameFormatted()).c_str());
//     AddVarRW("", TW_TYPE_FLOAT, &mRandomValue, ("label='Random Value' step=0.01" + GetNameFormatted()).c_str());
//     AddVarRW("", TW_TYPE_FLOAT, &mTimeLapse, ("label='Time Lapse' step=0.01" + GetNameFormatted()).c_str());
}

void Fog::EnableUniforms( wfe::RenderBuffer* source )
{
    // Enable current texture map
    mShader->EnableTexture(source->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_COLOR);

    // Enable the original depth texture map
    mShader->EnableTexture(WFE_GRAPHICS->GetDepthAndNormalBuffer()->GetColorTextureHandle(), Shader::WFE_SHADER_MAPTYPE_SHADOW);

    Camera * camera = WFE_CAMERA->GetActiveCamera();

	Vec3 nearCenter = camera->GetViewVec() * camera->GetNearPlaneDistance() * 1.2f;
    //Vec3 viewVec = camera->GetViewVec();
    nearCenter = Vec3(glm::inverse(camera->GetWorldToViewMtx()) * Vec4(nearCenter, 0));
    glUniform3f(mCameraViewVecHandle, -nearCenter.x, -nearCenter.y , -nearCenter.z);// -viewVec.x,-viewVec.y, -viewVec.z);
    Vec3 camPos = camera->GetEyePos();
    glUniform3f(mCameraEyePosHandle, camPos.x, camPos.y, camPos.z);
    glUniform1f(mCameraNear, camera->GetNearPlaneDistance());
    glUniform1f(mCameraFar, camera->GetFarPlaneDistance());

  
    float distEyeToNearPlane = glm::length(nearCenter - camera->GetEyePos());
	 float a = camera->GetFov();

    float nearHalfHeight = tan(a * 0.5f/180.f) * distEyeToNearPlane;
	float w = static_cast<float>(source->GetWidth());
	float h = static_cast<float>(source->GetHeight()); 
    float nearHalfWidth = nearHalfHeight * ( w/h )/*WFE_WINDOW->GetResoWidth() / WFE_WINDOW->GetResoHeight()*/;

    GLint pos = glGetUniformLocation(mShader->GetHandle(),"uNearTopLeft");
    Vec3 corner = nearCenter + (camera->GetUpVec() * nearHalfHeight) - (camera->GetSideVec() * nearHalfWidth);
    corner = Vec3(glm::inverse(camera->GetWorldToViewMtx()) * Vec4(corner, 0));
    glUniform3f(pos, corner.x, corner.y, corner.z);
    corner = nearCenter + (camera->GetUpVec() * nearHalfHeight) + (camera->GetSideVec() * nearHalfWidth);
    corner = Vec3(glm::inverse(camera->GetWorldToViewMtx()) * Vec4(corner, 0));
    pos = glGetUniformLocation(mShader->GetHandle(),"uNearTopRight");
    glUniform3f(pos, corner.x, corner.y, corner.z);
    corner = nearCenter - (camera->GetUpVec() * nearHalfHeight) - (camera->GetSideVec() * nearHalfWidth);
    corner = Vec3(glm::inverse(camera->GetWorldToViewMtx()) * Vec4(corner, 0));
    pos = glGetUniformLocation(mShader->GetHandle(),"uNearBottomLeft");
    glUniform3f(pos, corner.x, corner.y, corner.z);
    corner = nearCenter - (camera->GetUpVec() * nearHalfHeight) + (camera->GetSideVec() * nearHalfWidth);
    corner = Vec3(glm::inverse(camera->GetWorldToViewMtx()) * Vec4(corner, 0));
    pos = glGetUniformLocation(mShader->GetHandle(),"uNearBottomRight");
    glUniform3f(pos, corner.x, corner.y, corner.z);

//     //0.1-1.1
//     m_multiplier =  cos(WFE_FRC->GetLevelTime() / 3.f + 7.f);
// 
//     // Send uniforms
//     glUniform1f(mSepiaHandle,			mSepiaVaue);
//     glUniform1f(mNoiseHandle,			.5f + cos(WFE_FRC->GetLevelTime() / 2.f + 3.f ) * mNoiseValue) ;
//     glUniform1f(mScratchHandle,		.5f + 	cos(WFE_FRC->GetLevelTime() * 10.f  + 11.f) * mScratchValue);
//     glUniform1f(mInnerVignettingHandle, cos(WFE_FRC->GetLevelTime()) * mInnerVignetting + .1f);
//     glUniform1f(mOuterVignettingHandle, mOuterVignetting);
//     glUniform1f(mRandomValueHandle,		m_multiplier * mRandomValue / 2.f) ;
//     glUniform1f(mTimeLapseHandle,	.5f + 	cos(WFE_FRC->GetLevelTime() * 10.f + 13.f) * mTimeLapse);
}