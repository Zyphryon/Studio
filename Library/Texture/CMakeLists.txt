## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
##
## This work is licensed under the terms of the MIT license.
##
## For a copy, see <https://opensource.org/licenses/MIT>.
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

CMAKE_MINIMUM_REQUIRED(VERSION 3.26)

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Project
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

PROJECT(Studio_Texture VERSION 1.0.0 LANGUAGES CXX)

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Target
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

ZyAddTarget(${PROJECT_NAME} STATIC)

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Dependency (Zyphryon Engine)
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC "ZyEngine")

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Dependency (STB) (PUBLIC DOMAIN)
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE
        "STB_IMAGE_STATIC"
        "STB_IMAGE_WRITE_STATIC"
        "STBI_NO_STDIO"
        "STBI_WRITE_NO_STDIO"
        "STBI_ONLY_PNG"
        "STBI_ONLY_JPEG"
        "STBI_ONLY_TGA"
        "STBI_ONLY_BMP"
        "STBI_ONLY_HDR"
        "STBI_MAX_DIMENSIONS=65535")

FetchContent_Declare(External_STB
        GIT_REPOSITORY https://github.com/nothings/stb
        GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(External_STB)

IF (NOT TARGET STB)

    ADD_LIBRARY(STB INTERFACE)
    TARGET_INCLUDE_DIRECTORIES(STB INTERFACE ${external_stb_SOURCE_DIR})

ENDIF ()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE "STB")

## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
## Dependency (bc7enc_rdo) (MIT OR PUBLIC DOMAIN)
## -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

## Only the block encoders are taken: rgbcx for BC1 to BC5, bc7enc for BC7.
FetchContent_Declare(External_BC7Enc
        GIT_REPOSITORY https://github.com/richgel999/bc7enc_rdo
        GIT_TAG        b9438627eef73a1157e84201b6fa6eb2ffd6d9f0
        SOURCE_SUBDIR  None)
FetchContent_MakeAvailable(External_BC7Enc)

IF (NOT TARGET BC7Enc)

    ADD_LIBRARY(BC7Enc STATIC ${external_bc7enc_SOURCE_DIR}/bc7enc.cpp ${external_bc7enc_SOURCE_DIR}/rgbcx.cpp)
    TARGET_INCLUDE_DIRECTORIES(BC7Enc SYSTEM PUBLIC ${external_bc7enc_SOURCE_DIR})

ENDIF ()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE "BC7Enc")