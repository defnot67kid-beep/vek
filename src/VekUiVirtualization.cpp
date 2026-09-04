#include <vek/VekUiVirtualization.h>

#include <algorithm>
#include <cmath>

namespace vek::ui {

VirtualRange ComputeVirtualRange(std::size_t itemCount, float itemSize, float scrollOffset,
                                  float viewportSize, std::size_t overscan) {
    VirtualRange range;
    range.totalContentSize = itemSize * static_cast<float>(itemCount);
    if (itemCount == 0 || itemSize <= 0.0f || viewportSize <= 0.0f) {
        return range;
    }
    scrollOffset = std::max(0.0f, std::min(scrollOffset, std::max(0.0f, range.totalContentSize - viewportSize)));

    std::size_t firstVisible = static_cast<std::size_t>(std::floor(scrollOffset / itemSize));
    std::size_t lastVisible = static_cast<std::size_t>(std::floor((scrollOffset + viewportSize) / itemSize));
    lastVisible = std::min(lastVisible, itemCount - 1);
    firstVisible = std::min(firstVisible, itemCount - 1);

    range.visibleCount = (lastVisible >= firstVisible) ? (lastVisible - firstVisible + 1) : 0;

    std::size_t firstRendered = (firstVisible >= overscan) ? (firstVisible - overscan) : 0;
    std::size_t lastRendered = std::min(itemCount - 1, lastVisible + overscan);

    range.firstIndex = firstRendered;
    range.lastIndex = lastRendered;
    range.renderedCount = lastRendered - firstRendered + 1;
    range.topOffset = static_cast<float>(firstRendered) * itemSize;
    range.empty = false;
    return range;
}

VirtualRange ComputeVirtualRangeVariable(const std::vector<float>& itemSizes, float scrollOffset,
                                          float viewportSize, std::size_t overscan) {
    VirtualRange range;
    std::size_t itemCount = itemSizes.size();
    std::vector<float> prefix(itemCount + 1, 0.0f);
    for (std::size_t i = 0; i < itemCount; ++i) prefix[i + 1] = prefix[i] + std::max(0.0f, itemSizes[i]);
    range.totalContentSize = prefix.back();
    if (itemCount == 0 || viewportSize <= 0.0f) return range;

    scrollOffset = std::max(0.0f, std::min(scrollOffset, std::max(0.0f, range.totalContentSize - viewportSize)));
    float scrollEnd = scrollOffset + viewportSize;

    // Binary search for the first index whose end offset exceeds scrollOffset.
    auto firstIt = std::upper_bound(prefix.begin(), prefix.end(), scrollOffset) - 1;
    std::size_t firstVisible = static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, firstIt - prefix.begin()));
    firstVisible = std::min(firstVisible, itemCount - 1);

    auto lastIt = std::upper_bound(prefix.begin(), prefix.end(), scrollEnd) - 1;
    std::size_t lastVisible = static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, lastIt - prefix.begin()));
    lastVisible = std::min(lastVisible, itemCount - 1);

    range.visibleCount = (lastVisible >= firstVisible) ? (lastVisible - firstVisible + 1) : 0;

    std::size_t firstRendered = (firstVisible >= overscan) ? (firstVisible - overscan) : 0;
    std::size_t lastRendered = std::min(itemCount - 1, lastVisible + overscan);

    range.firstIndex = firstRendered;
    range.lastIndex = lastRendered;
    range.renderedCount = lastRendered - firstRendered + 1;
    range.topOffset = prefix[firstRendered];
    range.empty = false;
    return range;
}

VirtualGridRange ComputeVirtualGridRange(std::size_t itemCount, float cellWidth, float cellHeight,
                                          float viewportWidth, float viewportHeight, float scrollOffset,
                                          std::size_t overscanRows) {
    VirtualGridRange grid;
    if (cellWidth <= 0.0f || itemCount == 0) return grid;
    std::size_t columns = std::max<std::size_t>(1, static_cast<std::size_t>(std::floor(viewportWidth / cellWidth)));
    grid.columnsPerRow = columns;
    std::size_t rowCount = (itemCount + columns - 1) / columns;
    grid.rows = ComputeVirtualRange(rowCount, cellHeight, scrollOffset, viewportHeight, overscanRows);
    return grid;
}

} // namespace vek::ui
