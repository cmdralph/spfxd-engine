#include <coffee/asset_locator.h>
#include <coffee/file.h>
#include <coffee/log.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

void capture_log(const coffee::log_record& record, void* user_data) noexcept {
    auto* captured = static_cast<std::string*>(user_data);
    captured->assign(record.message);
}

} // namespace

int main() {
    const auto unique = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto root = std::filesystem::temp_directory_path() /
                      ("coffee-core-tests-" + unique);

    try {
        coffee::ensure_directory(root / "nested");
        coffee::write_text_file(root / "nested" / "message.txt", "coffee");
        const std::array<std::byte, 4> bytes{
            std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};
        coffee::write_binary_file(root / "data.bin", bytes);

        expect(coffee::read_text_file(root / "nested" / "message.txt") == "coffee",
               "text file round trip");
        expect(coffee::read_binary_file(root / "data.bin").size() == bytes.size(),
               "binary file round trip");

        coffee::AssetLocator assets(root);
        expect(assets.contains("nested/message.txt"), "asset lookup");
        expect(assets.read_text("nested/message.txt") == "coffee", "asset text read");

        std::string captured;
        coffee::set_log_level(coffee::log_level::trace);
        coffee::set_log_callback(capture_log, &captured);
        coffee::log_debug("test", "captured");
        expect(captured == "captured", "custom log callback");
        coffee::set_log_callback(nullptr);
        coffee::set_log_level(coffee::log_level::info);
    } catch (const std::exception& exception) {
        std::cerr << "FAILED with exception: " << exception.what() << '\n';
        ++failures;
    }

    std::error_code cleanup_error;
    std::filesystem::remove_all(root, cleanup_error);

    if (failures == 0) {
        std::cout << "All Coffee core tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
