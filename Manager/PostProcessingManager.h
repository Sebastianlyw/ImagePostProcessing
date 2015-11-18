/******************************************************************************/
/*!
\file   PostProcessingManager.h
\par    email: y.li\@digipen.edu
\par    name:  Sebastian Li Ye Wei
\par    Project: CS370 
\date   02/08/2013
\brief  
  Class that performs post processing effects for the game
  
All content (c) 2012 DigiPen Institute of Technology Singapore, all rights reserved.
*/
/******************************************************************************/
#ifndef POSTPROCESSINGMANAGER_H
#define POSTPROCESSINGMANAGER_H

/*****************************************************************************/
/*!
  Includes (Only include if required! Forward declare if you can!)
*/
/*****************************************************************************/
#include "PostProEffectTypeEnum.h"
#include "AntTweakBar\AntTweakBar.h"

/*****************************************************************************/
/*!
  Forward Declarations
*/
/*****************************************************************************/
namespace wfe
{
  class RenderBuffer;

  template<typename>
  class FactoryPlant;
}

class PostProEffect;

/*****************************************************************************/
/*!
  Type Declarations (Types that are associated with this class declared here)
*/
/*****************************************************************************/
typedef std::map<s32, wfe::FactoryPlant<PostProEffect>*> PostProEffectFactoryContainer;
typedef std::vector<PostProEffect*> PostProEffectContainer;
typedef std::vector<PostProEffect*>::iterator PostProEffectContainerIt;

class PostProcessingManager
{
public:
  //////////////////////////////////////////////////////////////////////////
  //Ctors
  PostProcessingManager();
  virtual ~PostProcessingManager();

  //////////////////////////////////////////////////////////////////////////
  //Member functions
  virtual void ApplyPostProEffects();

  //Clears all post pro effects
  void ClearPostProEffects();

  //Use this function to add new post pro effect to the effect stack
  void PushPostProEffect(PostProEffect* effect);

  //Removes the last effect
  void PopPostProEffect();

  // remove any post processing effect of given index, returns true if it was possible to remove it
  // yes i noe vector shldnt be removed this way but i dun really care about tat
  b8 RemovePostProEffect(u32 index);

  //////////////////////////////////////////////////////////////////////////
  //Getters (Implement simple ones here)
  b8 GetDrawDepthTexture() const { return mDrawDepthTexture; }
  GLuint GetOriginalTextureHandle() const { return mOriginalTextureHandle; }
  const PostProEffectContainer& GetPostProEffectContainer() const { return mPostProEffects; }

  //////////////////////////////////////////////////////////////////////////
  //Setters (Implement simple ones here)
  void SetDrawDepthTexture(b8 draw) { mDrawDepthTexture = draw; }

  static PostProEffectFactoryContainer mPostProEffectFactoryContainer;
  static TwBar* sStackManagerBar;
  static TwBar* sStackBar;
private:
  //////////////////////////////////////////////////////////////////////////
  //Private member functions (functions for internal class use only)

  //////////////////////////////////////////////////////////////////////////
  //Private member data
  wfe::RenderBuffer* mSourceBuffer;
  wfe::RenderBuffer* mDestBuffer;
  GLuint mOriginalTextureHandle;
  b8 mDrawDepthTexture;
  u8* mData;

  PostProEffectContainer mPostProEffects;
}; // class PostProcessingManager

#endif // POSTPROCESSINGMANAGER_H