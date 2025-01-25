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