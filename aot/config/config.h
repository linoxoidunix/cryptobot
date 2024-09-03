#pragma once
#include <string_view>
#include <toml++/toml.hpp>

namespace config {
class IPathToPythonLib {
  public:
    /**
     * @brief bool - status, if true that is ok
     * std::string_view corresponded path
     */
    using Answer                     = std::pair<bool, std::string_view>;
    virtual Answer PathToPythonLib() = 0;
    virtual ~IPathToPythonLib()      = default;
};

class IPathToPythonModule {
  public:
    /**
     * @brief bool - status, if true that is ok
     * std::string_view corresponded path
     */
    using Answer                        = std::pair<bool, std::string_view>;
    virtual Answer PathToPythonModule() = 0;
    virtual ~IPathToPythonModule()      = default;
};

class IPathToHistoryData {
  public:
    /**
     * @brief bool - status, if true that is ok
     * std::string_view corresponded path
     */
    using Answer                       = std::pair<bool, std::string_view>;
    virtual Answer PathToHistoryData() = 0;
    virtual ~IPathToHistoryData()      = default;
};

class BackTesting : public IPathToPythonLib,
                    public IPathToPythonModule,
                    public IPathToHistoryData {
    static constexpr std::string_view kGeneralField    = "default";
    static constexpr std::string_view kPathToPythonLib = "path_to_python_lib";
    static constexpr std::string_view kPathToPythonModule =
        "path_to_python_module";
    static constexpr std::string_view kPathToHistoricalData =
        "path_to_history_data";

    toml::table config;

  public:
    explicit BackTesting(std::string_view path_to_toml);
    IPathToPythonLib::Answer PathToPythonLib() override;
    IPathToPythonModule::Answer PathToPythonModule() override;
    IPathToHistoryData::Answer PathToHistoryData() override;

  private:
    BackTesting()                                   = delete;
    BackTesting(const BackTesting& other)           = delete;
    BackTesting operator=(const BackTesting& other) = delete;
};
}  // namespace config
