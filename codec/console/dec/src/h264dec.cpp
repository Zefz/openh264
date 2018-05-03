/*!
 * \copy
 *     Copyright (c)  2004-2013, Cisco Systems
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
 * h264dec.cpp:         Wels Decoder Console Implementation file
 */

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <memory>
#include <random>
#include <iostream>
#include <chrono>
#include <welsDecoderExt.h>
#include <OpenH264Decoder.hpp>
#include <codec_app_def.h>

#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"

using namespace std;

static int streamToFile(uint8_t* pDst[3], SBufferInfo* pInfo, FILE* pFp) {

    int iRet = 0;

    if (pFp && pDst[0] && pDst[1] && pDst[2] && pInfo) {
        int iStride[2];
        int iWidth = pInfo->UsrData.sSystemBuffer.iWidth;
        int iHeight = pInfo->UsrData.sSystemBuffer.iHeight;
        iStride[0] = pInfo->UsrData.sSystemBuffer.iStride[0];
        iStride[1] = pInfo->UsrData.sSystemBuffer.iStride[1];
        //std::cout << "size = " << iWidth << "x" << iHeight << "\n";
        //std::cout << "Stride = " << iStride[0] << ", " << iStride[1] << "\n";

        unsigned char*  pPtr = NULL;
        size_t frameSize = 0;

        pPtr = pDst[0];
        for (int i = 0; i < iHeight; i++) {
            fwrite (pPtr, 1, iWidth, pFp);
            pPtr += iStride[0];
            frameSize += iStride[0];
        }

        iHeight = iHeight / 2;
        iWidth = iWidth / 2;
        pPtr = pDst[1];
        for (int i = 0; i < iHeight; i++) {
            fwrite (pPtr, 1, iWidth, pFp);
            pPtr += iStride[1];
            frameSize += iStride[1];
        }

        pPtr = pDst[2];
        for (int i = 0; i < iHeight; i++) {
            fwrite (pPtr, 1, iWidth, pFp);
            pPtr += iStride[1];
            frameSize += iStride[1];
        }
        //std::cout << "Frame size = " << frameSize << " bytes\n";
    }

    return iRet;
}

void H264DecodeInstance (ISVCDecoder* pDecoder, const char* kpH264FileName, const char* kpOuputFileName,
                         int32_t& iWidth, int32_t& iHeight) {
    FILE* pH264File   = NULL;
    FILE* pYuvFile    = NULL;
// Lenght input mode support

    if (pDecoder == NULL) return;

    int64_t iTotal = 0;

    uint8_t* pDst[3] = {nullptr, nullptr, nullptr};

    int32_t iBufPos = 0;
    int32_t iFrameCount = 0;

    double dElapsed = 0;

    if (kpH264FileName) {
        pH264File = fopen (kpH264FileName, "rb");
        if (pH264File == NULL) {
            fprintf (stderr, "Can not open h264 source file, check its legal path related please..\n");
            return;
        }
        fprintf (stderr, "H264 source file name: %s..\n", kpH264FileName);
    } else {
        fprintf (stderr, "Can not find any h264 bitstream file to read..\n");
        fprintf (stderr, "----------------decoder return------------------------\n");
        return;
    }

    if (kpOuputFileName) {
        pYuvFile = fopen (kpOuputFileName, "wb");
        if (pYuvFile == NULL) {
            fprintf (stderr, "Can not open yuv file to output result of decoding..\n");
            // any options
            //return; // can let decoder work in quiet mode, no writing any output
        } else
            fprintf (stderr, "Sequence output file name: %s..\n", kpOuputFileName);
    } else {
        fprintf (stderr, "Can not find any output file to write..\n");
        // any options
    }

    printf ("------------------------------------------------------\n");

    fseek (pH264File, 0L, SEEK_END);
    int32_t iFileSize = (int32_t) ftell (pH264File);
    if (iFileSize <= 0) {
        fprintf (stderr, "Current Bit Stream File is too small, read error!!!!\n");
        fclose (pH264File);
        fclose(pYuvFile);
        return;
    }
    fseek (pH264File, 0L, SEEK_SET);

    std::unique_ptr<uint8_t[]> pBuf = std::make_unique<uint8_t[]>(iFileSize + 4);

    if (fread (pBuf.get(), 1, iFileSize, pH264File) != (uint32_t)iFileSize) {
        fprintf (stderr, "Unable to read whole file\n");
        fclose (pH264File);
        fclose(pYuvFile);
        return;
    }

    static constexpr uint8_t UI_START_CODE[4] = {0, 0, 0, 1};
    memcpy(pBuf.get() + iFileSize, &UI_START_CODE[0], 4); //confirmed_safe_unsafe_usage

    std::mt19937 rnd(std::random_device{}());
    std::uniform_int_distribution<int> uniform_dist(1, 100);

    while (iBufPos < iFileSize) {

        //Simulate packet loss
        //if(uniform_dist(rnd) <= 5) {
        //    iBufPos += 256;
        //}


        //Find next slice
        int32_t iSliceSize;
        for (iSliceSize = 4; iSliceSize < iFileSize; iSliceSize++) {
            if(*reinterpret_cast<const int32_t*>(pBuf.get() + iBufPos + iSliceSize) == 0x01000000) {
                break;
            }
/*
            const uint8_t *pos = &pBuf[iBufPos + iSliceSize];
            const int32_t val = pos[0]<<16 | pos[1]<<8 | pos[2];
            if(val == 0x01) {
                break;
            }
            if(val == 0x00 and pos[3] == 0x01) {
                break;
            }
*/
        }

        const auto iStart = std::chrono::steady_clock::now();

        SBufferInfo sDstBufInfo{};
        pDecoder->DecodeFrameNoDelay(&pBuf[iBufPos], iSliceSize, pDst, &sDstBufInfo);

        iTotal += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - iStart).count();

        if (sDstBufInfo.iBufferStatus == 1) {
            streamToFile(pDst, &sDstBufInfo, pYuvFile);
            iWidth  = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
            iHeight = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
            ++ iFrameCount;
        }

        iBufPos += iSliceSize;
        //std::cout << "slice size: " << iSliceSize << "\n";
    }
    static constexpr int32_t END_OF_STREAM_FLAG = 0;
    pDecoder->SetOption(DECODER_OPTION_END_OF_STREAM, (void*)&END_OF_STREAM_FLAG);

    dElapsed = iTotal / 1e6;
    fprintf (stderr, "-------------------------------------------------------\n");
    fprintf (stderr, "iWidth:\t\t%d\nheight:\t\t%d\nFrames:\t\t%d\ndecode time:\t%f sec\nFPS:\t\t%f fps\n",
             iWidth, iHeight, iFrameCount, dElapsed, (iFrameCount * 1.0) / dElapsed);
    fprintf (stderr, "-------------------------------------------------------\n");
}

int32_t main (int32_t iArgC, char* pArgV[]) {
    staaker::OpenH264Decoder decoder;

    SDecodingParam sDecParam{};
    string strInputFile, strOutputFile;

    sDecParam.sVideoProperty.size = sizeof (sDecParam.sVideoProperty);
    sDecParam.uiTargetDqLayer = std::numeric_limits<uint8_t>::max();
    sDecParam.eEcActiveIdc = ERROR_CON_SLICE_COPY_CROSS_IDR_FREEZE_RES_CHANGE;
    sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    if (iArgC < 3) {
        printf ("usage 1: h264dec.exe welsdec.cfg\n");
        printf ("usage 2: h264dec.exe welsdec.264 out.yuv\n");
        printf ("usage 3: h264dec.exe welsdec.264\n");
        strInputFile = "240p.h264";
        strOutputFile = "out.yuv";
    } else { //iArgC > 2
        strInputFile = pArgV[1];
        strOutputFile = pArgV[2];
    }

    int32_t iLevelSetting = WELS_LOG_WARNING;
    decoder.SetOption (DECODER_OPTION_TRACE_LEVEL, &iLevelSetting);

    if (decoder.Initialize(&sDecParam)) {
        printf ("Decoder initialization failed.\n");
        return 1;
    }

    int32_t iWidth = 0;
    int32_t iHeight = 0;

    H264DecodeInstance(&decoder, strInputFile.c_str(), strOutputFile.c_str(), iWidth, iHeight);

    decoder.Uninitialize();
    return 0;
}
