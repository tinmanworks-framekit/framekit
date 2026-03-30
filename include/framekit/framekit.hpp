#pragma once

/*
 ______                 _    _ _ _
|  ____|               | |  (_) | |
| |__ _ __ __ _ _ __ __| | ___| | |_
|  __| '__/ _` | '__/ _` |/ / | | __|
| |  | | | (_| | | | (_|   <| | | |_
|_|  |_|  \__,_|_|  \__,_|\_\_|_|\__|
*/

// Application and process contracts.
#include "framekit/spec/app_spec.hpp"
#include "framekit/spec/ipc_transport.hpp"
#include "framekit/spec/process_role.hpp"
#include "framekit/spec/process_spec.hpp"
#include "framekit/spec/process_topology.hpp"

// Core runtime orchestration.
#include "framekit/multiprocess/runtime.hpp"
#include "framekit/kernel/runtime.hpp"
#include "framekit/fault/policy_runtime.hpp"
#include "framekit/lifecycle/state.hpp"
#include "framekit/lifecycle/state_machine.hpp"
#include "framekit/lifecycle/hooks.hpp"

// Loop, platform, input, and events.
#include "framekit/input/routing.hpp"
#include "framekit/loop/policy.hpp"
#include "framekit/loop/stage_graph.hpp"
#include "framekit/platform/host_runtime.hpp"
#include "framekit/event/bus.hpp"

// Services and module graph.
#include "framekit/service/context.hpp"
#include "framekit/service/execution.hpp"
#include "framekit/service/bootstrap.hpp"
#include "framekit/module/graph.hpp"

// Multiprocess support contracts.
#include "framekit/multiprocess/handshake.hpp"
#include "framekit/multiprocess/liveness.hpp"
#include "framekit/multiprocess/supervisor.hpp"
#include "framekit/multiprocess/launcher.hpp"

// IPC contracts.
#include "framekit/ipc/control.hpp"
#include "framekit/ipc/data.hpp"
#include "framekit/ipc/transport.hpp"
