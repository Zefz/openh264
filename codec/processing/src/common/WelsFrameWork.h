/*!
 * \copy
 *     Copyright (c)  2011-2013, Cisco Systems
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
 * \file        :  WelsFrameWork.h
 *
 * \brief       :  framework of wels video processor class
 *
 * \date        :  2011/01/04
 *
 * \description :
 *
 *************************************************************************************
 */

#pragma once

#include <Strategy.hpp>
#include "../scenechangedetection/SceneChangeDetection.h"
#include "../downsample/downsample.h"
#include "../vaacalc/vaacalculation.h"
#include "../backgrounddetection/BackgroundDetection.h"
#include "../adaptivequantization/AdaptiveQuantization.h"
#include "../complexityanalysis/ComplexityAnalysis.h"
#include "../imagerotate/imagerotate.h"
#include "../scrolldetection/ScrollDetection.h"
#include "../denoise/denoise.h"

WELSVP_NAMESPACE_BEGIN

class CVpFrameWork : public IWelsVP {
 public:
  CVpFrameWork();
  ~CVpFrameWork() override;

 public:
  EResult Init (int32_t iType, void* pCfg) override;

  EResult Uninit (int32_t iType) override;

  EResult Flush (int32_t iType) override;

  EResult Process (int32_t iType, SPixMap* pSrc, SPixMap* pDst) override;

  EResult Get (int32_t iType, void* pParam) override;

  EResult Set (int32_t iType, void* pParam) override;

  EResult SpecialFeature (int32_t iType, void* pIn, void* pOut) override;

 private:
  bool  CheckValid (EMethods eMethod, SPixMap& sSrc, SPixMap& sDst);

  IStrategy* getStrategy(const EMethods eMethod);

private:
   CSceneChangeDetection<WelsVP::CSceneChangeDetectorVideo> _sceneChangeDetectorVideo;
   CSceneChangeDetection<CSceneChangeDetectorScreen> _sceneChangeDetectorScreen;
   CDownsampling _downsampling;
   CVAACalculation _vaaCalculation;
   CBackgroundDetection _backgroundDetection;
   CAdaptiveQuantization _adaptiveQuantization;
   CComplexityAnalysis _complexityAnalysis;
   CComplexityAnalysisScreen _complexityAnalysisScreen;
   CImageRotating _rotating;
   CScrollDetection _scrollDetection;
   CDenoiser _denoiser;
};

WELSVP_NAMESPACE_END
