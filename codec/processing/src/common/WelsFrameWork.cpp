/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <iostream>
#include "WelsFrameWork.h"

WELSVP_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////

CVpFrameWork::CVpFrameWork() :
    _denoiser{0},
    _sceneChangeDetectorVideo{METHOD_SCENE_CHANGE_DETECTION_VIDEO, 0},
    _sceneChangeDetectorScreen{METHOD_SCENE_CHANGE_DETECTION_SCREEN, 0},
    _downsampling{0},
    _vaaCalculation{0},
    _backgroundDetection{0},
    _adaptiveQuantization{0},
    _complexityAnalysis{0},
    _complexityAnalysisScreen{0},
    _rotating{0},
    _scrollDetection{0}
{
    //ctor
}

CVpFrameWork::~CVpFrameWork() {
  for (int32_t i = 0; i < MAX_STRATEGY_NUM; i++) {
        auto *result = getStrategy(static_cast<EMethods>(i));
        if (result) {
          Uninit (result->m_eMethod);
        }
  }

}

EResult CVpFrameWork::Init (int32_t iType, void* pCfg) {
  EResult eReturn   = RET_SUCCESS;

  Uninit (iType);

  IStrategy* pStrategy = getStrategy(static_cast<EMethods>(iType));
  if (pStrategy)
    eReturn = pStrategy->Init (0, pCfg);


  return eReturn;
}

EResult CVpFrameWork::Uninit (int32_t iType) {
  EResult eReturn        = RET_SUCCESS;

  IStrategy* pStrategy = getStrategy(static_cast<EMethods>(iType));
  if (pStrategy)
    eReturn = pStrategy->Uninit (0);


  return eReturn;
}

EResult CVpFrameWork::Flush (int32_t iType) {
  EResult eReturn        = RET_SUCCESS;

  return eReturn;
}

EResult CVpFrameWork::Process (int32_t iType, SPixMap* pSrcPixMap, SPixMap* pDstPixMap) {
  EResult eReturn        = RET_NOTSUPPORTED;
  EMethods eMethod    = WelsVpGetValidMethod (iType);
  SPixMap sSrcPic;
  SPixMap sDstPic;
  memset (&sSrcPic, 0, sizeof (sSrcPic)); // confirmed_safe_unsafe_usage
  memset (&sDstPic, 0, sizeof (sDstPic)); // confirmed_safe_unsafe_usage

  if (pSrcPixMap) sSrcPic = *pSrcPixMap;
  if (pDstPixMap) sDstPic = *pDstPixMap;
  if (!CheckValid (eMethod, sSrcPic, sDstPic))
    return RET_INVALIDPARAM;


  IStrategy* pStrategy = getStrategy(static_cast<EMethods>(iType));
  if (pStrategy)
    eReturn = pStrategy->Process (0, &sSrcPic, &sDstPic);


  return eReturn;
}

EResult CVpFrameWork::Get (int32_t iType, void* pParam) {
  EResult eReturn        = RET_SUCCESS;

  if (!pParam)
    return RET_INVALIDPARAM;


    IStrategy* pStrategy = getStrategy(static_cast<EMethods>(iType));
    if (pStrategy)
    eReturn = pStrategy->Get (0, pParam);


  return eReturn;
}

EResult CVpFrameWork::Set (int32_t iType, void* pParam) {
  EResult eReturn        = RET_SUCCESS;
  const int32_t iCurIdx = static_cast<int32_t>(iType % METHOD_MASK) - 1;

  if (!pParam)
    return RET_INVALIDPARAM;


    IStrategy* pStrategy = getStrategy(static_cast<EMethods>(iType));
  if (pStrategy)
    eReturn = pStrategy->Set (0, pParam);


  return eReturn;
}

EResult CVpFrameWork::SpecialFeature (int32_t iType, void* pIn, void* pOut) {
  EResult eReturn        = RET_SUCCESS;

  return eReturn;
}

bool  CVpFrameWork::CheckValid (EMethods eMethod, SPixMap& pSrcPixMap, SPixMap& pDstPixMap) {
  bool eReturn = false;

  if (eMethod == METHOD_NULL)
    goto exit;

  if (eMethod != METHOD_COLORSPACE_CONVERT) {
    if (pSrcPixMap.pPixel[0]) {
      if (pSrcPixMap.eFormat != VIDEO_FORMAT_I420 && pSrcPixMap.eFormat != VIDEO_FORMAT_YV12)
        goto exit;
    }
    if (pSrcPixMap.pPixel[0] && pDstPixMap.pPixel[0]) {
      if (pDstPixMap.eFormat != pSrcPixMap.eFormat)
        goto exit;
    }
  }

  if (pSrcPixMap.pPixel[0]) {
    if (pSrcPixMap.sRect.iRectWidth <= 0 || pSrcPixMap.sRect.iRectHeight <= 0
        || pSrcPixMap.sRect.iRectWidth * pSrcPixMap.sRect.iRectHeight > (MAX_MBS_PER_FRAME << 8))
      goto exit;
    if (pSrcPixMap.sRect.iRectTop >= pSrcPixMap.sRect.iRectHeight
        || pSrcPixMap.sRect.iRectLeft >= pSrcPixMap.sRect.iRectWidth || pSrcPixMap.sRect.iRectWidth > pSrcPixMap.iStride[0])
      goto exit;
  }
  if (pDstPixMap.pPixel[0]) {
    if (pDstPixMap.sRect.iRectWidth <= 0 || pDstPixMap.sRect.iRectHeight <= 0
        || pDstPixMap.sRect.iRectWidth * pDstPixMap.sRect.iRectHeight > (MAX_MBS_PER_FRAME << 8))
      goto exit;
    if (pDstPixMap.sRect.iRectTop >= pDstPixMap.sRect.iRectHeight
        || pDstPixMap.sRect.iRectLeft >= pDstPixMap.sRect.iRectWidth || pDstPixMap.sRect.iRectWidth > pDstPixMap.iStride[0])
      goto exit;
  }
  eReturn = true;

exit:
  return eReturn;
}

IStrategy *CVpFrameWork::getStrategy(const EMethods eMethod) {

    switch(eMethod) {
        case METHOD_NULL:
            return nullptr;

        case METHOD_COLORSPACE_CONVERT:
            //not implemented
            return nullptr;

        case METHOD_DENOISE:
            return &_denoiser;

        case METHOD_SCENE_CHANGE_DETECTION_VIDEO:
            return &_sceneChangeDetectorVideo;

        case METHOD_SCENE_CHANGE_DETECTION_SCREEN:
            return &_sceneChangeDetectorScreen;

        case METHOD_DOWNSAMPLE:
            return &_downsampling;

        case METHOD_VAA_STATISTICS:
            return &_vaaCalculation;

        case METHOD_BACKGROUND_DETECTION:
            return &_backgroundDetection;

        case METHOD_ADAPTIVE_QUANT:
            return &_adaptiveQuantization;

        case METHOD_COMPLEXITY_ANALYSIS:
            return &_complexityAnalysis;

        case METHOD_COMPLEXITY_ANALYSIS_SCREEN:
            return &_complexityAnalysisScreen;

        case METHOD_IMAGE_ROTATE:
            return &_rotating;

        case METHOD_SCROLL_DETECTION:
            return &_scrollDetection;

        case METHOD_MASK:
            return nullptr;
    }
}

WELSVP_NAMESPACE_END
