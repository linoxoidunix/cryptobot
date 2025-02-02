#pragma once

namespace aot{
enum class StatusSession {
    Resolving,
    Connecting,
    Handshaking,
    Ready,
    kInProcess,
    Expired,
    Closing,
    Cancelling
};
};

template <>
class fmt::formatter<aot::StatusSession> {
  public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(const aot::StatusSession& foo,
                          Context& ctx) const {

        return fmt::format_to(ctx.out(),
                              "StatusSession[{}]",
                              magic_enum::enum_name(foo));
    }
};