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
            : m_pDecContext{},
              m_pWelsTrace{},
              _memoryHandler{ICACHE_LINE_SIZE}
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

        WelsEndDecoder (&m_pDecContext);
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
                 "OpenH264Decoder::init_decoder(), openh264 codec version = %s, ParseOnly = %d",
                 VERSION_NUMBER, (int32_t)pParam->bParseOnly);

        //reset decoder context
        m_pDecContext = {};


        m_pDecContext.pMemAlign = &_memoryHandler;

        //fill in default value into context
        WelsDecoderDefaults (&m_pDecContext, &m_pWelsTrace.m_sLogCtx);

        //check param and update decoder context
        m_pDecContext.pParam = &_decodingParameters;
        int32_t iRet = DecoderConfigParam (&m_pDecContext, pParam);
        WELS_VERIFY_RETURN_IFNEQ (iRet, cmResultSuccess);

        //init decoder
        if(WelsInitDecoder (&m_pDecContext, &m_pWelsTrace.m_sLogCtx) != 0) {
            return cmMallocMemeError;
        }

        return cmResultSuccess;
    }

    int32_t OpenH264Decoder::ResetDecoder() {
        // TBC: need to be modified when context and trace point are null
        WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO, "ResetDecoder(), context error code is %d",
                 m_pDecContext.iErrorCode);
        SDecodingParam sPrevParam;
        memcpy (&sPrevParam, m_pDecContext.pParam, sizeof (SDecodingParam));

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

            m_pDecContext.bEndOfStreamFlag = iVal ? true : false;

            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_ERROR_CON_IDC) { // Indicate error concealment status
            if (pOption == nullptr)
                return cmInitParaError;

            iVal = * ((int*)pOption); // int value for error concealment idc
            iVal = WELS_CLIP3 (iVal, (int32_t) ERROR_CON_DISABLE, (int32_t) ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE);
            if ((m_pDecContext.pParam->bParseOnly) && (iVal != (int32_t) ERROR_CON_DISABLE)) {
                WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO,
                         "OpenH264Decoder::SetOption for ERROR_CON_IDC = %d not allowd for parse only!.", iVal);
                return cmInitParaError;
            }

            m_pDecContext.pParam->eEcActiveIdc = (ERROR_CON_IDC) iVal;
            InitErrorCon (&m_pDecContext);
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO,
                     "OpenH264Decoder::SetOption for ERROR_CON_IDC = %d.", iVal);

            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_TRACE_LEVEL) {
            uint32_t level = * ((uint32_t*)pOption);
            m_pWelsTrace.SetTraceLevel (level);
            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_TRACE_CALLBACK) {
            WelsTraceCallback callback = * ((WelsTraceCallback*)pOption);
            m_pWelsTrace.SetTraceCallback (callback);
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO,
                     "OpenH264Decoder::SetOption():DECODER_OPTION_TRACE_CALLBACK callback = %p.",
                     callback);
            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_TRACE_CALLBACK_CONTEXT) {
            void* ctx = * ((void**)pOption);
            m_pWelsTrace.SetTraceCallbackContext (ctx);
            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_GET_STATISTICS) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_WARNING,
                     "OpenH264Decoder::SetOption():DECODER_OPTION_GET_STATISTICS: this option is get-only!");
            return cmInitParaError;
        } else if (eOptID == DECODER_OPTION_STATISTICS_LOG_INTERVAL) {
            if (pOption) {
                m_pDecContext.sDecoderStatistics.iStatisticsLogInterval = (* ((unsigned int*)pOption));
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
            iVal = m_pDecContext.bEndOfStreamFlag;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        }
#ifdef LONG_TERM_REF
        else if (DECODER_OPTION_IDR_PIC_ID == eOptID) {
            iVal = m_pDecContext.uiCurIdrPicId;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_FRAME_NUM == eOptID) {
            iVal = m_pDecContext.iFrameNum;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_LTR_MARKING_FLAG == eOptID) {
            iVal = m_pDecContext.bCurAuContainLtrMarkSeFlag;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_LTR_MARKED_FRAME_NUM == eOptID) {
            iVal = m_pDecContext.iFrameNumOfAuMarkedLtr;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        }
#endif
        else if (DECODER_OPTION_VCL_NAL == eOptID) { //feedback whether or not have VCL NAL in current AU
            iVal = m_pDecContext.iFeedbackVclNalInAu;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_TEMPORAL_ID == eOptID) { //if have VCL NAL in current AU, then feedback the temporal ID
            iVal = m_pDecContext.iFeedbackTidInAu;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_IS_REF_PIC == eOptID) {
            iVal = m_pDecContext.iFeedbackNalRefIdc;
            if (iVal > 0)
                iVal = 1;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_ERROR_CON_IDC == eOptID) {
            iVal = (int) m_pDecContext.pParam->eEcActiveIdc;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_GET_STATISTICS == eOptID) { // get decoder statistics info for real time debugging
            SDecoderStatistics* pDecoderStatistics = (static_cast<SDecoderStatistics*> (pOption));

            memcpy (pDecoderStatistics, &m_pDecContext.sDecoderStatistics, sizeof (SDecoderStatistics));

            if (m_pDecContext.sDecoderStatistics.uiDecodedFrameCount != 0) { //not original status
                pDecoderStatistics->fAverageFrameSpeedInMs = (float) (m_pDecContext.dDecTime) /
                                                             (m_pDecContext.sDecoderStatistics.uiDecodedFrameCount);
                pDecoderStatistics->fActualAverageFrameSpeedInMs = (float) (m_pDecContext.dDecTime) /
                                                                   (m_pDecContext.sDecoderStatistics.uiDecodedFrameCount + m_pDecContext.sDecoderStatistics.uiFreezingIDRNum +
                                                                    m_pDecContext.sDecoderStatistics.uiFreezingNonIDRNum);
            }
            return cmResultSuccess;
        } else if (eOptID == DECODER_OPTION_STATISTICS_LOG_INTERVAL) {
            iVal = m_pDecContext.sDecoderStatistics.iStatisticsLogInterval;
            * ((unsigned int*)pOption) = static_cast<unsigned int>(iVal);
            return cmResultSuccess;
        } else if (DECODER_OPTION_GET_SAR_INFO == eOptID) { //get decoder SAR info in VUI
            PVuiSarInfo pVuiSarInfo = (static_cast<PVuiSarInfo> (pOption));
            memset (pVuiSarInfo, 0, sizeof (SVuiSarInfo));
            if (!m_pDecContext.pSps) {
                return cmInitExpected;
            } else {
                pVuiSarInfo->uiSarWidth = m_pDecContext.pSps->sVui.uiSarWidth;
                pVuiSarInfo->uiSarHeight = m_pDecContext.pSps->sVui.uiSarHeight;
                pVuiSarInfo->bOverscanAppropriateFlag = m_pDecContext.pSps->sVui.bOverscanAppropriateFlag;
                return cmResultSuccess;
            }
        } else if (DECODER_OPTION_PROFILE == eOptID) {
            if (!m_pDecContext.pSps) {
                return cmInitExpected;
            }
            iVal = (int) m_pDecContext.pSps->uiProfileIdc;
            * ((int*)pOption) = iVal;
            return cmResultSuccess;
        } else if (DECODER_OPTION_LEVEL == eOptID) {
            if (!m_pDecContext.pSps) {
                return cmInitExpected;
            }
            iVal = (int) m_pDecContext.pSps->uiLevelIdc;
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
        if (m_pDecContext.pParam == nullptr) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_ERROR, "Call DecodeFrame2 without Initialize.\n");
            return dsInitialOptExpected;
        }

        if (m_pDecContext.pParam->bParseOnly) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_ERROR, "bParseOnly should be false for this API calling! \n");
            m_pDecContext.iErrorCode |= dsInvalidArgument;
            return dsInvalidArgument;
        }
        if (CheckBsBuffer (&m_pDecContext, kiSrcLen)) {
            if (ResetDecoder())
                return dsOutOfMemory;

            return dsErrorFree;
        }
        if (kiSrcLen > 0 && kpSrc != nullptr) {
            m_pDecContext.bEndOfStreamFlag = false;
        } else {
            //For application MODE, the error detection should be added for safe.
            //But for CONSOLE MODE, when decoding LAST AU, kiSrcLen==0 && kpSrc==nullptr.
            m_pDecContext.bEndOfStreamFlag = true;
            m_pDecContext.bInstantDecFlag = true;
        }

        int64_t iStart, iEnd;
        iStart = WelsTime();
        ppDst[0] = ppDst[1] = ppDst[2] = nullptr;
        m_pDecContext.iErrorCode             = dsErrorFree; //initialize at the starting of AU decoding.
        m_pDecContext.iFeedbackVclNalInAu = FEEDBACK_UNKNOWN_NAL; //initialize
        unsigned long long uiInBsTimeStamp = pDstInfo->uiInBsTimeStamp;
        memset (pDstInfo, 0, sizeof (SBufferInfo));
        pDstInfo->uiInBsTimeStamp = uiInBsTimeStamp;
#ifdef LONG_TERM_REF
        m_pDecContext.bReferenceLostAtT0Flag       = false; //initialize for LTR
        m_pDecContext.bCurAuContainLtrMarkSeFlag = false;
        m_pDecContext.iFrameNumOfAuMarkedLtr      = 0;
        m_pDecContext.iFrameNum                       = -1; //initialize
#endif

        m_pDecContext.iFeedbackTidInAu             = -1; //initialize
        m_pDecContext.iFeedbackNalRefIdc           = -1; //initialize
        pDstInfo->uiOutYuvTimeStamp = 0;
        m_pDecContext.uiTimeStamp = pDstInfo->uiInBsTimeStamp;
        WelsDecodeBs (&m_pDecContext, kpSrc, kiSrcLen, ppDst, pDstInfo, nullptr); //iErrorCode has been modified in this function
        m_pDecContext.bInstantDecFlag = false; //reset no-delay flag

        //Decode frame failed?
        if (m_pDecContext.iErrorCode) {
            //for NBR, IDR frames are expected to decode as followed if error decoding an IDR currently
            EWelsNalUnitType eNalType = m_pDecContext.sCurNalHead.eNalUnitType;

            if (m_pDecContext.iErrorCode & dsOutOfMemory) {
                if (ResetDecoder())
                    return dsOutOfMemory;

                return dsErrorFree;
            }
            //for AVC bitstream (excluding AVC with temporal scalability, including TP), as long as error occur, SHOULD notify upper layer key frame loss.
            if (m_pDecContext.pParam->eEcActiveIdc == ERROR_CON_DISABLE) {
                if ((IS_PARAM_SETS_NALS (eNalType) or NAL_UNIT_CODED_SLICE_IDR == eNalType) or (VIDEO_BITSTREAM_AVC == m_pDecContext.eVideoType)) {
#ifdef LONG_TERM_REF
                    m_pDecContext.bParamSetsLostFlag = true;
#else
                    m_pDecContext.bReferenceLostAtT0Flag = true;
#endif
                }
            }

            if (m_pDecContext.bPrintFrameErrorTraceFlag) {
                WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO, "decode failed, failure type:%ld \n", m_pDecContext.iErrorCode);
                m_pDecContext.bPrintFrameErrorTraceFlag = false;
            } else {
                m_pDecContext.iIgnoredErrorInfoPacketCount ++;
                if (m_pDecContext.iIgnoredErrorInfoPacketCount == INT_MAX) {
                    WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_WARNING, "continuous error reached INT_MAX! Restart as 0.");
                    m_pDecContext.iIgnoredErrorInfoPacketCount = 0;
                }
            }

            if ((m_pDecContext.pParam->eEcActiveIdc != ERROR_CON_DISABLE) && (pDstInfo->iBufferStatus == 1)) {
                //TODO after dec status updated
                m_pDecContext.iErrorCode |= dsDataErrorConcealed;

                m_pDecContext.sDecoderStatistics.uiDecodedFrameCount++;
                if (m_pDecContext.sDecoderStatistics.uiDecodedFrameCount == 0) { //exceed max value of uint32_t
                    ResetDecStatNums (&m_pDecContext.sDecoderStatistics);
                    m_pDecContext.sDecoderStatistics.uiDecodedFrameCount++;
                }
                int32_t iMbConcealedNum = m_pDecContext.iMbEcedNum + m_pDecContext.iMbEcedPropNum;
                m_pDecContext.sDecoderStatistics.uiAvgEcRatio = m_pDecContext.iMbNum == 0 ?
                                                                 (m_pDecContext.sDecoderStatistics.uiAvgEcRatio * m_pDecContext.sDecoderStatistics.uiEcFrameNum) : ((
                                                                                                                                                                              m_pDecContext.sDecoderStatistics.uiAvgEcRatio * m_pDecContext.sDecoderStatistics.uiEcFrameNum) + ((
                                                                                                                                                                                                                                                                                          iMbConcealedNum * 100) / m_pDecContext.iMbNum));
                m_pDecContext.sDecoderStatistics.uiAvgEcPropRatio = m_pDecContext.iMbNum == 0 ?
                                                                     (m_pDecContext.sDecoderStatistics.uiAvgEcPropRatio * m_pDecContext.sDecoderStatistics.uiEcFrameNum) : ((
                                                                                                                                                                                      m_pDecContext.sDecoderStatistics.uiAvgEcPropRatio * m_pDecContext.sDecoderStatistics.uiEcFrameNum) + ((
                                                                                                                                                                                                                                                                                                      m_pDecContext.iMbEcedPropNum * 100) / m_pDecContext.iMbNum));
                m_pDecContext.sDecoderStatistics.uiEcFrameNum += (iMbConcealedNum == 0 ? 0 : 1);
                m_pDecContext.sDecoderStatistics.uiAvgEcRatio = m_pDecContext.sDecoderStatistics.uiEcFrameNum == 0 ? 0 :
                                                                 m_pDecContext.sDecoderStatistics.uiAvgEcRatio / m_pDecContext.sDecoderStatistics.uiEcFrameNum;
                m_pDecContext.sDecoderStatistics.uiAvgEcPropRatio = m_pDecContext.sDecoderStatistics.uiEcFrameNum == 0 ? 0 :
                                                                     m_pDecContext.sDecoderStatistics.uiAvgEcPropRatio / m_pDecContext.sDecoderStatistics.uiEcFrameNum;
            }
            iEnd = WelsTime();
            m_pDecContext.dDecTime += (iEnd - iStart) / 1e3;

            OutputStatisticsLog (m_pDecContext.sDecoderStatistics);

            return (DECODING_STATE) m_pDecContext.iErrorCode;
        }
        // else Error free, the current codec works well

        if (pDstInfo->iBufferStatus == 1) {

            m_pDecContext.sDecoderStatistics.uiDecodedFrameCount++;
            if (m_pDecContext.sDecoderStatistics.uiDecodedFrameCount == 0) { //exceed max value of uint32_t
                ResetDecStatNums (&m_pDecContext.sDecoderStatistics);
                m_pDecContext.sDecoderStatistics.uiDecodedFrameCount++;
            }

            OutputStatisticsLog (m_pDecContext.sDecoderStatistics);
        }
        iEnd = WelsTime();
        m_pDecContext.dDecTime += (iEnd - iStart) / 1e3;



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
        if (m_pDecContext.pParam == nullptr) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_ERROR, "Call DecodeParser without Initialize.\n");
            return dsInitialOptExpected;
        }

        if (!m_pDecContext.pParam->bParseOnly) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_ERROR, "bParseOnly should be true for this API calling! \n");
            m_pDecContext.iErrorCode |= dsInvalidArgument;
            return dsInvalidArgument;
        }
        int64_t iEnd, iStart = WelsTime();
        if (CheckBsBuffer (&m_pDecContext, kiSrcLen)) {
            if (ResetDecoder())
                return dsOutOfMemory;

            return dsErrorFree;
        }
        if (kiSrcLen > 0 && kpSrc != nullptr) {
            m_pDecContext.bEndOfStreamFlag = false;
        } else {
            //For application MODE, the error detection should be added for safe.
            //But for CONSOLE MODE, when decoding LAST AU, kiSrcLen==0 && kpSrc==nullptr.
            m_pDecContext.bEndOfStreamFlag = true;
            m_pDecContext.bInstantDecFlag = true;
        }

        m_pDecContext.iErrorCode = dsErrorFree; //initialize at the starting of AU decoding.
        m_pDecContext.pParam->eEcActiveIdc = ERROR_CON_DISABLE; //add protection to disable EC here.
        m_pDecContext.iFeedbackNalRefIdc = -1; //initialize
        if (!m_pDecContext.bFramePending) { //frame complete
            m_pDecContext.pParserBsInfo.iNalNum = 0;
            memset (m_pDecContext.pParserBsInfo.pNalLenInByte, 0, MAX_NAL_UNITS_IN_LAYER);
        }
        pDstInfo->iNalNum = 0;
        pDstInfo->iSpsWidthInPixel = pDstInfo->iSpsHeightInPixel = 0;
        m_pDecContext.uiTimeStamp = pDstInfo->uiInBsTimeStamp;
        pDstInfo->uiOutBsTimeStamp = 0;
        WelsDecodeBs (&m_pDecContext, kpSrc, kiSrcLen, nullptr, nullptr, pDstInfo);
        if (m_pDecContext.iErrorCode & dsOutOfMemory) {
            if (ResetDecoder())
                return dsOutOfMemory;
            return dsErrorFree;
        }

        if (!m_pDecContext.bFramePending && m_pDecContext.pParserBsInfo.iNalNum) {
            memcpy (pDstInfo, &m_pDecContext.pParserBsInfo, sizeof (SParserBsInfo));

            if (m_pDecContext.iErrorCode == ERR_NONE) { //update statistics: decoding frame count
                m_pDecContext.sDecoderStatistics.uiDecodedFrameCount++;
                if (m_pDecContext.sDecoderStatistics.uiDecodedFrameCount == 0) { //exceed max value of uint32_t
                    ResetDecStatNums (&m_pDecContext.sDecoderStatistics);
                    m_pDecContext.sDecoderStatistics.uiDecodedFrameCount++;
                }
            }
        }

        m_pDecContext.bInstantDecFlag = false; //reset no-delay flag

        if (m_pDecContext.iErrorCode && m_pDecContext.bPrintFrameErrorTraceFlag) {
            WelsLog (&m_pWelsTrace.m_sLogCtx, WELS_LOG_INFO, "decode failed, failure type:%d \n", m_pDecContext.iErrorCode);
            m_pDecContext.bPrintFrameErrorTraceFlag = false;
        }
        iEnd = WelsTime();
        m_pDecContext.dDecTime += (iEnd - iStart) / 1e3;

        return (DECODING_STATE) m_pDecContext.iErrorCode;
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
