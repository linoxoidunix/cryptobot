#include <string_view>
#include "aot/config/config.h"

using namespace std::literals;

config::BackTesting::BackTesting(std::string_view path_to_toml)
    : config(toml::parse_file(path_to_toml)) {}

std::string_view config::BackTesting::PathToPythonLib() {
    return config[kGeneralField][kPathToPythonLib].value_or(""sv);
}
std::string_view config::BackTesting::PathToPythonModule() {return ""sv;}
std::string_view config::BackTesting::PathToHistoryData() {return ""sv;}