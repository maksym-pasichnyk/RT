#pragma once

#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "glm/gtx/euler_angles.hpp"

struct BlockPosition;
struct ChunkPosition;

struct BlockPosition {
public:
    i32 x = 0;
    i32 y = 0;
    i32 z = 0;

public:
    [[nodiscard]]
    auto getChunkPosition() const -> ChunkPosition;

    [[nodiscard]]
    auto move(i32 ox, i32 oy, i32 oz) const -> BlockPosition;

public:
    explicit operator glm::ivec3() const noexcept {
        return {x, y, z};
    }
    friend auto operator<=>(const BlockPosition&, const BlockPosition&) noexcept = default;
};

struct ChunkPosition {
public:
    i32 x = 0;
    i32 z = 0;

public:
    [[nodiscard]]
    auto getBlockPositionX(i32 offset) const -> i32;

    [[nodiscard]]
    auto getBlockPositionZ(i32 offset) const -> i32;

    [[nodiscard]]
    auto getStartPosition() const -> BlockPosition;

    [[nodiscard]]
    auto getEndPosition() const -> BlockPosition;

public:
    friend auto operator<=>(const ChunkPosition&, const ChunkPosition&) noexcept = default;
};

inline auto BlockPosition::getChunkPosition() const -> ChunkPosition {
    return {x >> 4, z >> 4};
}

inline auto BlockPosition::move(i32 ox, i32 oy, i32 oz) const -> BlockPosition {
    return {x + ox, y + oy, z + oz};
}

inline auto ChunkPosition::getBlockPositionX(i32 offset) const -> i32 {
    return (x << 4) + offset;
}

inline auto ChunkPosition::getBlockPositionZ(i32 offset) const -> i32 {
    return (z << 4) + offset;
}

inline auto ChunkPosition::getStartPosition() const -> BlockPosition {
    return {getBlockPositionX(0), 0, getBlockPositionZ(0)};
}

inline auto ChunkPosition::getEndPosition() const -> BlockPosition {
    return {getBlockPositionX(15), 0, getBlockPositionZ(15)};
}
