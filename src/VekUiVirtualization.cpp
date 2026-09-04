#include <vek/VekUiVirtualization.h>

#include <algorithm>
#include <cmath>

namespace vek::ui {
namespace {
float ClampScroll(float content, float viewport, float offset) {
    const float maxScroll = std::max(0.0f, content - std::max(0.0f, viewport));
    if (!std::isfinite(offset)) offset = 0.0f;
    return std::clamp(offset, 0.0f, maxScroll);
}
void FinishExtents(VirtualRange& r, float viewport) {
    r.beforeExtent = std::max(0.0f, r.clampedScrollOffset);
    r.afterExtent = std::max(0.0f, r.totalContentSize - (r.clampedScrollOffset + viewport));
}
}

VirtualRange ComputeVirtualRange(std::size_t itemCount, float itemSize, float scrollOffset,
                                 float viewportSize, std::size_t overscan) {
    VirtualRange range;
    range.totalContentSize = std::max(0.0f, itemSize) * static_cast<float>(itemCount);
    if (itemCount == 0 || itemSize <= 0.0f || viewportSize <= 0.0f) return range;
    range.clampedScrollOffset = ClampScroll(range.totalContentSize, viewportSize, scrollOffset);
    const float end = std::max(range.clampedScrollOffset, range.clampedScrollOffset + viewportSize - 0.001f);
    std::size_t firstVisible = std::min(itemCount - 1, static_cast<std::size_t>(std::floor(range.clampedScrollOffset / itemSize)));
    std::size_t lastVisible = std::min(itemCount - 1, static_cast<std::size_t>(std::floor(end / itemSize)));
    range.visibleCount = lastVisible >= firstVisible ? lastVisible - firstVisible + 1 : 0;
    range.firstIndex = firstVisible >= overscan ? firstVisible - overscan : 0;
    range.lastIndex = std::min(itemCount - 1, lastVisible + overscan);
    range.renderedCount = range.lastIndex - range.firstIndex + 1;
    range.topOffset = static_cast<float>(range.firstIndex) * itemSize;
    range.empty = false;
    FinishExtents(range, viewportSize);
    return range;
}

VirtualRange ComputeVirtualRangeVariable(const std::vector<float>& itemSizes, float scrollOffset,
                                         float viewportSize, std::size_t overscan) {
    VirtualRange range;
    const std::size_t itemCount = itemSizes.size();
    std::vector<float> prefix(itemCount + 1, 0.0f);
    for (std::size_t i = 0; i < itemCount; ++i) prefix[i + 1] = prefix[i] + std::max(0.0f, itemSizes[i]);
    range.totalContentSize = prefix.back();
    if (itemCount == 0 || viewportSize <= 0.0f) return range;
    range.clampedScrollOffset = ClampScroll(range.totalContentSize, viewportSize, scrollOffset);
    const float scrollEnd = range.clampedScrollOffset + viewportSize;
    auto firstIt = std::upper_bound(prefix.begin(), prefix.end(), range.clampedScrollOffset) - 1;
    auto lastIt = std::upper_bound(prefix.begin(), prefix.end(), std::max(range.clampedScrollOffset, scrollEnd - 0.001f)) - 1;
    std::size_t firstVisible = std::min(itemCount - 1, static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, firstIt - prefix.begin())));
    std::size_t lastVisible = std::min(itemCount - 1, static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, lastIt - prefix.begin())));
    range.visibleCount = lastVisible >= firstVisible ? lastVisible - firstVisible + 1 : 0;
    range.firstIndex = firstVisible >= overscan ? firstVisible - overscan : 0;
    range.lastIndex = std::min(itemCount - 1, lastVisible + overscan);
    range.renderedCount = range.lastIndex - range.firstIndex + 1;
    range.topOffset = prefix[range.firstIndex];
    range.empty = false;
    FinishExtents(range, viewportSize);
    return range;
}

VirtualGridRange ComputeVirtualGridRange(std::size_t itemCount, float cellWidth, float cellHeight,
                                         float viewportWidth, float viewportHeight, float scrollOffset,
                                         std::size_t overscanRows) {
    VirtualGridRange grid;
    if (cellWidth <= 0.0f || cellHeight <= 0.0f || viewportWidth <= 0.0f || itemCount == 0) return grid;
    grid.columnsPerRow = std::max<std::size_t>(1, static_cast<std::size_t>(std::floor(viewportWidth / cellWidth)));
    const std::size_t rowCount = (itemCount + grid.columnsPerRow - 1) / grid.columnsPerRow;
    grid.rows = ComputeVirtualRange(rowCount, cellHeight, scrollOffset, viewportHeight, overscanRows);
    return grid;
}

ScrollbarMetrics ComputeScrollbarMetrics(float contentExtent, float viewportExtent, float scrollOffset,
                                          float trackLength, float minThumbLength) {
    ScrollbarMetrics m;
    contentExtent = std::max(0.0f, contentExtent); viewportExtent = std::max(0.0f, viewportExtent);
    m.trackLength = std::max(0.0f, trackLength); m.maxScroll = std::max(0.0f, contentExtent - viewportExtent);
    if (contentExtent <= 0.0f || viewportExtent <= 0.0f || m.trackLength <= 0.0f || m.maxScroll <= 0.0f) {
        m.thumbLength = m.trackLength; return m;
    }
    m.scrollable = true;
    const float clamped = ClampScroll(contentExtent, viewportExtent, scrollOffset);
    m.normalized = m.maxScroll > 0.0f ? clamped / m.maxScroll : 0.0f;
    m.thumbLength = std::clamp(m.trackLength * (viewportExtent / contentExtent), std::max(1.0f, minThumbLength), m.trackLength);
    m.thumbStart = (m.trackLength - m.thumbLength) * m.normalized;
    return m;
}

ScrollShadowMetrics ComputeScrollShadowMetrics(float contentExtent, float viewportExtent, float scrollOffset,
                                                float fadeDistance) {
    ScrollShadowMetrics s;
    if (contentExtent <= viewportExtent || viewportExtent <= 0.0f) return s;
    const float clamped = ClampScroll(contentExtent, viewportExtent, scrollOffset);
    const float after = std::max(0.0f, contentExtent - (clamped + viewportExtent));
    fadeDistance = std::max(1.0f, fadeDistance);
    s.leadingOpacity = std::clamp(clamped / fadeDistance, 0.0f, 1.0f);
    s.trailingOpacity = std::clamp(after / fadeDistance, 0.0f, 1.0f);
    return s;
}

std::size_t ComputeAdaptiveOverscan(std::size_t baseOverscan, float scrollVelocityPixelsPerSecond,
                                    float itemExtent, std::size_t maxOverscan) {
    if (itemExtent <= 0.0f || maxOverscan <= baseOverscan) return std::min(baseOverscan, maxOverscan);
    const float rowsPerSecond = std::abs(scrollVelocityPixelsPerSecond) / itemExtent;
    const std::size_t extra = static_cast<std::size_t>(std::ceil(std::min(12.0f, rowsPerSecond * 0.12f)));
    return std::min(maxOverscan, baseOverscan + extra);
}

} // namespace vek::ui
