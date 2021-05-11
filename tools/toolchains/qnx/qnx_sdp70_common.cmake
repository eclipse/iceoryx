# Copyright (c) 2021 by Robert Bosch GmbH. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
SET(CMAKE_SYSTEM_NAME QNX)
SET(TOOLCHAIN QNX)

include(CMakeForceCompiler)
SET(CMAKE_SYSTEM_VERSION 7.0.0)

SET(CMAKE_SHARED_LIBRARY_PREFIX "lib")
SET(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
SET(CMAKE_STATIC_LIBRARY_PREFIX "lib")
SET(CMAKE_STATIC_LIBRARY_SUFFIX ".a")

if(NOT DEFINED ENV{QNX_HOST})
    message(FATAL_ERROR "Environment variable QNX_HOST not set!")
endif(NOT DEFINED ENV{QNX_HOST})
if(NOT DEFINED ENV{QNX_TARGET})
    message(FATAL_ERROR "Environment variable QNX_TARGET not set!")
endif(NOT DEFINED ENV{QNX_TARGET})

SET(QNX_HOST   "$ENV{QNX_HOST}")
SET(QNX_TARGET "$ENV{QNX_TARGET}")

SET(CMAKE_MAKE_PROGRAM "${QNX_HOST}/usr/bin/make" CACHE PATH "QNX Make Program")
SET(CMAKE_SH           "${QNX_HOST}/usr/bin/ksh"  CACHE PATH "QNX shell Program")

set(CMAKE_C_COMPILER "${QNX_HOST}/usr/bin/qcc")
set(CMAKE_C_COMPILER_TARGET ${arch})
set(CMAKE_CXX_COMPILER "${QNX_HOST}/usr/bin/q++")
set(CMAKE_CXX_COMPILER_TARGET ${arch})

SET(CMAKE_STRIP   "${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-strip" CACHE PATH "strip")
SET(CMAKE_AR      "${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-ar" CACHE PATH "archive")
SET(CMAKE_LINKER  "${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-ld" CACHE PATH "linker")
SET(CMAKE_NM      "${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-nm" CACHE PATH "nm")
SET(CMAKE_OBJCOPY "${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-objcopy" CACHE PATH "objcopy")
SET(CMAKE_OBJDUMP "${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-objdump" CACHE PATH "objdump")
SET(CMAKE_RANLIB  "${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-ranlib" CACHE PATH "ranlib")

SET(CMAKE_C_FLAGS_DEBUG "-g -O0 -fno-builtin")
SET(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-builtin")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

SET(CMAKE_FIND_ROOT_PATH ${QNX_TARGET})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

add_compile_options("-D_QNX_SOURCE=1")

# QNX 7 ships with QCC 5.4.0 (based on GCC). Cmake cannot detect the compiler features,
# so they're added manually. The correct detection can be done if you set as cross-compiler
# SET(CMAKE_CXX_COMPILER ${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-c++)
# The file with the detected compile features is then in the build directory named CMakeCXXCompiler.cmake
# A similar bug was reported for Android, see: https://bugreports.qt.io/browse/QTBUG-54666

set(CMAKE_CXX_COMPILE_FEATURES "cxx_std_98;cxx_template_template_parameters;cxx_std_11;cxx_alias_templates;cxx_alignas;cxx_alignof;cxx_attributes;cxx_auto_type;cxx_constexpr;cxx_decltype;cxx_decltype_incomplete_return_types;cxx_default_function_template_args;cxx_defaulted_functions;cxx_defaulted_move_initializers;cxx_delegating_constructors;cxx_deleted_functions;cxx_enum_forward_declarations;cxx_explicit_conversions;cxx_extended_friend_declarations;cxx_extern_templates;cxx_final;cxx_func_identifier;cxx_generalized_initializers;cxx_inheriting_constructors;cxx_inline_namespaces;cxx_lambdas;cxx_local_type_template_args;cxx_long_long_type;cxx_noexcept;cxx_nonstatic_member_init;cxx_nullptr;cxx_override;cxx_range_for;cxx_raw_string_literals;cxx_reference_qualified_functions;cxx_right_angle_brackets;cxx_rvalue_references;cxx_sizeof_member;cxx_static_assert;cxx_strong_enums;cxx_thread_local;cxx_trailing_return_types;cxx_unicode_literals;cxx_uniform_initialization;cxx_unrestricted_unions;cxx_user_literals;cxx_variadic_macros;cxx_variadic_templates;cxx_std_14;cxx_aggregate_default_initializers;cxx_attribute_deprecated;cxx_binary_literals;cxx_contextual_conversions;cxx_decltype_auto;cxx_digit_separators;cxx_generic_lambdas;cxx_lambda_init_captures;cxx_relaxed_constexpr;cxx_return_type_deduction;cxx_variable_templates")
set(CMAKE_CXX98_COMPILE_FEATURES "cxx_std_98;cxx_template_template_parameters")
set(CMAKE_CXX11_COMPILE_FEATURES "cxx_std_11;cxx_alias_templates;cxx_alignas;cxx_alignof;cxx_attributes;cxx_auto_type;cxx_constexpr;cxx_decltype;cxx_decltype_incomplete_return_types;cxx_default_function_template_args;cxx_defaulted_functions;cxx_defaulted_move_initializers;cxx_delegating_constructors;cxx_deleted_functions;cxx_enum_forward_declarations;cxx_explicit_conversions;cxx_extended_friend_declarations;cxx_extern_templates;cxx_final;cxx_func_identifier;cxx_generalized_initializers;cxx_inheriting_constructors;cxx_inline_namespaces;cxx_lambdas;cxx_local_type_template_args;cxx_long_long_type;cxx_noexcept;cxx_nonstatic_member_init;cxx_nullptr;cxx_override;cxx_range_for;cxx_raw_string_literals;cxx_reference_qualified_functions;cxx_right_angle_brackets;cxx_rvalue_references;cxx_sizeof_member;cxx_static_assert;cxx_strong_enums;cxx_thread_local;cxx_trailing_return_types;cxx_unicode_literals;cxx_uniform_initialization;cxx_unrestricted_unions;cxx_user_literals;cxx_variadic_macros;cxx_variadic_templates")
set(CMAKE_CXX14_COMPILE_FEATURES "cxx_std_14;cxx_aggregate_default_initializers;cxx_attribute_deprecated;cxx_binary_literals;cxx_contextual_conversions;cxx_decltype_auto;cxx_digit_separators;cxx_generic_lambdas;cxx_lambda_init_captures;cxx_relaxed_constexpr;cxx_return_type_deduction;cxx_variable_templates")
