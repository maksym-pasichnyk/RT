#pragma once

#include "Core.hpp"
#include "Application.hpp"

struct MouseHandler {
public:
    explicit MouseHandler(const Arc<Window>& window) : window(window) {}

public:
    void grabMouse();
    void releaseMouse();
    void setIgnoreFirstMove();

    void onMove(f64 x, f64 y);
    void onPress(i32 button, i32 action, i32 mods);

    [[nodiscard]]
    auto getMouseDelta() -> const glm::dvec2&;
    void resetMouseDelta();

    [[nodiscard]]
    auto isMouseGrabbed() const -> bool;

private:
    Arc<Window> window;

    glm::dvec2 delta = {};
    glm::dvec2 cursor = {};

    bool mouseGrabbed = false;
    bool ignoreFirstMove = true;
};
