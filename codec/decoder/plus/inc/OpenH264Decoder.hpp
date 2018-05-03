#pragma once

#include "codec_api.h"
#include "codec_app_def.h"
#include "decoder_context.h"
#include "welsCodecTrace.h"
#include "cpu.h"

class ISVCDecoder;
namespace staaker
{
    using namespace WelsDec;

class OpenH264Decoder : public ISVCDecoder {
public:
    OpenH264Decoder();
    ~OpenH264Decoder() override;

    long EXTAPI Initialize (const SDecodingParam* pParam) override;
    long EXTAPI Uninitialize() override;

    /***************************************************************************
    *   Description:
    *       Decompress one frame, and output I420 or RGB24(in the future) decoded stream and its length.
    *   Input parameters:
    *       Parameter       TYPE                   Description
    *       pSrc            unsigned char*         the h264 stream to decode
    *       srcLength       int                    the length of h264 steam
    *       pDst            unsigned char*         buffer pointer of decoded data
    *       pDstInfo        SBufferInfo&           information provided to API including width, height, SW/HW option, etc
    *
    *   return: if decode frame success return 0, otherwise corresponding error returned.
    ***************************************************************************/
    virtual DECODING_STATE EXTAPI DecodeFrame (const unsigned char* kpSrc,
                                               const int kiSrcLen,
                                               unsigned char** ppDst,
                                               int* pStride,
                                               int& iWidth,
                                               int& iHeight);

    virtual DECODING_STATE EXTAPI DecodeFrameNoDelay (const unsigned char* kpSrc,
                                                      const int kiSrcLen,
                                                      unsigned char** ppDst,
                                                      SBufferInfo* pDstInfo);

    virtual DECODING_STATE EXTAPI DecodeFrame2 (const unsigned char* kpSrc,
                                                const int kiSrcLen,
                                                unsigned char** ppDst,
                                                SBufferInfo* pDstInfo);
    virtual DECODING_STATE EXTAPI DecodeParser (const unsigned char* kpSrc,
                                                const int kiSrcLen,
                                                SParserBsInfo* pDstInfo);
    virtual DECODING_STATE EXTAPI DecodeFrameEx (const unsigned char* kpSrc,
                                                 const int kiSrcLen,
                                                 unsigned char* pDst,
                                                 int iDstStride,
                                                 int& iDstLen,
                                                 int& iWidth,
                                                 int& iHeight,
                                                 int& color_format);

    virtual long EXTAPI SetOption (DECODER_OPTION eOptID, void* pOption);
    virtual long EXTAPI GetOption (DECODER_OPTION eOptID, void* pOption);

private:
    SWelsDecoderContext     m_pDecContext;
    welsCodecTrace          m_pWelsTrace;
    SDecodingParam          _decodingParameters;

    int32_t InitDecoder (const SDecodingParam* pParam);
    int32_t ResetDecoder();

    void OutputStatisticsLog (SDecoderStatistics& sDecoderStatistics);
};

} //namespace end
