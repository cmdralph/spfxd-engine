#include <coffee/shader_preprocessor.h>
#include <coffee/error.h>
#include <coffee/file.h>

#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace coffee {

namespace {

struct preprocess_state {
    std::unordered_set<std::string> once_files;
    std::vector<std::filesystem::path> include_stack;
};

std::string expand(std::string_view source, const std::filesystem::path& directory,
                   preprocess_state& state, int depth) {
    if (depth > 32) throw error(error_code::invalid_operation, "Shader include depth exceeded 32 files.");
    std::istringstream input{std::string(source)};
    std::ostringstream output;
    std::string line;
    int line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const auto first = line.find_first_not_of(" \t");
        const std::string_view trimmed = first == std::string::npos ? std::string_view{} : std::string_view(line).substr(first);
        if (trimmed == "#pragma once") continue;
        if (!trimmed.starts_with("#include")) {
            output << line << '\n';
            continue;
        }

        const auto quote_begin = trimmed.find('"');
        const auto quote_end = quote_begin == std::string_view::npos ? std::string_view::npos : trimmed.find('"', quote_begin + 1);
        if (quote_begin == std::string_view::npos || quote_end == std::string_view::npos) {
            throw error(error_code::shader_compilation, "Malformed shader #include on line " + std::to_string(line_number) + '.');
        }
        const auto included = std::filesystem::weakly_canonical(directory / std::string(trimmed.substr(quote_begin + 1, quote_end - quote_begin - 1)));
        const std::string key = included.generic_string();
        const std::string included_source = read_text_file(included);
        if (included_source.find("#pragma once") != std::string::npos && state.once_files.contains(key)) continue;
        if (std::find(state.include_stack.begin(), state.include_stack.end(), included) != state.include_stack.end()) {
            throw error(error_code::shader_compilation, "Cyclic shader include detected at: " + key);
        }
        if (included_source.find("#pragma once") != std::string::npos) state.once_files.insert(key);
        state.include_stack.push_back(included);
        output << "\n#line 1\n" << expand(included_source, included.parent_path(), state, depth + 1)
               << "\n#line " << line_number + 1 << "\n";
        state.include_stack.pop_back();
    }
    return output.str();
}

std::string inject_defines(std::string source, const ShaderPreprocessor::define_map& defines) {
    if (defines.empty()) return source;
    std::ostringstream block;
    std::vector<std::string> names;
    names.reserve(defines.size());
    for (const auto& [name, value] : defines) { static_cast<void>(value); names.push_back(name); }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        const auto& value = defines.at(name);
        if (name.empty() || name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos) {
            throw error(error_code::invalid_argument, "Shader define names may contain only letters, numbers and underscores.");
        }
        block << "#define " << name;
        if (!value.empty()) block << ' ' << value;
        block << '\n';
    }
    const auto version_end = source.starts_with("#version") ? source.find('\n') : std::string::npos;
    if (version_end == std::string::npos) return block.str() + source;
    source.insert(version_end + 1, block.str());
    return source;
}

} // namespace

std::string ShaderPreprocessor::from_file(const std::filesystem::path& path, const define_map& defines) {
    const auto absolute = std::filesystem::weakly_canonical(path);
    preprocess_state state;
    state.include_stack.push_back(absolute);
    const std::string source = read_text_file(absolute);
    if (source.find("#pragma once") != std::string::npos) state.once_files.insert(absolute.generic_string());
    return inject_defines(expand(source, absolute.parent_path(), state, 0), defines);
}

std::string ShaderPreprocessor::from_source(std::string_view source,
                                            const std::filesystem::path& include_directory,
                                            const define_map& defines) {
    preprocess_state state;
    return inject_defines(expand(source, include_directory, state, 0), defines);
}

} // namespace coffee
