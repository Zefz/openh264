cmake_minimum_required(VERSION 3.5)
project(openh264 CXX)

file(GLOB_RECURSE DECODER_SOURCES
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/au_parser.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/bit_stream.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/cabac_decoder.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/deblocking.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/decode_mb_aux.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/decode_slice.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/decoder.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/decoder_core.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/decoder_data_tables.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/error_concealment.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/fmo.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/get_intra_predictor.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/manage_dec_ref.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/memmgr_nal_unit.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/mv_pred.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/parse_mb_syn_cabac.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/parse_mb_syn_cavlc.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/pic_queue.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/src/rec_mb.cpp

        ${PROJECT_SOURCE_DIR}/codec/common/src/utils.cpp
 #       ${PROJECT_SOURCE_DIR}/codec/common/src/sad_common.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/memory_align.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/mc.cpp
 #       ${PROJECT_SOURCE_DIR}/codec/common/src/intra_pred_common.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/expand_pic.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/deblocking_common.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/crt_util_safe_x.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/cpu.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/copy_mb.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/common_tables.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/welsCodecTrace.cpp
        ${PROJECT_SOURCE_DIR}/codec/decoder/plus/src/OpenH264Decoder.cpp
        )

add_library(decoder STATIC ${DECODER_SOURCES})
target_compile_options(decoder PRIVATE -O3 -Wno-format -fno-strict-aliasing -fPIC)

target_include_directories(decoder PRIVATE
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/inc
        ${PROJECT_SOURCE_DIR}/codec/decoder/plus/inc
        ${PROJECT_SOURCE_DIR}/codec/common/inc
        ${PROJECT_SOURCE_DIR}/codec/api/svc)

add_executable(h264dec ${PROJECT_SOURCE_DIR}/codec/console/dec/src/h264dec.cpp)
link_directories(${PROJECT_SOURCE_DIR})
target_link_libraries(h264dec decoder)
add_dependencies(h264dec decoder)

target_include_directories(h264dec PRIVATE
        ${PROJECT_SOURCE_DIR}/codec/decoder/core/inc
        ${PROJECT_SOURCE_DIR}/codec/decoder/plus/inc
        ${PROJECT_SOURCE_DIR}/codec/common/inc
        ${PROJECT_SOURCE_DIR}/codec/api/svc)


add_executable(h264enc
        ${PROJECT_SOURCE_DIR}/codec/console/enc/src/h264encoder.cpp
        ${PROJECT_SOURCE_DIR}/codec/console/common/src/read_config.cpp

        ${PROJECT_SOURCE_DIR}/codec/encoder/plus/src/OpenH264Encoder.cpp

        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/au_set.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/deblocking.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/decode_mb_aux.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/encode_mb_aux.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/encoder.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/encoder_data_tables.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/encoder_ext.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/get_intra_predictor.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/md.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/mv_pred.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/nal_encap.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/paraset_strategy.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/picture_handle.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/ratectl.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/ref_list_mgr_svc.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/sample.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/set_mb_syn_cabac.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/set_mb_syn_cavlc.cpp
#        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/slice_multi_threading.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/svc_base_layer_md.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/svc_enc_slice_segment.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/svc_encode_mb.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/svc_encode_slice.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/svc_mode_decision.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/svc_motion_estimate.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/svc_set_mb_syn_cabac.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/svc_set_mb_syn_cavlc.cpp
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/wels_preprocess.cpp
#        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/wels_task_base.cpp
#        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/wels_task_encoder.cpp
#        ${PROJECT_SOURCE_DIR}/codec/encoder/core/src/wels_task_management.cpp

        ${PROJECT_SOURCE_DIR}/codec/common/src/utils.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/sad_common.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/mc.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/intra_pred_common.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/expand_pic.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/deblocking_common.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/crt_util_safe_x.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/cpu.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/copy_mb.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/common_tables.cpp
        ${PROJECT_SOURCE_DIR}/codec/common/src/welsCodecTrace.cpp
#        ${PROJECT_SOURCE_DIR}/codec/common/src/WelsThread.cpp
#        ${PROJECT_SOURCE_DIR}/codec/common/src/WelsThreadLib.cpp
#        ${PROJECT_SOURCE_DIR}/codec/common/src/WelsTaskThread.cpp
#        ${PROJECT_SOURCE_DIR}/codec/common/src/WelsThreadPool.cpp

        ${PROJECT_SOURCE_DIR}/codec/processing/src/common/memory.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/common/WelsFrameWork.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/common/WelsFrameWorkEx.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/denoise/denoise.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/denoise/denoise_filter.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/complexityanalysis/ComplexityAnalysis.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/scenechangedetection/SceneChangeDetection.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/backgrounddetection/BackgroundDetection.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/downsample/downsample.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/downsample/downsamplefuncs.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/vaacalc/vaacalcfuncs.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/vaacalc/vaacalculation.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/scrolldetection/ScrollDetection.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/scrolldetection/ScrollDetectionFuncs.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/adaptivequantization/AdaptiveQuantization.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/imagerotate/imagerotate.cpp
        ${PROJECT_SOURCE_DIR}/codec/processing/src/imagerotate/imagerotatefuncs.cpp
        )
target_include_directories(h264enc PRIVATE
        ${PROJECT_SOURCE_DIR}/codec/encoder/core/inc
        ${PROJECT_SOURCE_DIR}/codec/encoder/plus/inc
        ${PROJECT_SOURCE_DIR}/codec/processing/interface
        ${PROJECT_SOURCE_DIR}/codec/common/inc
        ${PROJECT_SOURCE_DIR}/codec/console/common/inc
        ${PROJECT_SOURCE_DIR}/codec/api/svc
        ${PROJECT_SOURCE_DIR}/codec/processing/src/common/
        )
target_compile_options(h264enc PRIVATE -Ofast)

#mplayer out.yuv -demuxer rawvideo -rawvideo w=320:h=240:fps=30:format=iyuv -aspect 16:9
