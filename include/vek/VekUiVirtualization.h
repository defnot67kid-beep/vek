#pragma once
// VekUiVirtualization — VEK UI Next virtualization (VEK 3.0).
//
// Computes the visible row/window range for lists, trees, tables and grids
// so hosts only materialize GuiNodes for items actually on/near screen
// (plus an overscan margin), regardless of total item count. A 50,000-row
// VirtualList should only ever produce on the order of (viewport/rowHeight
// + 2*overscan) live nodes.

#include <cstddef>
#include <vector>

namespace vek::ui {

struct VirtualRange {
    std::size_t firstIndex = 0;
    std::size_t lastIndex = 0;      // inclusive; lastIndex < firstIndex means empty
    std::size_t visibleCount = 0;   // items actually in the viewport (no overscan)
    std::size_t renderedCount = 0;  // items including overscan - what should be materialized
    float topOffset = 0.0f;         // pixel offset of firstIndex from content top
    float totalContentSize = 0.0f;  // total scrollable extent, for scrollbar sizing
    bool empty = true;
};

// Uniform-row-height virtualization (the common case: VirtualList / simple
// tables). `scrollOffset` and `viewportSize` are in the same units as
// `itemSize` (pixels). `overscan` is extra items rendered above/below the
// viewport to avoid pop-in during fast scroll/keyboard navigation.
VirtualRange ComputeVirtualRange(std::size_t itemCount, float itemSize, float scrollOffset,
                                  float viewportSize, std::size_t overscan = 4);

// Variable-row-height virtualization (trees/tables with mixed content).
// `itemSizes` must have `itemCount` entries. O(n) - callers should cache
// prefix sums for large datasets rather than calling this every frame;
// VekUiProfiler's layout_time_ms stat should be used to confirm cost.
VirtualRange ComputeVirtualRangeVariable(const std::vector<float>& itemSizes, float scrollOffset,
                                          float viewportSize, std::size_t overscan = 4);

// 2D virtualization for grids/asset browsers laid out in a fixed-column
// wrap (uniform cell size). Returns the range of *rows* to render; callers
// multiply by columnsPerRow to get the item index range.
struct VirtualGridRange {
    VirtualRange rows;
    std::size_t columnsPerRow = 1;
    std::size_t FirstItemIndex() const { return rows.firstIndex * columnsPerRow; }
    std::size_t LastItemIndex(std::size_t itemCount) const {
        std::size_t idx = (rows.lastIndex + 1) * columnsPerRow;
        return idx == 0 ? 0 : std::min(idx, itemCount) - 1;
    }
};

VirtualGridRange ComputeVirtualGridRange(std::size_t itemCount, float cellWidth, float cellHeight,
                                          float viewportWidth, float viewportHeight, float scrollOffset,
                                          std::size_t overscanRows = 2);

} // namespace vek::ui
