cmake_minimum_required(VERSION 2.8)
project(my_service_v2)
find_package(qibuild)
find_package(qimodule)

qi_sanitize_compile_flags(HIDDEN_SYMBOLS)

qi_create_module(moduletest api.hpp moduletest.cpp SHARED DEPENDS QI NO_INSTALL)
