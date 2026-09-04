#pragma once
// VekUiReactive — VEK UI Next reactive state layer (VEK 3.0).
//
// Provides signals, computed (derived) values, and effects/watchers with
// automatic dependency tracking, so declarative UI can update only the
// nodes that actually depend on changed state instead of rebuilding the
// whole tree every frame. This module has no dependency on GuiNode/layout
// so it can be reused for non-UI reactive state as well.

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <vek/VekScriptEngine.h>

namespace vek::ui {

using ReactiveId = std::uint64_t;

// A single dirty-tracking node in the reactive graph. Signals, Computeds and
// Effects are all reactive nodes; Computeds and Effects additionally act as
// "subscribers" that re-run when a dependency they read notifies.
class ReactiveNode {
public:
    virtual ~ReactiveNode() = default;
    ReactiveId Id() const { return id_; }

protected:
    explicit ReactiveNode(ReactiveId id) : id_(id) {}
    ReactiveId id_;
};

// Central scheduler: tracks which reactive node is "currently running" so
// that reads of Signal::value/Computed::value register a dependency edge,
// and batches re-computation so a burst of writes only triggers one update
// pass per dependent (dirty-node coalescing).
class ReactiveScope {
public:
    static ReactiveScope& Instance();

    ReactiveId NextId() { return ++counter_; }

    // Called by Signal/Computed reads to register the currently-running
    // subscriber (effect/computed) as a dependent, if any is active.
    void TrackRead(ReactiveId sourceId, std::function<void()> onNotify, void* subscriberKey);

    // Marks a source dirty and (unless batching) immediately notifies all
    // subscribers that read it. Returns the number of subscribers notified.
    std::size_t NotifyWrite(ReactiveId sourceId);

    // Runs `fn` with `subscriberKey`/`onDirty` registered as the active
    // reader, so any Signal/Computed touched inside `fn` records a
    // dependency edge back to this subscriber. Used by Computed and Effect.
    void RunTracked(void* subscriberKey, const std::function<void()>& onDirty, const std::function<void()>& fn);

    // Batches writes: dependents are only notified once, after `fn` returns,
    // even if their sources were written to multiple times inside `fn`.
    void Batch(const std::function<void()>& fn);

    // Removes all dependency edges for a subscriber (used on unmount/dtor).
    void Unsubscribe(void* subscriberKey);

    // Diagnostics: total tracked edges, useful for VekUiProfiler-style stats.
    std::size_t EdgeCount() const { return edges_.size(); }

private:
    ReactiveScope() = default;

    struct Edge {
        ReactiveId sourceId;
        void* subscriberKey;
        std::function<void()> onDirty;
    };

    ReactiveId counter_ = 0;
    std::vector<Edge> edges_;

    // Active-subscriber stack for nested RunTracked calls (e.g. a computed
    // reading another computed).
    struct ActiveSubscriber {
        void* key;
        std::function<void()> onDirty;
    };
    std::vector<ActiveSubscriber> stack_;

    int batchDepth_ = 0;
    std::unordered_set<ReactiveId> pendingDirty_;
    std::unordered_set<void*> pendingSubscribers_;
};

// Signal<T> holds a value of type T (defaults to VekValue so it can carry
// any VEK scripting value: number/bool/string/array/map) and notifies
// subscribers on write. Reads inside a Computed/Effect register a
// dependency automatically.
template <typename T = VekValue>
class Signal : public ReactiveNode {
public:
    explicit Signal(T initial = T{})
        : ReactiveNode(ReactiveScope::Instance().NextId()), value_(std::move(initial)) {}

    const T& Peek() const { return value_; }

    const T& Get() {
        ReactiveScope::Instance().TrackRead(id_, nullptr, nullptr);
        return value_;
    }

    void Set(T v) {
        value_ = std::move(v);
        ReactiveScope::Instance().NotifyWrite(id_);
    }

    // Update via mutator, avoiding a copy for large maps/arrays.
    void Update(const std::function<void(T&)>& mutator) {
        mutator(value_);
        ReactiveScope::Instance().NotifyWrite(id_);
    }

private:
    T value_;
};

// Computed<T> derives a value from other Signals/Computeds. It is lazily
// re-evaluated the first time it's read after a dependency changed
// ("dirty" flag), so N reads between writes cost one recompute, not N.
template <typename T = VekValue>
class Computed : public ReactiveNode {
public:
    explicit Computed(std::function<T()> fn)
        : ReactiveNode(ReactiveScope::Instance().NextId()), fn_(std::move(fn)) {
        Recompute();
    }

    ~Computed() override { ReactiveScope::Instance().Unsubscribe(this); }

    const T& Get() {
        if (dirty_) Recompute();
        ReactiveScope::Instance().TrackRead(id_, nullptr, nullptr);
        return cached_;
    }

private:
    void Recompute() {
        ReactiveScope& scope = ReactiveScope::Instance();
        scope.Unsubscribe(this);
        T result{};
        scope.RunTracked(this, [this] { dirty_ = true; }, [&] { result = fn_(); });
        cached_ = std::move(result);
        dirty_ = false;
    }

    std::function<T()> fn_;
    T cached_{};
    bool dirty_ = true;
};

// Effect runs `fn` immediately, then re-runs it any time a Signal/Computed
// it read during the last run changes. Used for DOM-like side effects:
// pushing a computed value into a GuiNode's text/value/visual style.
class Effect : public ReactiveNode {
public:
    explicit Effect(std::function<void()> fn);
    ~Effect() override;

    void Rerun();

private:
    std::function<void()> fn_;
};

// UiWatch mirrors Effect but is explicitly named for "watch(signal, cb)"
// style call sites used from component lifecycle code.
std::shared_ptr<Effect> Watch(std::function<void()> fn);

} // namespace vek::ui
