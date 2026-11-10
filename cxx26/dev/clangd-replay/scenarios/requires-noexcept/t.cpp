template <typename T, bool B>
concept C = requires(T t) { { t } noexcept(B); };
