#include <vek/VekUiReactive.h>

#include <algorithm>

namespace vek::ui {

ReactiveScope& ReactiveScope::Instance() {
    static ReactiveScope scope;
    return scope;
}

void ReactiveScope::TrackRead(ReactiveId sourceId, std::function<void()> /*onNotify*/, void* /*subscriberKey*/) {
    if (stack_.empty()) return;
    ActiveSubscriber& active = stack_.back();
    // Avoid duplicate edges for the same (source, subscriber) pair within a
    // single run so repeated reads of the same signal don't multiply fanout.
    for (const Edge& e : edges_) {
        if (e.sourceId == sourceId && e.subscriberKey == active.key) return;
    }
    edges_.push_back(Edge{sourceId, active.key, active.onDirty});
}

std::size_t ReactiveScope::NotifyWrite(ReactiveId sourceId) {
    if (batchDepth_ > 0) {
        pendingDirty_.insert(sourceId);
        return 0;
    }
    std::size_t count = 0;
    // Copy candidate callbacks first: a callback may itself mutate edges_
    // (e.g. a Computed recomputing and re-subscribing).
    std::vector<std::function<void()>> toRun;
    for (const Edge& e : edges_) {
        if (e.sourceId == sourceId && e.onDirty) {
            toRun.push_back(e.onDirty);
        }
    }
    for (auto& cb : toRun) {
        cb();
        ++count;
    }
    return count;
}

void ReactiveScope::RunTracked(void* subscriberKey, const std::function<void()>& onDirty, const std::function<void()>& fn) {
    stack_.push_back(ActiveSubscriber{subscriberKey, onDirty});
    fn();
    stack_.pop_back();
}

void ReactiveScope::Batch(const std::function<void()>& fn) {
    ++batchDepth_;
    fn();
    --batchDepth_;
    if (batchDepth_ == 0 && !pendingDirty_.empty()) {
        auto dirtySources = std::move(pendingDirty_);
        pendingDirty_.clear();
        // Collect the unique set of subscriber callbacks to run, so a
        // subscriber depending on several sources written in this batch
        // still only re-runs once (batching coalesces notifications).
        pendingSubscribers_.clear();
        std::vector<std::function<void()>> toRun;
        for (const Edge& e : edges_) {
            if (dirtySources.count(e.sourceId) && e.onDirty) {
                if (pendingSubscribers_.insert(e.subscriberKey).second) {
                    toRun.push_back(e.onDirty);
                }
            }
        }
        for (auto& cb : toRun) cb();
    }
}

void ReactiveScope::Unsubscribe(void* subscriberKey) {
    edges_.erase(std::remove_if(edges_.begin(), edges_.end(), [&](const Edge& e) {
        return e.subscriberKey == subscriberKey;
    }), edges_.end());
}

Effect::Effect(std::function<void()> fn)
    : ReactiveNode(ReactiveScope::Instance().NextId()), fn_(std::move(fn)) {
    Rerun();
}

Effect::~Effect() {
    ReactiveScope::Instance().Unsubscribe(this);
}

void Effect::Rerun() {
    ReactiveScope& scope = ReactiveScope::Instance();
    scope.Unsubscribe(this);
    scope.RunTracked(this, [this] { Rerun(); }, fn_);
}

std::shared_ptr<Effect> Watch(std::function<void()> fn) {
    return std::make_shared<Effect>(std::move(fn));
}

} // namespace vek::ui
