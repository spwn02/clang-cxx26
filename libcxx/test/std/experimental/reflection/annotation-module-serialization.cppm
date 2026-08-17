module;

#include <meta>

export module AnnotationSerialization;

export struct Rename {
  char value[4];
};

export enum class Kind {
  Value [[= Rename{"new"}]],
};

export consteval auto extracted() -> Rename {
  constexpr auto annotation = std::meta::annotations_of(^^Kind::Value)[0];
  return std::meta::extract<Rename>(annotation);
}
