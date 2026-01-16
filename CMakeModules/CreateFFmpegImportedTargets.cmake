# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: James Turner <james@flightgear.org>

# helper function to find botht the static or import library and the DLL,
# needed on windows
function(find_ffmpeg_library retval basename)

    find_library(imported_fflib_${basename}
        NAMES ${basename}
        PATH_SUFFIXES lib
    )

    if (imported_fflib_${basename})
        set(${retval} ${imported_fflib_${basename}} PARENT_SCOPE)
    else()
        message(STATUS "Couldn't find link library for ${basename}")
        return()
    endif()

    if (MSVC)
        find_library(imported_ffdll_${basename}
            NAMES ${basename}
            PATH_SUFFIXES bin
        )

        if (imported_ffdll_${basename})
            set(${retval}_dll ${imported_ffdll_${basename}} PARENT_SCOPE)
        endif()
    endif()

endfunction()


set (Simgear_FFmpeg_Components avcodec avformat avutil swscale swresample)

foreach (comp ${Simgear_FFmpeg_Components})
    string(TOUPPER ${comp} _ffmpeg_module_UC)
    set(tgt imported_${_ffmpeg_module_UC})
    add_library(${tgt} SHARED IMPORTED GLOBAL)
    add_library(FFmpeg::${comp} ALIAS ${tgt})

    find_ffmpeg_library(releaseLib ${comp})

    set_property(TARGET ${tgt} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${${_ffmpeg_module_UC}_INCLUDE_DIRS})

# no worrying about debug/RelWithDbgInfo support for the moment,
# 99%, FFmpeg is built in release mode
    if (MSVC)
        set_property(TARGET ${tgt} PROPERTY IMPORTED_IMPLIB ${releaseLib})
        set_property(TARGET ${tgt} PROPERTY IMPORTED_LOCATION ${releaseLib_dll})
    else()
        set_property(TARGET ${tgt} PROPERTY IMPORTED_LOCATION ${releaseLib})
    endif()
endforeach()

target_link_libraries(imported_AVCODEC INTERFACE FFmpeg::swscale FFmpeg::swresample)

if (APPLE)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    find_library(COREMEDIA_FRAMEWORK CoreMedia)
    find_library(VT_FRAMEWORK VideoToolbox)
    find_library(AT_FRAMEWORK AudioToolbox)
    find_library(SECURITY_FRAMEWORK Security)
    find_library(iconv_lib iconv)
    find_library(bz_lib NAMES bz2 bzip2)
    find_library(x265_lib NAMES x265)

    #target_link_libraries(avformat INTERFACE -framework BZip2)
    target_link_libraries(imported_AVUTIL INTERFACE ${COREVIDEO_FRAMEWORK} ${VT_FRAMEWORK})
    target_link_libraries(imported_AVCODEC INTERFACE ${COREVIDEO_FRAMEWORK} ${COREAUDIO_FRAMEWORK} ${COREMEDIA_FRAMEWORK} ${SECURITY_FRAMEWORK} ${VT_FRAMEWORK} ${AT_FRAMEWORK} imported_SWSCALE ${iconv_lib} ${bz_lib} ${x265_lib})
endif()
