#pragma once

#include "framekit/runtime/lifecycle_state.hpp"

#include <vector>

namespace framekit::runtime {

class LifecycleStateMachine {
public:
    LifecycleStateMachine();

    LifecycleState State() const;
    bool CanTransitionTo(LifecycleState next) const;
    bool TransitionTo(LifecycleState next);

    const std::vector<LifecycleState>& History() const;

private:
    LifecycleState state_ = LifecycleState::kStopped;
    std::vector<LifecycleState> history_;
};

} // namespace framekit::runtime
