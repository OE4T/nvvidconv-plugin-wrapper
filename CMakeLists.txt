# SPDX-License-Identifier: MIT
# Copyright (c) 2021, OpenEmbedded for Tegra Project

cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0048 NEW)
project(nvvidconv_plugin_wrapper LANGUAGES C VERSION 1.0.0)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libv4l2 REQUIRED IMPORTED_TARGET libv4l2)

add_library(v4l2_nvvidconv_wrapper SHARED nvvidconv_plugin_wrapper.c)
target_include_directories(v4l2_nvvidconv_wrapper PUBLIC PkgConfig::libv4l2)
target_compile_definitions(v4l2_nvvidconv_wrapper PRIVATE _GNU_SOURCE)
target_link_libraries(v4l2_nvvidconv_wrapper PUBLIC ${CMAKE_DL_LIBS})
install(TARGETS v4l2_nvvidconv_wrapper LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/libv4l/plugins)
