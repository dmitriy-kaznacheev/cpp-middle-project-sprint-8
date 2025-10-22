#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <streambuf>
#include <string>

using namespace std::string_literals;

// helper function to get file content
std::string file_contents(const std::string &file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return {};  // or throw an exception
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());

    return content;
}

// helper function to get refactored file content
std::string refactored_file_contents(const std::string &test_name) {
    auto src_file_path = "../tests/tests_data/"s + test_name + ".cpp"s;
    auto tmp_file_path = "../tests/tests_data/tmp/"s + test_name + "_tmp.cpp"s;
    std::filesystem::copy_file(src_file_path, tmp_file_path, std::filesystem::copy_options::overwrite_existing);

    std::string command = "./refactor_tool "s + tmp_file_path + " --"s;
    int result = system(command.c_str());
    EXPECT_EQ(result, 0) << "refactor_tool call failed";

    auto content = file_contents(tmp_file_path);
    std::filesystem::remove(tmp_file_path);

    return content;
}

class refactor_tool : public testing::TestWithParam<std::string> {};

TEST_P(refactor_tool, test) {
    auto test_name = GetParam();
    auto ref_file_path = "../tests/tests_data/"s + test_name + "_ref.cpp"s;

    auto actual = file_contents(ref_file_path);
    auto expected = refactored_file_contents(test_name);

    EXPECT_EQ(actual, expected);
}

INSTANTIATE_TEST_SUITE_P(test, refactor_tool, testing::Values("test1"s, "test2"s, "test3"s));