#include "aot/Bybit.h"

#include "nlohmann/json.hpp"

std::string bybit::ArgsBody::Body() {
    nlohmann::json json(*this);
    return json.dump();
};