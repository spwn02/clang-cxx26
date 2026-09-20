// RUN: %clang_cc1 -std=c++20 -emit-module-interface %s -o %t.pcm

module;
#define MODULE_NAME not_the_module_name

export module MODULE_NAME;

export int value;
