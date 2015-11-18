/******************************************************************************/
/*!
\file   PostProEffect.h
\par    email: y.li\@digipen.edu
\par    name:  Sebastian Li Ye Wei
\par    Project: CS370 
\date   02/08/2013
\brief  
Class that performs post processing effects for the game

All content (c) 2012 DigiPen Institute of Technology Singapore, all rights reserved.
*/
/******************************************************************************/
#ifndef PostProH
#define PostProH

#include "AntTweakBar\AntTweakBar.h"
#include "PostProEffectTypeEnum.h"

/*****************************************************************************/
/*!
  Includes (Only include if required! Forward declare if you can!)
*/
/*****************************************************************************/
namespace wfe
{
  class RenderBuffer;
}


/*****************************************************************************/
/*!
  Forward Declarations
*/
/*****************************************************************************/
namespace wfe
{
  class RenderBuffer;
}


/*****************************************************************************/
/*!
  Type Declarations (Types that are associated with this class declared here)
*/
/*****************************************************************************/
enum PostProcessingCombineModes
{
  POSTPRO_CM_REPLACE,
  POSTPRO_CM_NORMAL,
  POSTPRO_CM_ADD,
  POSTPRO_CM_SUB,
  POSTPRO_CM_NUM
};

class PostProEffect
{
public:
  //////////////////////////////////////////////////////////////////////////
  //Ctors
  PostProEffect(s32 type);
  virtual ~PostProEffect();

  //////////////////////////////////////////////////////////////////////////
  //Member functions
  virtual void Apply(wfe::RenderBuffer*& source, wfe::RenderBuffer*& dest); //Default apply function. uses the post pro effect's shader to draw over the screen

  //////////////////////////////////////////////////////////////////////////
  //Getters (Implement simple ones here)
  PostProcessingCombineModes GetCombineMode() const { return mCombineMode; }

  //////////////////////////////////////////////////////////////////////////
  //Setters (Implement simple ones here)
  void SetCombineMode(PostProcessingCombineModes mode) { mCombineMode = mode; }
  void SetShader(wfe::Shader* shader) { mShader = shader; }

  void CreateATBMain(u32 index);
  virtual void CreateATB() = 0;

  virtual void EnableUniforms(wfe::RenderBuffer* source);
  virtual void PreBindUpdate(wfe::RenderBuffer*) {}

  const std::string& GetName() const { return mName; }
  const std::string& GetNameFormatted() const { return mNameFormatted; }

  static const u32 mObjPerPage = 8;

  s32 GetType() const { return mType; }
  void PushbackPreEffects(s32 type);
protected:
  void AddVarRW(cstr const name, TwType type, void* var, cstr const def);
  
  wfe::Shader* mShader;
  std::vector<PostProEffect*> mPrePostProEffect;
  u32 mInputTextureHandle;
  b8 mKeepInputImage;
private:
  //////////////////////////////////////////////////////////////////////////
  //Private member functions (functions for internal class use only)

  //////////////////////////////////////////////////////////////////////////
  //Private member data
  PostProcessingCombineModes mCombineMode;

  std::string mName;
  std::string mNameFormatted;

  u8* mData;
  s32 mType;
}; // class PostProEffect

class Desaturation : public PostProEffect
{
public:
  Desaturation();

  virtual void CreateATB();
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = DESATURATION;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;

  f32 mSaturation;
};

class SepiaTone : public PostProEffect
{
public:
  SepiaTone();

  virtual void CreateATB() {}
  //virtual void EnableUniforms(wfe::RenderBuffer* source);

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = SEPIA_TONE;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;
};

class BlurHorizontal : public PostProEffect
{
public:
  BlurHorizontal();

  virtual void CreateATB();
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = BLUR_HORIZONTAL;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;
   s32 mHalfSize;
private:
 
  f32 mBlurCutoff;
  b8 mInvert;
  b8 mApplyNaiveDOF;
  u32 mBlurCutoffHandle;
  u32 mInvertHandle;
  u32 mApplyNaiveDOFHandle;
};

class BlurVertical : public PostProEffect
{
public:
  BlurVertical();

  virtual void CreateATB();
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = BLUR_VERTICAL;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;
  s32 mHalfSize;
private:
  
  f32 mBlurCutoff;
  b8 mInvert;
  b8 mApplyNaiveDOF;
  u32 mBlurCutoffHandle;
  u32 mInvertHandle;
  u32 mApplyNaiveDOFHandle;
};

class BlackWhite : public PostProEffect
{
public:
  BlackWhite();

  virtual void CreateATB();
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = BLACK_WHITE;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;

private:
  f32 mTolerance;
};

class UnsharpMaskingDepth : public PostProEffect
{
public:
    UnsharpMaskingDepth();

    virtual void CreateATB();
    virtual void EnableUniforms(wfe::RenderBuffer* source);

    //////////////////////////////////////////////////////////////////////////
    static const s32 sType = UNSHARP_MASKING_DEPTH;
    //Static page size variable. This determines how many objects the object
    //allocator places on one page
    //Note: Components MUST have this!
    static const u32 mObjPerPage = 8;

private:
    float mLambda;
    GLint mLambdaHandle;
    s32 mHalfSize;
};


class GaussianBlur : public PostProEffect
{
public:
    GaussianBlur();

    virtual void CreateATB();
    virtual void EnableUniforms(wfe::RenderBuffer* source);

    //////////////////////////////////////////////////////////////////////////
    static const s32 sType = GAUSSIAN_BLUR;
    //Static page size variable. This determines how many objects the object
    //allocator places on one page
    //Note: Components MUST have this!
    static const u32 mObjPerPage = 8;

    f32 mRadius;
};

class Laplacian : public PostProEffect
{
public:
    Laplacian();

    virtual void CreateATB();
    virtual void EnableUniforms(wfe::RenderBuffer* source);

    //////////////////////////////////////////////////////////////////////////
    static const s32 sType = LAPLACIAN;
    //Static page size variable. This determines how many objects the object
    //allocator places on one page
    //Note: Components MUST have this!
    static const u32 mObjPerPage = 8;

private:
};

class Sobel : public PostProEffect
{
public:
    Sobel();

    virtual void CreateATB();
    virtual void EnableUniforms(wfe::RenderBuffer* source);

    //////////////////////////////////////////////////////////////////////////
    static const s32 sType = SOBEL;
    //Static page size variable. This determines how many objects the object
    //allocator places on one page
    //Note: Components MUST have this!
    static const u32 mObjPerPage = 8;

private:
};

class UnsharpMasking : public PostProEffect
{
public:
  UnsharpMasking();

  virtual void CreateATB();
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = UNSHARP_MASKING;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;

private:
  float mWeightage;
  GLint mWeightageHandle;
};

class Negative : public PostProEffect
{
public:
  Negative();

  virtual void CreateATB() {}

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = NEGATIVE;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;
};

class HueChange : public PostProEffect
{
public:
  HueChange();

  virtual void CreateATB();
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = HUE_CHANGE;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;
private:
  f32 mHue;
  f32 mSaturation;
  f32 mValue;
  GLint mHueHandle;
  GLint mSaturationHandle;
  GLint mValueHandle;
};

class RealisticDOF : public PostProEffect
{
public:
  RealisticDOF();

  virtual void CreateATB();
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = REALISTIC_DOF;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;
private:
  f32 mBias;
  b8 mInvert;
  u32 mInvertHandle;
  u32 mBiasHandle;
};

class BlurHorizontalDepth : public PostProEffect
{
public:
  BlurHorizontalDepth();

  virtual void CreateATB() {}
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  friend class UnsharpMaskingDepth;

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = BLUR_HORIZONTAL_DEPTH;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;

private:
  s32 mHalfSize;
  u32 mApplyNaiveDOFHandle;
};

class BlurVerticalDepth : public PostProEffect
{
public:
  BlurVerticalDepth();

  virtual void CreateATB() {}
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  friend class UnsharpMaskingDepth;
  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = BLUR_VERTICAL_DEPTH;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;

private:
  s32 mHalfSize;
  u32 mApplyNaiveDOFHandle;
};

class AdditiveNoise : public PostProEffect
{
public:
    AdditiveNoise();

    virtual void CreateATB();
    virtual void EnableUniforms(wfe::RenderBuffer* source);

    //////////////////////////////////////////////////////////////////////////
    static const s32 sType = ADDITIVE_NOISE;
    //Static page size variable. This determines how many objects the object
    //allocator places on one page
    //Note: Components MUST have this!
    static const u32 mObjPerPage = 8;

private:
    u32 mNoisyTextureHandle;

    GLint mOffsetXHandle;
    GLint mOffsetYHandle;
    GLint mBiasHandle;

    f32 mBias;
};



class BloomCombine : public PostProEffect
{
public:
	BloomCombine();

	virtual void CreateATB();
	virtual void EnableUniforms(wfe::RenderBuffer* source);
	virtual void PreBindUpdate(wfe::RenderBuffer* source  );
	//////////////////////////////////////////////////////////////////////////
	static const s32 sType = BLOOM;
	//Static page size variable. This determines how many objects the object
	//allocator places on one page
	//Note: Components MUST have this!
	static const u32 mObjPerPage = 8;

private:
   wfe::RenderBuffer* mRenderBuffer[6];

	 GLint locC1, locC2, locC3, locC4,locC5;
	 float m_coefP1, m_coefP1x, m_coefP1y, m_coefP1z, m_coefP2;

};

class LuminanceThreshold : public PostProEffect
{
public:
  LuminanceThreshold();

  virtual void CreateATB() {}

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = LUMINANCE_THRESHOLD;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;
};

class OldFilm : public PostProEffect
{
public:
	OldFilm();

	virtual void CreateATB();
	virtual void EnableUniforms(wfe::RenderBuffer* source);

	//////////////////////////////////////////////////////////////////////////
	static const s32 sType = OLD_FILM;
	//Static page size variable. This determines how many objects the object
	//allocator places on one page
	//Note: Components MUST have this!
	static const u32 mObjPerPage = 8;

private:
	GLint mSepiaHandle, mNoiseHandle, mScratchHandle,
		  mInnerVignettingHandle, mOuterVignettingHandle,
		  mRandomValueHandle, mTimeLapseHandle;
	float mSepiaVaue, mNoiseValue, mScratchValue,
		  mInnerVignetting, mOuterVignetting, 
		  mRandomValue, mTimeLapse, m_multiplier;


};

class PPSSAO : public PostProEffect
{
public:
  PPSSAO();
  ~PPSSAO();

  virtual void CreateATB();
  virtual void EnableUniforms(wfe::RenderBuffer* source);

  //////////////////////////////////////////////////////////////////////////
  static const s32 sType = SSAO;
  //Static page size variable. This determines how many objects the object
  //allocator places on one page
  //Note: Components MUST have this!
  static const u32 mObjPerPage = 8;

private:
  f32 mAOStrength;
  f32 mAOSampleDistance;
  f32 mAOScale;
  s32 mAOSamples;
  s32 mAOSamples2;
};

class Fog : public PostProEffect
{
public:
    Fog();

    virtual void CreateATB();
    virtual void EnableUniforms(wfe::RenderBuffer* source);

    //////////////////////////////////////////////////////////////////////////
    static const s32 sType = FOG;
    //Static page size variable. This determines how many objects the object
    //allocator places on one page
    //Note: Components MUST have this!
    static const u32 mObjPerPage = 8;

private:
    GLint mCameraViewVecHandle;
    GLint mCameraEyePosHandle;
    GLint mCameraNear;
    GLint mCameraFar;
};


#endif // PostProH