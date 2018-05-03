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

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
#include <cassert>
#include <csignal>
#include <cstdarg>

#ifdef ONLY_ENC_FRAMES_NUM
#undef ONLY_ENC_FRAMES_NUM
#endif//ONLY_ENC_FRAMES_NUM
#define ONLY_ENC_FRAMES_NUM INT_MAX // 2, INT_MAX // type the num you try to encode here, 2, 10, etc

//#define STICK_STREAM_SIZE

#include "measure_time.h"
#include "read_config.h"

#include "typedefs.h"

#include "codec_def.h"
#include "codec_api.h"
#include "extern.h"
#include "macros.h"
#include "wels_const.h"

#include "mt_defs.h"
#include "WelsThreadLib.h"

#if defined(__linux__) || defined(__unix__)
#define _FILE_OFFSET_BITS 64
#endif

#include <iostream>
#include <welsEncoderExt.h>

using namespace std;
using namespace WelsEnc;

/*
 *  Layer Context
 */
typedef struct LayerpEncCtx_s {
  int32_t       iDLayerQp;
  SSliceArgument  sSliceArgument;
} SLayerPEncCtx;

typedef struct tagFilesSet {
  string strBsFile;
  string strSeqFile;    // for cmd lines
  string strLayerCfgFile[MAX_DEPENDENCY_LAYER];
  char   sRecFileName[MAX_DEPENDENCY_LAYER][MAX_FNAME_LEN];
  uint32_t uiFrameToBeCoded;
  bool     bEnableMultiBsFile;
} SFilesSet;

void PrintHelp() {
  printf ("\n Wels SVC Encoder Usage:\n\n");
  printf (" Syntax: welsenc.exe -h\n");

  printf ("\n Supported Options:\n");
  printf ("  -bf          Bit Stream File\n");
  printf ("  -org         Original file, example: -org src.yuv\n");
  printf ("  -sw          the source width\n");
  printf ("  -sh          the source height\n");
  printf ("  -utype       usage type\n");
  printf ("  -savc        simulcast avc\n");
  printf ("  -frms        Number of total frames to be encoded\n");
  printf ("  -frin        input frame rate\n");
  printf ("  -numtl       Temporal layer number (default: 1)\n");
  printf ("  -iper        Intra period (default: -1) : must be a power of 2 of GOP size (or -1)\n");
  printf ("  -nalsize     the Maximum NAL size. which should be larger than the each layer slicesize when slice mode equals to SM_SIZELIMITED_SLICE\n");
  printf ("  -spsid       SPS/PPS id strategy: 0:const, 1: increase, 2: sps list, 3: sps list and pps increase, 4: sps/pps list\n");
  printf ("  -cabac       Entropy coding mode(0:cavlc 1:cabac \n");
  printf ("  -complexity  Complexity mode (default: 0),0: low complexity, 1: medium complexity, 2: high complexity\n");
  printf ("  -denois      Control denoising  (default: 0)\n");
  printf ("  -scene       Control scene change detection (default: 0)\n");
  printf ("  -bgd         Control background detection (default: 0)\n");
  printf ("  -aq          Control adaptive quantization (default: 0)\n");
  printf ("  -ltr         Control long term reference (default: 0)\n");
  printf ("  -ltrnum      Control the number of long term reference((1-4):screen LTR,(1-2):video LTR \n");
  printf ("  -ltrper      Control the long term reference marking period \n");
  printf ("  -threadIdc   0: auto(dynamic imp. internal encoder); 1: multiple threads imp. disabled; > 1: count number of threads \n");
  printf ("  -loadbalancing   0: turn off loadbalancing between slices when multi-threading available; 1: (default value) turn on loadbalancing between slices when multi-threading available\n");
  printf ("  -deblockIdc  Loop filter idc (0: on, 1: off, \n");
  printf ("  -alphaOffset AlphaOffset(-6..+6): valid range \n");
  printf ("  -betaOffset  BetaOffset (-6..+6): valid range\n");
  printf ("  -rc          rate control mode: -1-rc off; 0-quality mode; 1-bitrate mode; 2: buffer based mode,can't control bitrate; 3: bitrate mode based on timestamp input;\n");
  printf ("  -tarb        Overall target bitrate\n");
  printf ("  -maxbrTotal  Overall max bitrate\n");
  printf ("  -maxqp       Maximum Qp (default: %d, or for screen content usage: %d)\n", QP_MAX_VALUE, MAX_SCREEN_QP);
  printf ("  -minqp       Minimum Qp (default: %d, or for screen content usage: %d)\n", QP_MIN_VALUE, MIN_SCREEN_QP);
  printf ("  -numl        Number Of Layers: Must exist with layer_cfg file and the number of input layer_cfg file must equal to the value set by this command\n");
  printf ("  The options below are layer-based: (need to be set with layer id)\n");
  printf ("  -lconfig     (Layer) (spatial layer configure file)\n");
  printf ("  -drec        (Layer) (reconstruction file);example: -drec 0 rec.yuv.  Setting the reconstruction file, this will only functioning when dumping reconstruction is enabled\n");
  printf ("  -dprofile    (Layer) (layer profile);example: -dprofile 0 66.  Setting the layer profile, this should be the same for all layers\n");
  printf ("  -dw          (Layer) (output width)\n");
  printf ("  -dh          (Layer) (output height)\n");
  printf ("  -frout       (Layer) (output frame rate)\n");
  printf ("  -lqp         (Layer) (base quality layer qp : must work with -ldeltaqp or -lqparr)\n");
  printf ("  -ltarb       (Layer) (spatial layer target bitrate)\n");
  printf ("  -lmaxb       (Layer) (spatial layer max bitrate)\n");
  printf ("  -slcmd       (Layer) (spatial layer slice mode): pls refer to layerX.cfg for details ( -slcnum: set target slice num; -slcsize: set target slice size constraint ; -slcmbnum: set the first slice mb num under some slice modes) \n");
  printf ("  -trace       (Level)\n");
  printf ("\n");
}

int ParseCommandLine (int argc, char** argv, SSourcePicture& pSrcPic, SEncParamExt& pSvcParam, SFilesSet& sFileSet) {
  char* pCommand = NULL;
  SLayerPEncCtx sLayerCtx[MAX_SPATIAL_LAYER_NUM];
  int n = 0;
  string str_ ("SlicesAssign");

  while (n < argc) {
    pCommand = argv[n++];

    if (!strcmp (pCommand, "-bf") && (n < argc))
      sFileSet.strBsFile.assign (argv[n++]);
    else if (!strcmp (pCommand, "-utype") && (n < argc))
      pSvcParam.iUsageType = (EUsageType)atoi (argv[n++]);

    else if (!strcmp (pCommand, "-savc") && (n < argc))
      pSvcParam.bSimulcastAVC =  atoi (argv[n++]) ? true : false;

    else if (!strcmp (pCommand, "-org") && (n < argc))
      sFileSet.strSeqFile.assign (argv[n++]);

    else if (!strcmp (pCommand, "-sw") && (n < argc))//source width
      pSrcPic.iPicWidth = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-sh") && (n < argc))//source height
      pSrcPic.iPicHeight = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-frms") && (n < argc))
      sFileSet.uiFrameToBeCoded = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-frin") && (n < argc))
      pSvcParam.fMaxFrameRate = (float) atof (argv[n++]);

    else if (!strcmp (pCommand, "-numtl") && (n < argc))
      pSvcParam.iTemporalLayerNum = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-mfile") && (n < argc))
      sFileSet.bEnableMultiBsFile = atoi (argv[n++]) ? true : false;

    else if (!strcmp (pCommand, "-iper") && (n < argc))
      pSvcParam.uiIntraPeriod = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-nalsize") && (n < argc))
      pSvcParam.uiMaxNalSize = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-spsid") && (n < argc)) {
      int32_t iValue = atoi (argv[n++]);
      switch (iValue) {
      case 0:
        pSvcParam.eSpsPpsIdStrategy  = CONSTANT_ID;
        break;
      case 0x01:
        pSvcParam.eSpsPpsIdStrategy  = INCREASING_ID;
        break;
      case 0x02:
        pSvcParam.eSpsPpsIdStrategy  = SPS_LISTING;
        break;
      case 0x03:
        pSvcParam.eSpsPpsIdStrategy  = SPS_LISTING_AND_PPS_INCREASING;
        break;
      case 0x06:
        pSvcParam.eSpsPpsIdStrategy  = SPS_PPS_LISTING;
        break;
      default:
        pSvcParam.eSpsPpsIdStrategy  = CONSTANT_ID;
        break;
      }
    } else if (!strcmp (pCommand, "-cabac") && (n < argc))
      pSvcParam.iEntropyCodingModeFlag = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-complexity") && (n < argc))
      pSvcParam.iComplexityMode = (ECOMPLEXITY_MODE)atoi (argv[n++]);

    else if (!strcmp (pCommand, "-denois") && (n < argc))
      pSvcParam.bEnableDenoise = atoi (argv[n++]) ? true : false;

    else if (!strcmp (pCommand, "-scene") && (n < argc))
      pSvcParam.bEnableSceneChangeDetect = atoi (argv[n++]) ? true : false;

    else if (!strcmp (pCommand, "-bgd") && (n < argc))
      pSvcParam.bEnableBackgroundDetection = atoi (argv[n++]) ? true : false;

    else if (!strcmp (pCommand, "-aq") && (n < argc))
      pSvcParam.bEnableAdaptiveQuant = atoi (argv[n++]) ? true : false;

    else if (!strcmp (pCommand, "-fs") && (n < argc))
      pSvcParam.bEnableFrameSkip = atoi (argv[n++]) ? true : false;

    else if (!strcmp (pCommand, "-ltr") && (n < argc))
      pSvcParam.bEnableLongTermReference = atoi (argv[n++]) ? true : false;

    else if (!strcmp (pCommand, "-ltrnum") && (n < argc))
      pSvcParam.iLTRRefNum = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-ltrper") && (n < argc))
      pSvcParam.iLtrMarkPeriod = atoi (argv[n++]);

     else if (!strcmp (pCommand, "-deblockIdc") && (n < argc))
      pSvcParam.iLoopFilterDisableIdc = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-alphaOffset") && (n < argc))
      pSvcParam.iLoopFilterAlphaC0Offset = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-betaOffset") && (n < argc))
      pSvcParam.iLoopFilterBetaOffset = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-rc") && (n < argc))
      pSvcParam.iRCMode = static_cast<RC_MODES> (atoi (argv[n++]));

    else if (!strcmp (pCommand, "-tarb") && (n < argc))
      pSvcParam.iTargetBitrate = 1000 * atoi (argv[n++]);

    else if (!strcmp (pCommand, "-maxbrTotal") && (n < argc))
      pSvcParam.iMaxBitrate = 1000 * atoi (argv[n++]);

    else if (!strcmp (pCommand, "-maxqp") && (n < argc))
      pSvcParam.iMaxQp = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-minqp") && (n < argc))
      pSvcParam.iMinQp = atoi (argv[n++]);

    else if (!strcmp (pCommand, "-numl") && (n < argc)) {
      pSvcParam.iSpatialLayerNum = atoi (argv[n++]);
    }
    else if (!strcmp (pCommand, "-dprofile") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->uiProfileIdc = (EProfileIdc) atoi (argv[n++]);
    } else if (!strcmp (pCommand, "-drec") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      const unsigned int iLen = (int) strlen (argv[n]);
      if (iLen >= sizeof (sFileSet.sRecFileName[iLayer]))
        return 1;
      sFileSet.sRecFileName[iLayer][iLen] = '\0';
      strncpy (sFileSet.sRecFileName[iLayer], argv[n++], iLen); // confirmed_safe_unsafe_usage
    } else if (!strcmp (pCommand, "-dw") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->iVideoWidth = atoi (argv[n++]);
    }

    else if (!strcmp (pCommand, "-dh") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->iVideoHeight = atoi (argv[n++]);
    }

    else if (!strcmp (pCommand, "-frout") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->fFrameRate = (float)atof (argv[n++]);
    }

    else if (!strcmp (pCommand, "-lqp") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->iDLayerQp = sLayerCtx[iLayer].iDLayerQp =  atoi (argv[n++]);
    }
    //sLayerCtx[iLayer].num_quality_layers = pDLayer->num_quality_layers = 1;

    else if (!strcmp (pCommand, "-ltarb") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->iSpatialBitrate = 1000 * atoi (argv[n++]);
    }

    else if (!strcmp (pCommand, "-lmaxb") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->iMaxSpatialBitrate = 1000 * atoi (argv[n++]);
    }

    else if (!strcmp (pCommand, "-slcmd") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];

      switch (atoi (argv[n++])) {
      case 0:
        pDLayer->sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
        break;
      case 1:
        pDLayer->sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
        break;
      case 2:
        pDLayer->sSliceArgument.uiSliceMode = SM_RASTER_SLICE;
        break;
      case 3:
        pDLayer->sSliceArgument.uiSliceMode = SM_SIZELIMITED_SLICE;
        break;
      default:
        pDLayer->sSliceArgument.uiSliceMode = SM_RESERVED;
        break;
      }
    }

    else if (!strcmp (pCommand, "-slcsize") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->sSliceArgument.uiSliceSizeConstraint = atoi (argv[n++]);
    }

    else if (!strcmp (pCommand, "-slcnum") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->sSliceArgument.uiSliceNum = atoi (argv[n++]);
    } else if (!strcmp (pCommand, "-slcmbnum") && (n + 1 < argc)) {
      unsigned int iLayer = atoi (argv[n++]);
      SSpatialLayerConfig* pDLayer = &pSvcParam.sSpatialLayers[iLayer];
      pDLayer->sSliceArgument.uiSliceMbNum[0] = atoi (argv[n++]);
    }
  }
  return 0;
}



int FillSpecificParameters (SEncParamExt& sParam) {
  /* Test for temporal, spatial, SNR scalability */
  sParam.iUsageType = CAMERA_VIDEO_REAL_TIME;
  sParam.fMaxFrameRate  = 30.0f;                // input frame rate
  sParam.iPicWidth      = 320;                 // width of picture in samples
  sParam.iPicHeight     = 240;                  // height of picture in samples
  sParam.iTargetBitrate = 1000000;              // target bitrate desired
  sParam.iMaxBitrate    = UNSPECIFIED_BIT_RATE;
  sParam.iRCMode        = RC_QUALITY_MODE;      //  rc mode control
  sParam.iTemporalLayerNum = 3;    // layer number at temporal level
  sParam.iSpatialLayerNum  = 1;    // layer number at spatial level
  sParam.bEnableDenoise    = false;    // denoise control
  sParam.bEnableBackgroundDetection = true; // background detection control
  sParam.bEnableAdaptiveQuant       = true; // adaptive quantization control
  sParam.bEnableFrameSkip           = true; // frame skipping
  sParam.bEnableLongTermReference   = false; // long term reference control
  sParam.iLtrMarkPeriod = 30;
  sParam.uiIntraPeriod  = 320;           // period of Intra frame
  sParam.eSpsPpsIdStrategy = INCREASING_ID;
  sParam.bPrefixNalAddingCtrl = false;
  sParam.iComplexityMode = LOW_COMPLEXITY;
  sParam.bSimulcastAVC         = false;

  sParam.sSpatialLayers[0].uiProfileIdc       = PRO_SCALABLE_BASELINE;
  sParam.sSpatialLayers[0].iVideoWidth        = 320;
  sParam.sSpatialLayers[0].iVideoHeight       = 240;
  sParam.sSpatialLayers[0].fFrameRate         = 25.0f;
  sParam.sSpatialLayers[0].iSpatialBitrate    = 1000000;
  sParam.sSpatialLayers[0].iMaxSpatialBitrate    = UNSPECIFIED_BIT_RATE;
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceNum = 1;

  sParam.fMaxFrameRate = 30.0f;

  return 0;
}

int ProcessEncoding (ISVCEncoder* pPtrEnc, int argc, char** argv) {
  int iRet = 0;

  SEncParamExt sSvcParam{};
  int64_t iStart = 0, iTotal = 0;

  // Preparing encoding process
  int32_t iActualFrameEncodedCount = 0;
  int32_t iFrameIdx = 0;
  int32_t iTotalFrameMax = -1;

  // Inactive with sink with output file handler
  FILE* pFpBs[4] = {nullptr, nullptr, nullptr, nullptr};

#if defined(COMPARE_DATA)
  //For getting the golden file handle
  FILE* fpGolden = NULL;
#endif
#if defined ( STICK_STREAM_SIZE )
  FILE* fTrackStream = fopen ("coding_size.stream", "wb");
#endif
  // for configuration file
  CReadConfig cRdCfg;
  int iParsedNum = 1;

  SFrameBSInfo sFbi{};
  pPtrEnc->GetDefaultParams (&sSvcParam);

  SFilesSet fs;
  memset (&fs.sRecFileName[0][0], 0, sizeof (fs.sRecFileName));
  fs.bEnableMultiBsFile = false;

  FillSpecificParameters (sSvcParam);

  //fill default pSrcPic
  SSourcePicture pSrcPic;
  pSrcPic.iColorFormat = videoFormatI420;
  pSrcPic.uiTimeStamp = 0;

  if (ParseCommandLine (argc - iParsedNum, argv + iParsedNum, pSrcPic, sSvcParam, fs) != 0) {
    printf ("parse pCommand line failed\n");
    return EXIT_FAILURE;
  }

  int g_LevelSetting = WELS_LOG_ERROR;
  pPtrEnc->SetOption (ENCODER_OPTION_TRACE_LEVEL, &g_LevelSetting);

  //finish reading the configurations
  const uint32_t iSourceWidth = pSrcPic.iPicWidth;
  const uint32_t iSourceHeight = pSrcPic.iPicHeight;
  const uint32_t kiPicResSize = iSourceWidth * iSourceHeight * 3 >> 1;

  uint8_t pYUV[kiPicResSize];

  //update pSrcPic
  pSrcPic.iStride[0] = iSourceWidth;
  pSrcPic.iStride[1] = pSrcPic.iStride[2] = pSrcPic.iStride[0] >> 1;

  pSrcPic.pData[0] = pYUV;
  pSrcPic.pData[1] = pSrcPic.pData[0] + (iSourceWidth * iSourceHeight);
  pSrcPic.pData[2] = pSrcPic.pData[1] + (iSourceWidth * iSourceHeight >> 2);

  //update sSvcParam
  sSvcParam.iPicWidth = 0;
  sSvcParam.iPicHeight = 0;
  for (int iLayer = 0; iLayer < sSvcParam.iSpatialLayerNum; iLayer++) {
    SSpatialLayerConfig* pDLayer = &sSvcParam.sSpatialLayers[iLayer];
    sSvcParam.iPicWidth = WELS_MAX (sSvcParam.iPicWidth, pDLayer->iVideoWidth);
    sSvcParam.iPicHeight = WELS_MAX (sSvcParam.iPicHeight, pDLayer->iVideoHeight);
  }
  //if target output resolution is not set, use the source size
  sSvcParam.iPicWidth = (!sSvcParam.iPicWidth) ? iSourceWidth : sSvcParam.iPicWidth;
  sSvcParam.iPicHeight = (!sSvcParam.iPicHeight) ? iSourceHeight : sSvcParam.iPicHeight;

  iTotalFrameMax = (int32_t)fs.uiFrameToBeCoded;
  //  sSvcParam.bSimulcastAVC = true;
  if (cmResultSuccess != pPtrEnc->InitializeExt (&sSvcParam)) { // SVC encoder initialization
    fprintf (stderr, "SVC encoder Initialize failed\n");
    return EXIT_FAILURE;
  }
  for (int iLayer = 0; iLayer < MAX_DEPENDENCY_LAYER; iLayer++) {
    if (fs.sRecFileName[iLayer][0] != 0) {
      SDumpLayer sDumpLayer;
      sDumpLayer.iLayer = iLayer;
      sDumpLayer.pFileName = fs.sRecFileName[iLayer];
      if (cmResultSuccess != pPtrEnc->SetOption (ENCODER_OPTION_DUMP_FILE, &sDumpLayer)) {
        fprintf (stderr, "SetOption ENCODER_OPTION_DUMP_FILE failed!\n");
        return EXIT_FAILURE;
      }
    }
  }
  // Inactive with sink with output file handler
  if (fs.strBsFile.length() > 0) {
    bool bFileOpenErr = false;
    if (sSvcParam.iSpatialLayerNum == 1 || fs.bEnableMultiBsFile == false) {
      pFpBs[0] = fopen (fs.strBsFile.c_str(), "wb");
      bFileOpenErr = (pFpBs[0] == NULL);
    } else { //enable multi bs file writing
      string filename_layer;
      string add_info[4] = {"_layer0", "_layer1", "_layer2", "_layer3"};
      string::size_type found = fs.strBsFile.find_last_of ('.');
      for (int i = 0; i < sSvcParam.iSpatialLayerNum; ++i) {
        filename_layer = fs.strBsFile.insert (found, add_info[i]);
        pFpBs[i] = fopen (filename_layer.c_str(), "wb");
        fs.strBsFile = filename_layer.erase (found, 7);
        bFileOpenErr |= (pFpBs[i] == NULL);
      }
    }
    if (bFileOpenErr) {
      fprintf (stderr, "Can not open file (%s) to write bitstream!\n", fs.strBsFile.c_str());
      return EXIT_FAILURE;
    }
  } else {
    fprintf (stderr, "Don't set the proper bitstream filename!\n");
    return EXIT_FAILURE;
  }

#if defined(COMPARE_DATA)
  //For getting the golden file handle
  if ((fpGolden = fopen (argv[3], "rb")) == NULL) {
    fprintf (stderr, "Unable to open golden sequence file, check corresponding path!\n");
    iRet = 1;
    goto INSIDE_MEM_FREE;
  }
#endif

  FILE* pFileYUV = fopen (fs.strSeqFile.c_str(), "rb");
  if (pFileYUV != NULL) {
#if defined(_WIN32) || defined(_WIN64)
#if _MSC_VER >= 1400
    if (!_fseeki64 (pFileYUV, 0, SEEK_END)) {
      int64_t i_size = _ftelli64 (pFileYUV);
      _fseeki64 (pFileYUV, 0, SEEK_SET);
      iTotalFrameMax = WELS_MAX ((int32_t) (i_size / kiPicResSize), iTotalFrameMax);
    }
#else
    if (!fseek (pFileYUV, 0, SEEK_END)) {
      int64_t i_size = ftell (pFileYUV);
      fseek (pFileYUV, 0, SEEK_SET);
      iTotalFrameMax = WELS_MAX ((int32_t) (i_size / kiPicResSize), iTotalFrameMax);
    }
#endif
#else
    if (!fseeko (pFileYUV, 0, SEEK_END)) {
      int64_t i_size = ftello (pFileYUV);
      fseeko (pFileYUV, 0, SEEK_SET);
      iTotalFrameMax = WELS_MAX ((int32_t) (i_size / kiPicResSize), iTotalFrameMax);
    }
#endif
  } else {
    fprintf (stderr, "Unable to open source sequence file (%s), check corresponding path!\n",
             fs.strSeqFile.c_str());
    iRet = 1;
    goto INSIDE_MEM_FREE;
  }

  iFrameIdx = 0;
  while (iFrameIdx < iTotalFrameMax && (((int32_t)fs.uiFrameToBeCoded <= 0)
                                        || (iFrameIdx < (int32_t)fs.uiFrameToBeCoded))) {

#ifdef ONLY_ENC_FRAMES_NUM
    // Only encoded some limited frames here
    if (iActualFrameEncodedCount >= ONLY_ENC_FRAMES_NUM) {
      break;
    }
#endif//ONLY_ENC_FRAMES_NUM
    bool bCanBeRead = false;
    bCanBeRead = (fread (pYUV, 1, kiPicResSize, pFileYUV) == kiPicResSize);

    if (!bCanBeRead)
      break;
    // To encoder this frame
    iStart = WelsTime();
    pSrcPic.uiTimeStamp = WELS_ROUND (iFrameIdx * (1000 / sSvcParam.fMaxFrameRate));
    int iEncFrames = pPtrEnc->EncodeFrame (&pSrcPic, &sFbi);
    iTotal += WelsTime() - iStart;
    ++ iFrameIdx;
    if (videoFrameTypeSkip == sFbi.eFrameType) {
      continue;
    }

    if (iEncFrames == cmResultSuccess) {
      int iLayer = 0;
      int iFrameSize = 0;
      while (iLayer < sFbi.iLayerNum) {
        SLayerBSInfo* pLayerBsInfo = &sFbi.sLayerInfo[iLayer];
        if (pLayerBsInfo != NULL) {
          int iLayerSize = 0;
          int iNalIdx = pLayerBsInfo->iNalCount - 1;
          do {
            iLayerSize += pLayerBsInfo->pNalLengthInByte[iNalIdx];
            -- iNalIdx;
          } while (iNalIdx >= 0);
#if defined(COMPARE_DATA)
          //Comparing the result of encoder with golden pData
          {
            unsigned char* pUCArry = new unsigned char [iLayerSize];

            fread (pUCArry, 1, iLayerSize, fpGolden);

            for (int w = 0; w < iLayerSize; w++) {
              if (pUCArry[w] != pLayerBsInfo->pBsBuf[w]) {
                fprintf (stderr, "error @frame%d/layer%d/byte%d!!!!!!!!!!!!!!!!!!!!!!!!\n", iFrameIdx, iLayer, w);
                //fprintf(stderr, "%x - %x\n", pUCArry[w], pLayerBsInfo->pBsBuf[w]);
                break;
              }
            }
            fprintf (stderr, "frame%d/layer%d comparation completed!\n", iFrameIdx, iLayer);

            delete [] pUCArry;
          }
#endif
          if (sSvcParam.iSpatialLayerNum == 1 || fs.bEnableMultiBsFile == false)
            fwrite (pLayerBsInfo->pBsBuf, 1, iLayerSize, pFpBs[0]); // write pure bit stream into file
          else { //multi bs file write
            if (pLayerBsInfo->uiSpatialId == 0) {
              unsigned char five_bits = pLayerBsInfo->pBsBuf[4] & 0x1f;
              if ((five_bits == 0x07) || (five_bits == 0x08)) { //sps or pps
                for (int i = 0; i < sSvcParam.iSpatialLayerNum; ++i) {
                  fwrite (pLayerBsInfo->pBsBuf, 1, iLayerSize, pFpBs[i]);
                }
              } else {
                fwrite (pLayerBsInfo->pBsBuf, 1, iLayerSize, pFpBs[0]);
              }
            } else {
              fwrite (pLayerBsInfo->pBsBuf, 1, iLayerSize, pFpBs[pLayerBsInfo->uiSpatialId]);
            }
          }
          iFrameSize += iLayerSize;
        }
        ++ iLayer;
      }
#if defined (STICK_STREAM_SIZE)
      if (fTrackStream) {
        fwrite (&iFrameSize, 1, sizeof (int), fTrackStream);
      }
#endif//STICK_STREAM_SIZE
      ++ iActualFrameEncodedCount; // excluding skipped frame time
    } else {
      fprintf (stderr, "EncodeFrame(), ret: %d, frame index: %d.\n", iEncFrames, iFrameIdx);
    }

  }

  if (iActualFrameEncodedCount > 0) {
    double dElapsed = iTotal / 1e6;
    printf ("Width:\t\t%d\nHeight:\t\t%d\nFrames:\t\t%d\nencode time:\t%f sec\nFPS:\t\t%f fps\n",
            sSvcParam.iPicWidth, sSvcParam.iPicHeight,
            iActualFrameEncodedCount, dElapsed, (iActualFrameEncodedCount * 1.0) / dElapsed);
#if defined (WINDOWS_PHONE)
    g_fFPS = (iActualFrameEncodedCount * 1.0f) / (float) dElapsed;
    g_dEncoderTime = dElapsed;
    g_iEncodedFrame = iActualFrameEncodedCount;
#endif
  }
INSIDE_MEM_FREE:
  for (int i = 0; i < sSvcParam.iSpatialLayerNum; ++i) {
    if (pFpBs[i]) {
      fclose (pFpBs[i]);
      pFpBs[i] = NULL;
    }
  }
#if defined (STICK_STREAM_SIZE)
  if (fTrackStream) {
    fclose (fTrackStream);
    fTrackStream = NULL;
  }
#endif
#if defined (COMPARE_DATA)
  if (fpGolden) {
    fclose (fpGolden);
    fpGolden = NULL;
  }
#endif
  // Destruction memory introduced in this routine
  if (pFileYUV != nullptr) {
    fclose (pFileYUV);
  }

  return iRet;
}

/****************************************************************************
 * main:
 ****************************************************************************/
int main (int argc, char** argv)
{
  CWelsH264SVCEncoder encoder;

  if (argc > 2) {
    ProcessEncoding (&encoder, argc, argv);
  }
  else {
    PrintHelp();
  }

  return EXIT_SUCCESS;
}
