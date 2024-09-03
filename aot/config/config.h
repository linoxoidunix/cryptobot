#include <string_view>
#include <toml++/toml.hpp>

namespace config {
class IPathToPythonLib {
  public:
    virtual std::string_view PathToPythonLib() = 0;
    virtual ~IPathToPythonLib()                = default;
};

class IPathToPythonModule {
  public:
    virtual std::string_view PathToPythonModule() = 0;
    virtual ~IPathToPythonModule()                = default;
};

class IPathToHistoryData {
  public:
    virtual std::string_view PathToHistoryData() = 0;
    virtual ~IPathToHistoryData()                = default;
};

class BackTesting : public IPathToPythonLib,
                    public IPathToPythonModule,
                    public IPathToHistoryData {
    static constexpr std::string_view kGeneralField = "default";
    static constexpr std::string_view kPathToPythonLib = "path_to_python_lib";

    toml::table config;

  public:
    explicit BackTesting(std::string_view path_to_toml);

    std::string_view PathToPythonLib() override;
    std::string_view PathToPythonModule() override;
    std::string_view PathToHistoryData() override;

  private:
    BackTesting()                         = delete;
    BackTesting(const BackTesting& other) = delete;
};
}  // namespace config
