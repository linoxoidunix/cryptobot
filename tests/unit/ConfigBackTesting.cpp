#include <iostream>
#include <fstream> 
#include <string_view>
#include "gtest/gtest.h"

#include "aot/Logger.h"
#include "aot/config/config.h"

using namespace std::literals;

constexpr std::string_view default_config{
    "[default]\npath_to_python_lib = \"path_to_python_lib\"\npath_to_python_module = \"path_to_python_module\"\npath_to_history_data = \"path_to_history_data\"\n"
};

TEST(ConfigBackTesting, ReadEmptyString) {
    using namespace config;
    EXPECT_NO_THROW(BackTesting backtesting(""sv));
    fmtlog::poll();
}

TEST(ConfigBackTesting, PathToPythonLibForEmptyString) {
    using namespace config;
    BackTesting config(""sv);
    auto [status, path] = config.PathToPythonLib();
    EXPECT_EQ(status, false);
    fmtlog::poll();
}

TEST(ConfigBackTesting, PathToPythonModuleForEmptyString) {
    using namespace config;
    BackTesting config(""sv);
    auto [status, path] = config.PathToPythonModule();
    EXPECT_EQ(status, false);
    fmtlog::poll();
}

TEST(ConfigBackTesting, PathToHistoryDataForEmptyString) {
    using namespace config;
    BackTesting config(""sv);
    auto [status, path] = config.PathToHistoryData();
    EXPECT_EQ(status, false);
    fmtlog::poll();
}

TEST(ConfigBackTesting, PathToPythonLibForNotEmptyFile) {
    using namespace config;
    std::ofstream outfile ("test.toml");
    outfile << default_config;
    outfile.close();
    BackTesting config("test.toml"sv);
    auto [status, path] = config.PathToPythonLib();
    EXPECT_EQ(status, true);
    EXPECT_EQ(path, "path_to_python_lib");
    fmtlog::poll();
    std::remove("test.toml");
}

TEST(ConfigBackTesting, PathToPythonModuleForNotEmptyFile) {
    using namespace config;
    std::ofstream outfile ("test.toml");
    outfile << default_config;
    outfile.close();
    BackTesting config("test.toml"sv);
    auto [status, path] = config.PathToPythonModule();
    EXPECT_EQ(status, true);
    EXPECT_EQ(path, "path_to_python_module");
    fmtlog::poll();
    std::remove("test.toml");
}

TEST(ConfigBackTesting, PathToHistoreDataForNotEmptyFile) {
    using namespace config;
    std::ofstream outfile ("test.toml");
    outfile << default_config;
    outfile.close();
    BackTesting config("test.toml"sv);
    auto [status, path] = config.PathToHistoryData();
    EXPECT_EQ(status, true);
    EXPECT_EQ(path, "path_to_history_data");
    fmtlog::poll();
    std::remove("test.toml");
}


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}