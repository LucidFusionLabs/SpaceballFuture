# $Id: CMakeLists.txt 1320 2014-10-26 08:12:52Z justin $

if(LFL_IOS)
  set(CRASHREPORTING_LIB app_fabric_crashreporting)
else()
  set(CRASHREPORTING_LIB app_null_crashreporting)
endif()

if(LFL_MOBILE)
  set(BILLING_LIB ${LFL_APP_BILLING})
  set(ADVERTISING_LIB ${LFL_APP_ADVERTISING})
else()
  set(BILLING_LIB app_null_billing)
  set(ADVERTISING_LIB app_null_advertising)
endif()

lfl_set_os_toolkit(SPACEBALL)
lfl_project(SpaceballFuture)

if(NOT LFL_MOBILE)
lfl_add_package(SpaceballFutureServer SOURCES spaceballserv.cpp
                COMPILE_DEFINITIONS -DLFL_BOX2D INCLUDE_DIRECTORIES ${BOX2D_INCLUDE}
                LINK_LIBRARIES ${LFL_APP_LIB} game app_null_framework app_null_graphics
                app_null_toolkit app_null_audio app_null_camera app_null_matrix app_null_fft
                app_simple_resampler app_simple_loader ${LFL_APP_CONVERT}
                app_null_png app_null_jpeg app_null_gif app_null_ogg app_null_css ${LFL_APP_FONT}
                app_null_ssl app_null_tu app_null_js app_null_crashreporting
                ${BOX2D_LIB} ${SPACEBALL_TOOLKIT} ${LFL_APP_OS})
endif()

if(NOT LFL_MOBILE)
  set(SPACEBALL_LIB_FILES ${PORTAUDIO_LIB_FILES} ${OPENCV_LIB_FILES} ${FFMPEG_LIB_FILES})
endif()

lfl_add_package(SpaceballFuture SOURCES spaceball.cpp
                COMPILE_DEFINITIONS -DLFL_BOX2D INCLUDE_DIRECTORIES ${BOX2D_INCLUDE}
                LINK_LIBRARIES ${LFL_APP_LIB} game ${SPACEBALL_FRAMEWORK} ${SPACEBALL_GRAPHICS}
                ${LFL_APP_AUDIO} app_null_camera app_null_matrix app_null_fft
                ${LFL_APP_GAME_LOADER} app_stb_ogg ${LFL_APP_CONVERT} app_null_css ${LFL_APP_FONT}
                app_null_ssl app_null_tu app_null_js ${BILLING_LIB} ${ADVERTISING_LIB} ${LFL_APP_NAG}
                ${CRASHREPORTING_LIB} ${BOX2D_LIB} ${SPACEBALL_TOOLKIT} ${LFL_APP_OS}
                ASSET_FILES ${LFL_APP_ASSET_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/assets/*
                LIB_FILES ${LFL_APP_LIB_FILES} ${SPACEBALL_LIB_FILES})
lfl_post_build_start(SpaceballFuture)
