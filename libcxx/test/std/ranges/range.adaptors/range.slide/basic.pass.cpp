// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-ranges

#include <array>
#include <cassert>
#include <ranges>
#include <utility>
#include <vector>

int main() {
  std::array<int, 5> input{1, 2, 3, 4, 5};
  auto windows = input | std::ranges::views::slide(3);
  assert(windows.size() == 3);
  int first = 1;
  for (auto window : windows) {
    assert(*window.begin() == first);
    assert(*std::ranges::next(window.begin(), 2) == first + 2);
    ++first;
  }

  std::array<int, 2> short_input{1, 2};
  assert(std::ranges::views::slide(short_input, 3).size() == 0);

  // Regression test: slide_view<V>::__iterator advertised
  // random_access_iterator_tag for a random-access base without actually
  // implementing the full random-access iterator operation set
  // (operator-=, operator-(iterator, difference_type), operator[], and
  // the ordering operators) -- random_access_range<slide_view<V>> was
  // therefore false despite the iterator's own iterator_concept claiming
  // otherwise.
  {
    using Base = std::ranges::ref_view<std::vector<int>>;
    static_assert(std::ranges::random_access_range<Base>);
    using Slide = std::ranges::slide_view<Base>;
    static_assert(std::ranges::random_access_range<Slide>);
    static_assert(std::ranges::sized_range<Slide>);

    std::vector<int> v{1, 2, 3, 4, 5};
    auto sv = v | std::ranges::views::slide(3);
    auto it = sv.begin();

    auto it2 = it + 2;
    assert((*it2)[0] == 3);
    assert(it2 - it == 2);
    it2 -= 1;
    assert((*it2)[0] == 2);
    assert(it < it2);
    assert(it2 > it);
    assert(it <= it);
    assert(it2 >= it2);

    auto w = sv[2];
    assert(w[0] == 3 && w[1] == 4 && w[2] == 5);
  }

  // Regression test: slide_view<V>::__sentinel had no default constructor,
  // so it wasn't semiregular and couldn't satisfy sentinel_for -- for any
  // non-common base (e.g. views::repeat), slide_view::end() returns a
  // __sentinel, so the whole slide_view failed to model range at all.
  {
    using Base = decltype(std::views::repeat(1));
    static_assert(std::ranges::view<Base>);
    static_assert(std::ranges::random_access_range<Base>);
    static_assert(!std::ranges::common_range<Base>);

    using Window = decltype(std::declval<Base>() | std::ranges::views::slide(3));
    static_assert(std::ranges::range<Window>);
    static_assert(std::ranges::view<Window>);

    auto windowed = std::views::repeat(1) | std::ranges::views::slide(3);
    auto it        = windowed.begin();
    assert((*it)[0] == 1);
    assert((*it)[2] == 1);
  }
}
