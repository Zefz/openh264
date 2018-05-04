#include <iostream>
#include "OpenH264Decoder.hpp"
#include "version.h"

#include "decoder.h"
#include "decoder_core.h"
#include "error_concealment.h"

#include "measure_time.h"

#if defined(_WIN32) /*&& defined(_DEBUG)*/
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#endif

namespace staaker
{

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/***************************************************************************
*   Description:
*       class OpenH264Decoder constructor function, do initialization  and
*       alloc memory required
*
*   Input parameters: none
*
*   return: none
***************************************************************************/
    OpenH264Decoder::OpenH264Decoder ()
            : _decoderContext{},
              m_pWelsTrace{}
    {

        m_pWelsTrace.SetCodecInstance (this);
        m_pWelsTrace.SetTraceLevel (WELS_LOG_ERROR);

        WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO, "OpenH264Decoder::OpenH264Decoder() entry");
    }

/***************************************************************************
*   Description:
*       class OpenH264Decoder destructor function, destroy allocced memory
*
*   Input parameters: none
*
*   return: none
***************************************************************************/
    OpenH264Decoder::~OpenH264Decoder() {
        WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO, "OpenH264Decoder::~OpenH264Decoder()");

        WelsEndDecoder(_decoderContext);
    }

    long OpenH264Decoder::Initialize (const SDecodingParam* pParam) {
        int iRet;

        if (pParam == nullptr) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_ERROR, "OpenH264Decoder::Initialize(), invalid input argument.");
            return cmInitParaError;
        }

        // H.264 decoder initialization,including memory allocation,then open it ready to decode
        iRet = InitDecoder (pParam);
        if (iRet)
            return iRet;

        return cmResultSuccess;
    }

    long OpenH264Decoder::Uninitialize() {
        return ERR_NONE;
    }

// the return value of this function is not suitable, it need report failure info to upper layer.
    int32_t OpenH264Decoder::InitDecoder (const SDecodingParam* pParam) {

        WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO,
                 "OpenH264Decoder::init_decoder(), openh264 codec version = %s, ParseOnly = %s",
                 VERSION_NUMBER, pParam->bParseOnly ? "true" : "false");

        //reset decoder context
        _decoderContext = {};

        //fill in default value into context
        WelsDecoderDefaults (_decoderContext, &m_pWelsTrace.m_sLogCtx);

        //check param and update decoder context
        _decoderContext.pParam = &_decodingParameters;
        int32_t iRet = DecoderConfigParam (_decoderContext, pParam);
        WELS_VERIFY_RETURN_IFNEQ (iRet, cmResultSuccess);

        //init decoder
        if(WelsInitDecoder (_decoderContext, &m_pWelsTrace.m_sLogCtx) != 0) {
            return cmMallocMemeError;
        }

        return cmResultSuccess;
    }

    int32_t OpenH264Decoder::ResetDecoder() {
        // TBC: need to be modified when context and trace point are null
        WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO, "ResetDecoder(), context error code is %d",
                 _decoderContext.iErrorCode);
        SDecodingParam sPrevParam;
        memcpy (&sPrevParam, _decoderContext.pParam, sizeof (SDecodingParam));

        InitDecoder(&sPrevParam);

        return ERR_INFO_UNINIT;
    }

/*
 * Set Option
 */
    long OpenH264Decoder::SetOption (DECODER_OPTION eOptID, void* pOption) {
        int iVal = 0;

        if (eOptID == DECODER_OPTION_END_OF_STREAM) { // Indicate bit-stream of the final frame to be decoded
            if (pOption == nullptr)
                return cmInitParaError;

            iVal = * ((int*)pOption); // boolean value for whether enabled End Of Stream flag

            _decoderContext.bEndOfStreamFlag = iVal ? true : false;

            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_ERROR_CON_IDC) { // Indicate error concealment status
            if (pOption == nullptr)
                return cmInitParaError;

            iVal = * ((int*)pOption); // int value for error concealment idc
            iVal = WELS_CLIP3 (iVal, (int32_t) ERROR_CON_DISABLE, (int32_t) ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE);
            if ((_decoderContext.pParam->bParseOnly) && (iVal != (int32_t) ERROR_CON_DISABLE)) {
                WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO,
                         "OpenH264Decoder::SetOption for ERROR_CON_IDC = %d not allowd for parse only!.", iVal);
                return cmInitParaError;
            }

            _decoderContext.pParam->eEcActiveIdc = (ERROR_CON_IDC) iVal;
            InitErrorCon (_decoderContext);
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO,
                     "OpenH264Decoder::SetOption for ERROR_CON_IDC = %d.", iVal);

            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_TRACE_LEVEL) {
            uint32_t level = * ((uint32_t*)pOption);
            m_pWelsTrace.SetTraceLevel (level);
            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_GET_STATISTICS) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_WARNING,
                     "OpenH264Decoder::SetOption():DECODER_OPTION_GET_STATISTICS: this option is get-only!");
            return cmInitParaError;
        } else if (eOptID == DECODER_OPTION_STATISTICS_LOG_INTERVAL) {
            if (pOption) {
                _decoderContext.sDecoderStatistics.iStatisticsLogInterval = (* ((unsigned int*)pOption));
                return cmResultSuccess;
            }
        } else if (eOptID == DECODER_OPTION_GET_SAR_INFO) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_WARNING,
                     "OpenH264Decoder::SetOption():DECODER_OPTION_GET_SAR_INFO: this option is get-only!");
            return cmInitParaError;
        }
        return cmInitParaError;
    }

/*
 *  Get Option
 */
    long OpenH264Decoder::GetOption (DECODER_OPTION eOptID, void* pOption) {
        int iVal = 0;

        if (pOption == nullptr)
            return cmInitParaError;

        if (DECODER_OPTION_END_OF_STREAM == eOptID) {
            iVal = _decoderContext.bEndOfStreamFlag;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        }
#ifdef LONG_TERM_REF
        else if (DECODER_OPTION_IDR_PIC_ID == eOptID) {
            iVal = _decoderContext.uiCurIdrPicId;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_FRAME_NUM == eOptID) {
            iVal = _decoderContext.iFrameNum;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_LTR_MARKING_FLAG == eOptID) {
            iVal = _decoderContext.bCurAuContainLtrMarkSeFlag;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_LTR_MARKED_FRAME_NUM == eOptID) {
            iVal = _decoderContext.iFrameNumOfAuMarkedLtr;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        }
#endif
        else if (DECODER_OPTION_VCL_NAL == eOptID) { //feedback whether or not have VCL NAL in current AU
            iVal = _decoderContext.iFeedbackVclNalInAu;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_TEMPORAL_ID == eOptID) { //if have VCL NAL in current AU, then feedback the temporal ID
            iVal = _decoderContext.iFeedbackTidInAu;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_IS_REF_PIC == eOptID) {
            iVal = _decoderContext.iFeedbackNalRefIdc;
            if (iVal > 0)
                iVal = 1;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_ERROR_CON_IDC == eOptID) {
            iVal = (int) _decoderContext.pParam->eEcActiveIdc;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_GET_STATISTICS == eOptID) { // get decoder statistics info for real time debugging
            SDecoderStatistics* pDecoderStatistics = (static_cast<SDecoderStatistics*> (pOption));

            memcpy (pDecoderStatistics, &_decoderContext.sDecoderStatistics, sizeof (SDecoderStatistics));

            if (_decoderContext.sDecoderStatistics.uiDecodedFrameCount != 0) { //not original status
                pDecoderStatistics->fAverageFrameSpeedInMs = (float) (_decoderContext.dDecTime) /
                                                             (_decoderContext.sDecoderStatistics.uiDecodedFrameCount);
                pDecoderStatistics->fActualAverageFrameSpeedInMs = (float) (_decoderContext.dDecTime) /
                                                                   (_decoderContext.sDecoderStatistics.uiDecodedFrameCount + _decoderContext.sDecoderStatistics.uiFreezingIDRNum +
                                                                    _decoderContext.sDecoderStatistics.uiFreezingNonIDRNum);
            }
            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_STATISTICS_LOG_INTERVAL) {
            iVal = _decoderContext.sDecoderStatistics.iStatisticsLogInterval;
            * ((unsigned int*)pOption) = static_cast<unsigned int>(iVal);
            return cmResultSuccess;
        } else if (DECODER_OPTION_GET_SAR_INFO == eOptID) { //get decoder SAR info in VUI
            PVuiSarInfo pVuiSarInfo = (static_cast<PVuiSarInfo> (pOption));
            memset (pVuiSarInfo, 0, sizeof (SVuiSarInfo));
            if (!_decoderContext.pSps) {
                return cmInitExpected;
            } else {
                pVuiSarInfo->uiSarWidth = _decoderContext.pSps->sVui.uiSarWidth;
                pVuiSarInfo->uiSarHeight = _decoderContext.pSps->sVui.uiSarHeight;
                pVuiSarInfo->bOverscanAppropriateFlag = _decoderContext.pSps->sVui.bOverscanAppropriateFlag;
                return cmResultSuccess;
            }
        } else if (DECODER_OPTION_PROFILE == eOptID) {
            if (!_decoderContext.pSps) {
                return cmInitExpected;
            }
            iVal = (int) _decoderContext.pSps->uiProfileIdc;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_LEVEL == eOptID) {
            if (!_decoderContext.pSps) {
                return cmInitExpected;
            }
            iVal = (int) _decoderContext.pSps->uiLevelIdc;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        }

        return cmInitParaError;
    }

    DECODING_STATE OpenH264Decoder::DecodeFrameNoDelay (const unsigned char* kpSrc,
                                                     const int kiSrcLen,
                                                     unsigned char** ppDst,
                                                     SBufferInfo* pDstInfo) {
        int iRet;
        iRet = DecodeFrame2 (kpSrc, kiSrcLen, ppDst, pDstInfo);
        iRet |= DecodeFrame2 (nullptr, 0, ppDst, pDstInfo);
        return (DECODING_STATE) iRet;
    }

    DECODING_STATE OpenH264Decoder::DecodeFrame2 (const unsigned char* kpSrc,
                                               const int kiSrcLen,
                                               unsigned char** ppDst,
                                               SBufferInfo* pDstInfo) {
        if (_decoderContext.pParam == nullptr) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_ERROR, "Call DecodeFrame2 without Initialize.\n");
            return dsInitialOptExpected;
        }

        if (_decoderContext.pParam->bParseOnly) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_ERROR, "bParseOnly should be false for this API calling! \n");
            _decoderContext.iErrorCode |= dsInvalidArgument;
            return dsInvalidArgument;
        }
        if (CheckBsBuffer (_decoderContext, kiSrcLen)) {
            if (ResetDecoder())
                return dsOutOfMemory;

            return dsErrorFree;
        }
        if (kiSrcLen > 0 && kpSrc != nullptr) {
            _decoderContext.bEndOfStreamFlag = false;
        } else {
            //For application MODE, the error detection should be added for safe.
            //But for CONSOLE MODE, when decoding LAST AU, kiSrcLen==0 && kpSrc==nullptr.
            _decoderContext.bEndOfStreamFlag = true;
            _decoderContext.bInstantDecFlag = true;
        }

        int64_t iStart, iEnd;
        iStart = WelsTime();
        ppDst[0] = ppDst[1] = ppDst[2] = nullptr;
        _decoderContext.iErrorCode             = dsErrorFree; //initialize at the starting of AU decoding.
        _decoderContext.iFeedbackVclNalInAu = FEEDBACK_UNKNOWN_NAL; //initialize
        unsigned long long uiInBsTimeStamp = pDstInfo->uiInBsTimeStamp;
        *pDstInfo = {};
        pDstInfo->uiInBsTimeStamp = uiInBsTimeStamp;
#ifdef LONG_TERM_REF
        _decoderContext.bReferenceLostAtT0Flag       = false; //initialize for LTR
        _decoderContext.bCurAuContainLtrMarkSeFlag = false;
        _decoderContext.iFrameNumOfAuMarkedLtr      = 0;
        _decoderContext.iFrameNum                       = -1; //initialize
#endif

        _decoderContext.iFeedbackTidInAu             = -1; //initialize
        _decoderContext.iFeedbackNalRefIdc           = -1; //initialize
        pDstInfo->uiOutYuvTimeStamp = 0;
        _decoderContext.uiTimeStamp = pDstInfo->uiInBsTimeStamp;
        WelsDecodeBs (_decoderContext, kpSrc, kiSrcLen, ppDst, pDstInfo, nullptr); //iErrorCode has been modified in this function
        _decoderContext.bInstantDecFlag = false; //reset no-delay flag

        //Decode frame failed?
        if (_decoderContext.iErrorCode) {
            //for NBR, IDR frames are expected to decode as followed if error decoding an IDR currently
            EWelsNalUnitType eNalType = _decoderContext.sCurNalHead.eNalUnitType;

            if (_decoderContext.iErrorCode & dsOutOfMemory) {
                if (ResetDecoder())
                    return dsOutOfMemory;

                return dsErrorFree;
            }
            //for AVC bitstream (excluding AVC with temporal scalability, including TP), as long as error occur, SHOULD notify upper layer key frame loss.
            if (_decoderContext.pParam->eEcActiveIdc == ERROR_CON_DISABLE) {
                if ((IS_PARAM_SETS_NALS (eNalType) or NAL_UNIT_CODED_SLICE_IDR == eNalType) or (VIDEO_BITSTREAM_AVC == _decoderContext.eVideoType)) {
#ifdef LONG_TERM_REF
                    _decoderContext.bParamSetsLostFlag = true;
#else
                    m_pDecContext.bReferenceLostAtT0Flag = true;
#endif
                }
            }

            if (_decoderContext.bPrintFrameErrorTraceFlag) {
                WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO, "decode failed, failure type:%ld \n", _decoderContext.iErrorCode);
                _decoderContext.bPrintFrameErrorTraceFlag = false;
            } else {
                _decoderContext.iIgnoredErrorInfoPacketCount ++;
                if (_decoderContext.iIgnoredErrorInfoPacketCount == std::numeric_limits<int32_t>::max()) {
                    WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_WARNING, "continuous error reached INT_MAX! Restart as 0.");
                    _decoderContext.iIgnoredErrorInfoPacketCount = 0;
                }
            }

            if ((_decoderContext.pParam->eEcActiveIdc != ERROR_CON_DISABLE) && (pDstInfo->iBufferStatus == 1)) {
                //TODO after dec status updated
                _decoderContext.iErrorCode |= dsDataErrorConcealed;

                _decoderContext.sDecoderStatistics.uiDecodedFrameCount++;
                if (_decoderContext.sDecoderStatistics.uiDecodedFrameCount == 0) { //exceed max value of uint32_t
                    ResetDecStatNums (&_decoderContext.sDecoderStatistics);
                    _decoderContext.sDecoderStatistics.uiDecodedFrameCount++;
                }
                int32_t iMbConcealedNum = _decoderContext.iMbEcedNum + _decoderContext.iMbEcedPropNum;
                _decoderContext.sDecoderStatistics.uiAvgEcRatio = _decoderContext.iMbNum == 0 ?
                                                                 (_decoderContext.sDecoderStatistics.uiAvgEcRatio * _decoderContext.sDecoderStatistics.uiEcFrameNum) : ((
                                                                                                                                                                              _decoderContext.sDecoderStatistics.uiAvgEcRatio * _decoderContext.sDecoderStatistics.uiEcFrameNum) + ((
                                                                                                                                                                                                                                                                                          iMbConcealedNum * 100) / _decoderContext.iMbNum));
                _decoderContext.sDecoderStatistics.uiAvgEcPropRatio = _decoderContext.iMbNum == 0 ?
                                                                     (_decoderContext.sDecoderStatistics.uiAvgEcPropRatio * _decoderContext.sDecoderStatistics.uiEcFrameNum) : ((
                                                                                                                                                                                      _decoderContext.sDecoderStatistics.uiAvgEcPropRatio * _decoderContext.sDecoderStatistics.uiEcFrameNum) + ((
                                                                                                                                                                                                                                                                                                      _decoderContext.iMbEcedPropNum * 100) / _decoderContext.iMbNum));
                _decoderContext.sDecoderStatistics.uiEcFrameNum += (iMbConcealedNum == 0 ? 0 : 1);
                _decoderContext.sDecoderStatistics.uiAvgEcRatio = _decoderContext.sDecoderStatistics.uiEcFrameNum == 0 ? 0 :
                                                                 _decoderContext.sDecoderStatistics.uiAvgEcRatio / _decoderContext.sDecoderStatistics.uiEcFrameNum;
                _decoderContext.sDecoderStatistics.uiAvgEcPropRatio = _decoderContext.sDecoderStatistics.uiEcFrameNum == 0 ? 0 :
                                                                     _decoderContext.sDecoderStatistics.uiAvgEcPropRatio / _decoderContext.sDecoderStatistics.uiEcFrameNum;
            }
            iEnd = WelsTime();
            _decoderContext.dDecTime += (iEnd - iStart) / 1e3;

            OutputStatisticsLog (_decoderContext.sDecoderStatistics);

            return (DECODING_STATE) _decoderContext.iErrorCode;
        }
        // else Error free, the current codec works well

        if (pDstInfo->iBufferStatus == 1) {

            _decoderContext.sDecoderStatistics.uiDecodedFrameCount++;
            if (_decoderContext.sDecoderStatistics.uiDecodedFrameCount == 0) { //exceed max value of uint32_t
                ResetDecStatNums (&_decoderContext.sDecoderStatistics);
                _decoderContext.sDecoderStatistics.uiDecodedFrameCount++;
            }

            OutputStatisticsLog (_decoderContext.sDecoderStatistics);
        }
        iEnd = WelsTime();
        _decoderContext.dDecTime += (iEnd - iStart) / 1e3;



        return dsErrorFree;
    }

    void OpenH264Decoder::OutputStatisticsLog (SDecoderStatistics& sDecoderStatistics) {
        if ((sDecoderStatistics.uiDecodedFrameCount > 0) && (sDecoderStatistics.iStatisticsLogInterval > 0)
            && ((sDecoderStatistics.uiDecodedFrameCount % sDecoderStatistics.iStatisticsLogInterval) == 0)) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO,
                     "DecoderStatistics: uiWidth=%d, uiHeight=%d, fAverageFrameSpeedInMs=%.1f, fActualAverageFrameSpeedInMs=%.1f, \
              uiDecodedFrameCount=%d, uiResolutionChangeTimes=%d, uiIDRCorrectNum=%d, \
              uiAvgEcRatio=%d, uiAvgEcPropRatio=%d, uiEcIDRNum=%d, uiEcFrameNum=%d, \
              uiIDRLostNum=%d, uiFreezingIDRNum=%d, uiFreezingNonIDRNum=%d, iAvgLumaQp=%d, \
              iSpsReportErrorNum=%d, iSubSpsReportErrorNum=%d, iPpsReportErrorNum=%d, iSpsNoExistNalNum=%d, iSubSpsNoExistNalNum=%d, iPpsNoExistNalNum=%d, \
              uiProfile=%d, uiLevel=%d, \
              iCurrentActiveSpsId=%d, iCurrentActivePpsId=%d,",
                     sDecoderStatistics.uiWidth,
                     sDecoderStatistics.uiHeight,
                     sDecoderStatistics.fAverageFrameSpeedInMs,
                     sDecoderStatistics.fActualAverageFrameSpeedInMs,

                     sDecoderStatistics.uiDecodedFrameCount,
                     sDecoderStatistics.uiResolutionChangeTimes,
                     sDecoderStatistics.uiIDRCorrectNum,

                     sDecoderStatistics.uiAvgEcRatio,
                     sDecoderStatistics.uiAvgEcPropRatio,
                     sDecoderStatistics.uiEcIDRNum,
                     sDecoderStatistics.uiEcFrameNum,

                     sDecoderStatistics.uiIDRLostNum,
                     sDecoderStatistics.uiFreezingIDRNum,
                     sDecoderStatistics.uiFreezingNonIDRNum,
                     sDecoderStatistics.iAvgLumaQp,

                     sDecoderStatistics.iSpsReportErrorNum,
                     sDecoderStatistics.iSubSpsReportErrorNum,
                     sDecoderStatistics.iPpsReportErrorNum,
                     sDecoderStatistics.iSpsNoExistNalNum,
                     sDecoderStatistics.iSubSpsNoExistNalNum,
                     sDecoderStatistics.iPpsNoExistNalNum,

                     sDecoderStatistics.uiProfile,
                     sDecoderStatistics.uiLevel,

                     sDecoderStatistics.iCurrentActiveSpsId,
                     sDecoderStatistics.iCurrentActivePpsId);
        }
    }

    DECODING_STATE OpenH264Decoder::DecodeParser (const unsigned char* kpSrc,
                                               const int kiSrcLen,
                                               SParserBsInfo* pDstInfo) {
        if (_decoderContext.pParam == nullptr) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_ERROR, "Call DecodeParser without Initialize.\n");
            return dsInitialOptExpected;
        }

        if (!_decoderContext.pParam->bParseOnly) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_ERROR, "bParseOnly should be true for this API calling! \n");
            _decoderContext.iErrorCode |= dsInvalidArgument;
            return dsInvalidArgument;
        }
        int64_t iEnd, iStart = WelsTime();
        if (CheckBsBuffer (_decoderContext, kiSrcLen)) {
            if (ResetDecoder())
                return dsOutOfMemory;

            return dsErrorFree;
        }
        if (kiSrcLen > 0 && kpSrc != nullptr) {
            _decoderContext.bEndOfStreamFlag = false;
        } else {
            //For application MODE, the error detection should be added for safe.
            //But for CONSOLE MODE, when decoding LAST AU, kiSrcLen==0 && kpSrc==nullptr.
            _decoderContext.bEndOfStreamFlag = true;
            _decoderContext.bInstantDecFlag = true;
        }

        _decoderContext.iErrorCode = dsErrorFree; //initialize at the starting of AU decoding.
        _decoderContext.pParam->eEcActiveIdc = ERROR_CON_DISABLE; //add protection to disable EC here.
        _decoderContext.iFeedbackNalRefIdc = -1; //initialize
        if (!_decoderContext.bFramePending) { //frame complete
            _decoderContext.pParserBsInfo.iNalNum = 0;
            memset (_decoderContext.pParserBsInfo.pNalLenInByte, 0, MAX_NAL_UNITS_IN_LAYER);
        }
        pDstInfo->iNalNum = 0;
        pDstInfo->iSpsWidthInPixel = pDstInfo->iSpsHeightInPixel = 0;
        _decoderContext.uiTimeStamp = pDstInfo->uiInBsTimeStamp;
        pDstInfo->uiOutBsTimeStamp = 0;
        WelsDecodeBs (_decoderContext, kpSrc, kiSrcLen, nullptr, nullptr, pDstInfo);
        if (_decoderContext.iErrorCode & dsOutOfMemory) {
            if (ResetDecoder())
                return dsOutOfMemory;
            return dsErrorFree;
        }

        if (!_decoderContext.bFramePending && _decoderContext.pParserBsInfo.iNalNum) {
            memcpy (pDstInfo, &_decoderContext.pParserBsInfo, sizeof (SParserBsInfo));

            if (_decoderContext.iErrorCode == ERR_NONE) { //update statistics: decoding frame count
                _decoderContext.sDecoderStatistics.uiDecodedFrameCount++;
                if (_decoderContext.sDecoderStatistics.uiDecodedFrameCount == 0) { //exceed max value of uint32_t
                    ResetDecStatNums (&_decoderContext.sDecoderStatistics);
                    _decoderContext.sDecoderStatistics.uiDecodedFrameCount++;
                }
            }
        }

        _decoderContext.bInstantDecFlag = false; //reset no-delay flag

        if (_decoderContext.iErrorCode && _decoderContext.bPrintFrameErrorTraceFlag) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO, "decode failed, failure type:%d \n", _decoderContext.iErrorCode);
            _decoderContext.bPrintFrameErrorTraceFlag = false;
        }
        iEnd = WelsTime();
        _decoderContext.dDecTime += (iEnd - iStart) / 1e3;

        return (DECODING_STATE) _decoderContext.iErrorCode;
    }

    DECODING_STATE OpenH264Decoder::DecodeFrame (const unsigned char* kpSrc,
                                              const int kiSrcLen,
                                              unsigned char** ppDst,
                                              int* pStride,
                                              int& iWidth,
                                              int& iHeight) {
        SBufferInfo    DstInfo;

        memset (&DstInfo, 0, sizeof (SBufferInfo));
        DstInfo.UsrData.sSystemBuffer.iStride[0] = pStride[0];
        DstInfo.UsrData.sSystemBuffer.iStride[1] = pStride[1];
        DstInfo.UsrData.sSystemBuffer.iWidth = iWidth;
        DstInfo.UsrData.sSystemBuffer.iHeight = iHeight;

        DECODING_STATE eDecState = DecodeFrame2 (kpSrc, kiSrcLen, ppDst, &DstInfo);
        if (eDecState == dsErrorFree) {
            pStride[0] = DstInfo.UsrData.sSystemBuffer.iStride[0];
            pStride[1] = DstInfo.UsrData.sSystemBuffer.iStride[1];
            iWidth     = DstInfo.UsrData.sSystemBuffer.iWidth;
            iHeight    = DstInfo.UsrData.sSystemBuffer.iHeight;
        }

        return eDecState;
    }

DECODING_STATE OpenH264Decoder::DecodeFrameEx (const unsigned char* kpSrc,
                                            const int kiSrcLen,
                                            unsigned char* pDst,
                                            int iDstStride,
                                            int& iDstLen,
                                            int& iWidth,
                                            int& iHeight,
                                            int& iColorFormat) {
    return dsErrorFree;
}

} //namespace end
