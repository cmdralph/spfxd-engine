#include <coffee/shader_preprocessor.h>

#include <iostream>
#include <string>

int main() {
    const std::string result = coffee::ShaderPreprocessor::from_source(
        "#version 330 core\nvoid main() {}\n", {}, {{"COFFEE_TEST", "42"}, {"FEATURE", ""}});
    const auto version = result.find("#version 330 core");
    const auto define = result.find("#define COFFEE_TEST 42");
    const auto body = result.find("void main()");
    if (version == std::string::npos || define == std::string::npos || body == std::string::npos ||
        !(version < define && define < body)) {
        std::cerr << "Shader define injection failed.\n";
        return 1;
    }
    std::cout << "All Coffee shader preprocessor tests passed.\n";
    return 0;
}
