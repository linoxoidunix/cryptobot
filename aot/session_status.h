#pragma once

namespace aot{
enum class StatusSession {
    Resolving,
    Connecting,
    Handshaking,
    Ready,
    Expired,
    Closing,
    Cancelling
};
};