#pragma once

#include "Options.hpp"

struct PlayerInput {
private:
    Arc<Options> options;

public:
    bool upKey = false;
    bool downKey = false;
    bool leftKey = false;
    bool rightKey = false;
    bool shiftKey = false;
    bool jumpingKey = false;

    f32 rightImpulse = 0.f;
    f32 forwardImpulse = 0.f;

public:
    explicit PlayerInput(Arc<Options> options) : options(std::move(options)) {}

    void tick() {
        upKey = options->keyUp->down;
        downKey = options->keyDown->down;
        leftKey = options->keyLeft->down;
        rightKey = options->keyRight->down;
        shiftKey = options->keyShift->down;
        jumpingKey = options->keyJumping->down;

        rightImpulse = (rightKey ? 1.f : 0.f) - (leftKey ? 1.f : 0.f);
        forwardImpulse = (upKey ? 1.f : 0.f) - (downKey ? 1.f : 0.f);
    }
};
