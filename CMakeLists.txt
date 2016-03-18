# $Id: CMakeLists.txt 1320 2014-10-26 08:12:52Z justin $

lfl_project(spaceball)
add_definitions(${LFL_APP_DEF} -DLFL_BOX2D)
include_directories(${LFL_APP_INCLUDE} ${BOX2D_INCLUDE})

lfl_add_target(spaceballserv spaceballserv.cpp)
target_link_libraries(spaceballserv ${LFL_APP_LIB} app_null_framework app_null_graphics
                      app_null_audio app_null_camera app_null_matrix app_null_fft
                      app_simple_resampler app_simple_loader ${LFL_APP_CONVERT}
                      app_null_png app_null_jpeg app_null_gif app_null_css app_null_ttf
                      app_null_ssl app_null_tu app_null_js ${BOX2D_LIB} ${LFL_APP_OS})

lfl_add_target(spaceball spaceball.cpp)
target_link_libraries(spaceball ${LFL_APP_LIB} ${LFL_APP_FRAMEWORK} ${LFL_APP_GRAPHICS}
                      ${LFL_APP_AUDIO} app_null_camera app_null_matrix app_null_fft
                      app_ffmpeg_resampler app_ffmpeg_loader ${LFL_APP_CONVERT}
                      app_libpng_png app_libjpeg_jpeg app_null_gif app_null_css app_null_ttf
                      app_null_ssl app_null_tu app_null_js ${BOX2D_LIB} ${LFL_APP_OS})
set(LFL_APP_LIB_FILES ${LFL_APP_LIB_FILES} ${PORTAUDIO_LIB_FILES} ${OPENCV_LIB_FILES} ${FFMPEG_LIB_FILES})
lfl_post_build_start(spaceball spaceball Spaceball6006)
