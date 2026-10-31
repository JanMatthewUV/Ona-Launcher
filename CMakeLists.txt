cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{VITASDK})
    message(FATAL_ERROR "VITASDK is not set. Install VitaSDK and export VITASDK before configuring.")
endif()

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

project(ona_launcher C)

set(VITA_APP_NAME "Ona Launcher")
set(VITA_TITLEID "ONA000001")
set(VITA_VERSION "01.00")

add_executable(${PROJECT_NAME}
    src/main.c
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Werror
    -std=c11
)

target_link_libraries(${PROJECT_NAME}
    vita2d
    SceCtrl_stub
    SceDisplay_stub
    SceGxm_stub
    SceIofilemgr_stub
    SceTouch_stub
    SceAppMgr_stub
    SceCommonDialog_stub
    ScePgf_stub
    m
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    sce_sys/param.sfo
)
