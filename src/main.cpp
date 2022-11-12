#include "GameApplication.hpp"
#include "spdlog/spdlog.h"

auto main(i32 argc, char** argv) -> i32 {
    try {
        setenv("VFX_ENABLE_API_VALIDATION", "1", 1);

        auto game = Arc<GameApplication>::alloc();
        game->run();
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
    }
    return 0;
}