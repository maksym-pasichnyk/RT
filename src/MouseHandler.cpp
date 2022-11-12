#include "MouseHandler.hpp"

#include "GLFW/glfw3.h"

void MouseHandler::setIgnoreFirstMove() {
    ignoreFirstMove = true;
}

void MouseHandler::grabMouse() {
    if (mouseGrabbed) {
        return;
    }
    Application::pollEvents();

    auto [sx, sy] = window->getSize();
    cursor = glm::dvec2(glm::ivec2(sx, sy) / 2);

    mouseGrabbed = true;
    ignoreFirstMove = true;
    glfwGetCursorPos(window->handle, &cursor.x, &cursor.y);
    glfwSetInputMode(window->handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void MouseHandler::releaseMouse() {
    if (!mouseGrabbed) {
        return;
    }
    Application::pollEvents();

    auto [sx, sy] = window->getSize();
    cursor = glm::dvec2(glm::ivec2(sx, sy) / 2);

    mouseGrabbed = false;
    glfwSetCursorPos(window->handle, cursor.x, cursor.y);
    glfwSetInputMode(window->handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void MouseHandler::onMove(f64 x, f64 y) {
    if (ignoreFirstMove) {
        ignoreFirstMove = false;
        cursor = glm::dvec2(x, y);
    }
    if (mouseGrabbed) {
        delta += glm::dvec2(x, y) - cursor;
    }
    cursor = glm::vec2(x, y);
}

void MouseHandler::onPress(i32 button, i32 action, i32 mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            grabMouse();
        }
    }
}

auto MouseHandler::getMouseDelta() -> const glm::dvec2& {
    return delta;
}

void MouseHandler::resetMouseDelta() {
    delta = {};
}

auto MouseHandler::isMouseGrabbed() const -> bool {
    return mouseGrabbed;
}