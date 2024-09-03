#include "aot/config/config.h"
#include "aot/Logger.h"

#include <string_view>

using namespace std::literals;

config::BackTesting::BackTesting(std::string_view path_to_toml)
    {
        try{
            config = (toml::parse_file(path_to_toml));
        }
        catch(...){
            loge("can't open file=\"{}\"", path_to_toml);
        }
    }

config::IPathToPythonLib::Answer config::BackTesting::PathToPythonLib() {
    auto path = config[kGeneralField][kPathToPythonLib].value_or(""sv);
    return {!path.empty(), path};
}
config::IPathToPythonModule::Answer config::BackTesting::PathToPythonModule() {
    auto path = config[kGeneralField][kPathToPythonModule].value_or(""sv);
    return {!path.empty(), path};
}
config::IPathToHistoryData::Answer config::BackTesting::PathToHistoryData() {
    auto path = config[kGeneralField][kPathToHistoricalData].value_or(""sv);
    return {!path.empty(), path};
}