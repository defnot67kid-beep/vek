#pragma once
// VekUiVirtualization — virtualization + scroll presentation metrics (VEK 3.1).

#include <cstddef>
#include <vector>

namespace vek::ui {

struct VirtualRange {
    std::size_t firstIndex = 0;
    std::size_t lastIndex = 0;
    std::size_t visibleCount = 0;
    std::size_t renderedCount = 0;
    float topOffset = 0.0f;
    float totalContentSize = 0.0f;
    float clampedScrollOffset = 0.0f;
    float beforeExtent = 0.0f;
    float afterExtent = 0.0f;
    bool empty = true;
};

VirtualRange ComputeVirtualRange(std::size_t itemCount, float itemSize, float scrollOffset,
                                 float viewportSize, std::size_t overscan = 4);
VirtualRange ComputeVirtualRangeVariable(const std::vector<float>& itemSizes, float scrollOffset,
                                         float viewportSize, std::size_t overscan = 4);

struct VirtualGridRange {
    VirtualRange rows;
    std::size_t columnsPerRow = 1;
    std::size_t FirstItemIndex() const { return rows.firstIndex * columnsPerRow; }
    std::size_t LastItemIndex(std::size_t itemCount) const {
        if (itemCount == 0) return 0;
        std::size_t idx = (rows.lastIndex + 1) * columnsPerRow;
        return std::min(idx, itemCount) - 1;
    }
};
VirtualGridRange ComputeVirtualGridRange(std::size_t itemCount, float cellWidth, float cellHeight,
                                         float viewportWidth, float viewportHeight, float scrollOffset,
                                         std::size_t overscanRows = 2);

// Presentation-independent scrollbar metrics. Hosts can use these for both
// ordinary scroll containers and virtualized lists/tables without knowing how
// many rows were materialized.
struct ScrollbarMetrics {
    float trackStart = 0.0f;
    float trackLength = 0.0f;
    float thumbStart = 0.0f;
    float thumbLength = 0.0f;
    float maxScroll = 0.0f;
    float normalized = 0.0f;
    bool scrollable = false;
};

ScrollbarMetrics ComputeScrollbarMetrics(float contentExtent, float viewportExtent, float scrollOffset,
                                          float trackLength, float minThumbLength = 28.0f);

struct ScrollShadowMetrics {
    float leadingOpacity = 0.0f;
    float trailingOpacity = 0.0f;
};

// Produces gentle edge-fade strengths (0..1) based on how much content is
// clipped before/after the viewport. fadeDistance controls how quickly the
// shadow reaches full strength.
ScrollShadowMetrics ComputeScrollShadowMetrics(float contentExtent, float viewportExtent, float scrollOffset,
                                                float fadeDistance = 24.0f);

// Overscan can increase slightly with scroll velocity to avoid pop-in during
// fast wheel/trackpad movement while remaining bounded for huge data sets.
std::size_t ComputeAdaptiveOverscan(std::size_t baseOverscan, float scrollVelocityPixelsPerSecond,
                                    float itemExtent, std::size_t maxOverscan = 32);

} // namespace vek::ui
