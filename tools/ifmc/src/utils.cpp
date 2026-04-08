#include <filesystem>
#include <system_error>

extern "C" int create_dir(const char* path, int can_exists) {
    std::error_code ec;
    std::filesystem::path p(path);
    if (!can_exists && std::filesystem::exists(p)) {
        return 17;
    }

    bool created = std::filesystem::create_directory(p, ec);

    if (ec) return ec.value(); 
    
    return 0;
}