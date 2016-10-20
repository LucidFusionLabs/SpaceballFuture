# $Id: CMakeLists.txt 1320 2014-10-26 08:12:52Z justin $

lfl_project(spaceball)

if(NOT LFL_MOBILE)
lfl_add_package(spaceballserv SOURCES spaceballserv.cpp
                COMPILE_DEFINITIONS -DLFL_BOX2D INCLUDE_DIRECTORIES ${BOX2D_INCLUDE}
                LINK_LIBRARIES ${LFL_APP_LIB} game app_null_framework app_null_graphics
                app_null_audio app_null_camera app_null_matrix app_null_fft
                app_simple_resampler app_simple_loader ${LFL_APP_CONVERT}
                app_null_png app_null_jpeg app_null_gif app_null_ogg app_null_css ${LFL_APP_FONT}
                app_null_ssl app_null_tu app_null_js ${BOX2D_LIB} ${LFL_APP_OS})
endif()

lfl_add_package(spaceball SOURCES spaceball.cpp
                COMPILE_DEFINITIONS -DLFL_BOX2D INCLUDE_DIRECTORIES ${BOX2D_INCLUDE}
                LINK_LIBRARIES ${LFL_APP_LIB} game ${LFL_APP_FRAMEWORK} ${LFL_APP_GRAPHICS}
                ${LFL_APP_AUDIO} app_null_camera app_null_matrix app_null_fft
                ${LFL_APP_GAME_LOADER} app_stb_ogg ${LFL_APP_CONVERT} app_null_css ${LFL_APP_FONT}
                app_null_ssl app_null_tu app_null_js ${BOX2D_LIB} ${LFL_APP_OS})

if(NOT LFL_MOBILE)
  set(LFL_APP_LIB_FILES ${LFL_APP_LIB_FILES} ${PORTAUDIO_LIB_FILES} ${OPENCV_LIB_FILES} ${FFMPEG_LIB_FILES})
endif()

lfl_post_build_start(spaceball spaceball Spaceball6006)
