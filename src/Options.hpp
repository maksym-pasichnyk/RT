#pragma once

#include "Core.hpp"
#include "KeyMapping.hpp"

struct Options {
    Arc<KeyMapping> keyUp = Arc<KeyMapping>::alloc();
    Arc<KeyMapping> keyDown = Arc<KeyMapping>::alloc();
    Arc<KeyMapping> keyLeft = Arc<KeyMapping>::alloc();
    Arc<KeyMapping> keyRight = Arc<KeyMapping>::alloc();
    Arc<KeyMapping> keyShift = Arc<KeyMapping>::alloc();
    Arc<KeyMapping> keyJumping = Arc<KeyMapping>::alloc();

    f32 gamma = 2.2f;
    f32 exposure = 1.0f;
};
