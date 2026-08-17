module;

#include <meta>

export module ParameterAnnotations;

export struct Inject {
  int slot;
};

export void configure(int count [[= Inject{7}]]);
