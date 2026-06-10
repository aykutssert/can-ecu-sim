# can-ecu-sim

Simulated in-vehicle CAN bus network with Zephyr RTOS-based ECU firmware - exploring real-time embedded systems for automotive software.

## Story

Modern vehicles are networks of small computers (ECUs) talking to each other over CAN bus - engine control, braking, climate, infotainment, all exchanging messages on a shared network with strict timing requirements.

This project builds that world from scratch: a CAN bus simulation layer, ECU firmware running on an RTOS that produces, consumes, and reacts to CAN messages under real-time constraints, and a native GUI for live monitoring. The focus is on the core problems of in-vehicle embedded software: message framing, scheduling, timing guarantees, and concurrency.

## Architecture

```
+----------------+        +----------------+        +----------------+
|   ECU Node A   |        |   CAN Bus Sim  |        |   ECU Node B   |
| (Zephyr,       | <----> | (virtual CAN,  | <----> | (Zephyr,       |
|  native_sim)   |        |  vcan0)        |        |  native_sim)   |
| sensor + logic |        |                |        | actuator + logic|
+----------------+        +----------------+        +----------------+
                                  |
                                  v
                         +-----------------+        +-----------------+
                         | TCP Bridge      | <----> | Monitor GUI     |
                         | (Lima VM,       |  TCP   | (Dear ImGui,    |
                         |  vcan0 -> lines)|        |  macOS native)  |
                         +-----------------+        +-----------------+
```

- **CAN Bus Simulator**: virtual message bus, frame format, multiple nodes publishing/subscribing
- **ECU Firmware**: Zephyr RTOS tasks (sensor read, control loop, CAN tx/rx) with priority scheduling. Two build targets: `native_sim` (CAN tx/rx tasks, connected to vcan0 via Zephyr's native CAN driver) and `qemu_cortex_m3` (scheduler/timing demo under real emulation)
- **Monitor GUI**: real-time view of CAN traffic and ECU state (Dear ImGui), running natively on macOS. A small TCP bridge (`monitor-gui/bridge`, runs in the Lima VM) reads `vcan0` and streams `ID#HEXDATA` lines over TCP through Lima's automatic port forwarding to the macOS GUI.

## Tech Stack

- **Language**: C/C++17
- **RTOS**: Zephyr
- **Emulation**: QEMU (`qemu_cortex_m3`, for the scheduling/timing demo)
- **Bus simulation**: Linux SocketCAN (vcan)
- **Build**: CMake
- **GUI**: Dear ImGui + GLFW + OpenGL3 (native macOS app, FetchContent for ImGui)
- **Platform**: macOS (host, runs the GUI) + Lima Linux VM (bus simulator, ECU firmware, TCP bridge)

## Development Environment

This project builds and runs inside a Linux VM (vcan and CAN-bound code do not run on macOS directly).

```sh
limactl start --name=can-dev lima/can-dev.yaml   # one-time setup
limactl shell can-dev                            # enter the VM
cd ~/can-ecu-sim                                  # repo, mounted read-write from the host
```

`vcan0` is brought up automatically on every VM boot (see `lima/can-dev.yaml`). Edit files on macOS with your normal editor; build and run inside the VM shell.

To reclaim disk space when the VM is no longer needed:

```sh
limactl stop can-dev
limactl delete can-dev
```

### Running the Monitor GUI (Phase 3)

In the Lima VM, build and run the bridge against `vcan0`:

```sh
cmake -S . -B build && cmake --build build -j4
./build/monitor-gui/bridge/can_monitor_bridge vcan0 35000
```

(Optionally also run `./ecu-firmware/build-native/zephyr/zephyr.exe --can-if=vcan0`
to generate live traffic, see Phase 2.)

On macOS, build and run the GUI (requires `cmake`, `glfw` - `brew install glfw`):

```sh
cd monitor-gui/app
cmake -S . -B build && cmake --build build -j4
./build/can_monitor_gui 35000
```

The GUI connects to `127.0.0.1:35000`, which Lima forwards to the bridge's
listen socket inside the VM.

## Decisions

Architectural choices get recorded here once made, with the reason. Keeps the rationale visible instead of buried in commit history.

| Decision | Choice | Reason |
|---|---|---|
| RTOS | Zephyr | Industrial automotive RTOS (used by Bosch, NXP, Nordic), not a teaching toy like the bare-metal alternatives. Has a built-in CAN subsystem, active QEMU board support, and a native Linux simulator target. |
| RTOS <-> CAN bus integration | `native_sim` board with `CONFIG_CAN_NATIVE_LINUX` (socketcan-native-sim snippet), connected to the same `vcan0` as Phase 1 | QEMU's CAN controller emulation (SJA1000/Kvaser PCI via `can-host-socketcan`) is x86-PCI-only with no confirmed Zephyr driver - too fragile to build on. Zephyr's native_sim CAN driver is official, documented, and binds directly to a host SocketCAN interface, so the firmware talks to the exact same bus as the Phase 1 tools. The QEMU target (`qemu_cortex_m3`) is kept for what it's actually good at: demonstrating RTOS scheduling/timing under real cross-compiled emulation, without CAN. |
| Bus simulation backend | Linux SocketCAN (vcan) inside a Lima VM (Ubuntu, standard kernel) | Real Linux CAN stack, compatible with `can-utils`. Docker Desktop was ruled out: its LinuxKit kernel is built without `CONFIG_CAN_VCAN`, so `modprobe vcan` fails. Ubuntu's generic kernel ships `vcan` as a module. Lima also gives a real QEMU-backed Linux VM, reusable groundwork for Phase 2. |
| Monitor GUI runs on macOS, not in the Lima VM | A small TCP bridge process in the Lima VM forwards `vcan0` traffic as text lines; the GUI is a native macOS app connecting to `127.0.0.1:<port>` | Dear ImGui + GLFW + OpenGL need a real display; the Lima VM is headless. Verified first that Lima auto-forwards VM-bound TCP ports to macOS `127.0.0.1` (critical assumption, checked before writing any GUI code) - this makes the split low-risk. |

## Roadmap

### Phase 1 - CAN Bus Simulator
- [x] Define CAN frame format (ID, DLC, data)
- [x] Virtual bus: multiple nodes can publish/subscribe to frames
- [x] CLI tool to send/receive/log frames
- [x] Basic tests for bus correctness (ordering, delivery)

### Phase 2 - ECU Firmware (Zephyr RTOS)
- [x] Toolchain: Zephyr SDK + west workspace set up in the Lima VM
- [x] `native_sim` target: ECU task connected to vcan0 (CAN_NATIVE_LINUX), exchanging frames with Phase 1 CLI tools
- [x] ECU task: periodic sensor read -> CAN tx
- [x] ECU task: CAN rx -> actuator logic
- [x] `qemu_cortex_m3` target: RTOS bring-up (boot, task scheduler running under QEMU)
- [x] Demonstrate priority scheduling / timing behavior under load (qemu_cortex_m3)

### Phase 3 - Monitor GUI
- [x] Dear ImGui app showing live CAN traffic
- [x] ECU state visualization (task states, message rates)

## Success Criteria

- Two or more simulated ECUs exchange CAN messages over the bus simulator
- ECU logic runs as Zephyr RTOS firmware (not bespoke application code), with at least one target cross-compiled and run under QEMU emulation
- System demonstrates a real-time constraint (e.g. periodic task meets its deadline, documented with measurements)
- Known limitations and design trade-offs are documented, not hidden
- GUI shows live system state for demo purposes

## Phase 2 Results

`ecu-firmware/sensor_node` (native_sim, on vcan0) and the Phase 1 CLI tools act as
two ECU nodes on the same bus:

- `sensor_node` sends `0x100` (counter) every 100ms - visible live in `can_dump`
- `can_send vcan0 200#01` / `200#00` is received by `sensor_node`'s CAN rx
  filter and logged as `actuator: ON` / `actuator: OFF`

`ecu-firmware/scheduling_demo` (qemu_cortex_m3) runs a high-priority periodic
task (5ms period) alongside a low-priority thread that continuously
busy-waits. Measured wake-up jitter against an absolute deadline over 500
iterations:

```
periodic task: 500 iterations, period 5ms, high prio 1 vs load prio 5
wakeup jitter: avg=0us worst=0us (tick=100us)
```

The high-priority task meets its deadline every iteration despite the
low-priority thread never yielding - preemptive priority scheduling works as
expected.

**Limitations:**
- Jitter resolution is bounded by the kernel tick (100us at
  `CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000`); sub-tick jitter cannot be observed.
- QEMU's clock is not wall-clock real time, so absolute timing numbers do not
  transfer to real hardware - the result demonstrates *relative* scheduling
  behavior (deadline met under contention), not real-world latency bounds.
- `sensor_node` and the bus-simulator CLI tools both run as Linux processes on
  the same host (native_sim); only the scheduling demo runs as cross-compiled
  firmware under emulation.

## Phase 3 Results

`monitor-gui/bridge` (Lima VM) opens `vcan0` and a TCP listen socket; on connect
it streams every frame as `ID#HEXDATA\n`. `monitor-gui/app` (macOS) connects to
that socket through Lima's port forwarding, parses the stream on a background
thread, and renders:

- Connection status (connected / retrying)
- "ECU State": live decode of `0x100` (sensor counter, little-endian uint16)
  and `0x200` (actuator ON/OFF)
- A live traffic table (ID, DLC, data bytes, frame count, rate in Hz) for every
  CAN ID seen

End-to-end run: `sensor_node` (native_sim) + `can_monitor_bridge` in the Lima
VM, `can_monitor_gui` on macOS connected to `127.0.0.1:35000`. The GUI showed
`connected=1` and the `0x100` row updating at the firmware's ~100ms tx rate
with an increasing frame count.

**Limitations:**
- `can_monitor_bridge` serves one client at a time and detects a dropped
  client lazily (only on the next failed `send()`); reconnecting quickly after
  killing the GUI can leave stale half-closed sockets for a short while. Not
  an issue for normal single-GUI use.
- The `0x200` actuator row only populates once something sends a `200#01` /
  `200#00` frame on the bus (e.g. via `can_send`, see Phase 2 Results) -
  `sensor_node` only consumes it, it does not produce it.

## Status

Phase 1 - done. Phase 2 - done. Phase 3 - done.
