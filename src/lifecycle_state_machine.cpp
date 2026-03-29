#include "framekit/lifecycle/state_machine.hpp"

namespace framekit::runtime {

namespace {

bool IsLegalTransition(LifecycleState current, LifecycleState next) {
    switch (current) {
        case LifecycleState::kBootstrapping:
            return next == LifecycleState::kInitializing || next == LifecycleState::kFaulted;
        case LifecycleState::kInitializing:
            return next == LifecycleState::kRunning ||
                   next == LifecycleState::kStopping ||
                   next == LifecycleState::kFaulted;
        case LifecycleState::kRunning:
            return next == LifecycleState::kStopping || next == LifecycleState::kFaulted;
        case LifecycleState::kStopping:
            return next == LifecycleState::kStopped || next == LifecycleState::kFaulted;
        case LifecycleState::kStopped:
            return next == LifecycleState::kBootstrapping;
        case LifecycleState::kFaulted:
            return next == LifecycleState::kStopping ||
                   next == LifecycleState::kStopped ||
                   next == LifecycleState::kBootstrapping;
    }

    return false;
}

} // namespace

LifecycleStateMachine::LifecycleStateMachine()
    : history_{LifecycleState::kStopped} {}

LifecycleState LifecycleStateMachine::State() const {
    return state_;
}

bool LifecycleStateMachine::CanTransitionTo(LifecycleState next) const {
    return IsLegalTransition(state_, next);
}

bool LifecycleStateMachine::TransitionTo(LifecycleState next) {
    if (!CanTransitionTo(next)) {
        return false;
    }

    state_ = next;
    history_.push_back(next);
    return true;
}

const std::vector<LifecycleState>& LifecycleStateMachine::History() const {
    return history_;
}

} // namespace framekit::runtime
