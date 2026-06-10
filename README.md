# can-ecu-sim

Simulated in-vehicle CAN bus network with RTOS-based ECU firmware running in QEMU - exploring real-time embedded systems for automotive software.

## Story

Modern vehicles are networks of small computers (ECUs) talking to each other over CAN bus - engine control, braking, climate, infotainment, all exchanging messages on a shared network with strict timing requirements.

This project builds that world from scratch: a CAN bus simulation layer, and ECU firmware running on an RTOS that produces, consumes, and reacts to CAN messages under real-time constraints. The goal is to understand and demonstrate the core problems of in-vehicle embedded software: message framing, scheduling, timing guarantees, and concurrency - not to build a toy CRUD app with a "vehicle" theme.

## Architecture

```
+----------------+        +----------------+        +----------------+
|   ECU Node A   |        |   CAN Bus Sim  |        |   ECU Node B   |
| (RTOS/QEMU)    | <----> | (virtual CAN,  | <----> | (RTOS/QEMU)    |
| sensor + logic |        |  message bus)  |        | actuator + logic|
+----------------+        +----------------+        +----------------+
                                  |
                                  v
                         +-----------------+
                         | Monitor/Logger  |
                         | (Dear ImGui GUI)|
                         +-----------------+
```

- **CAN Bus Simulator**: virtual message bus, frame format, multiple nodes publishing/subscribing
- **ECU Firmware**: RTOS tasks (sensor read, control loop, CAN tx/rx) with priority scheduling, running in QEMU
- **Monitor GUI**: real-time view of CAN traffic and ECU state (Dear ImGui)

## Tech Stack

- **Language**: C/C++17
- **RTOS**: FreeRTOS (or Zephyr - TBD during Phase 2)
- **Emulation**: QEMU
- **Bus simulation**: Linux SocketCAN (vcan)
- **Build**: CMake
- **GUI**: Dear ImGui
- **Platform**: macOS (host) + Lima Linux VM (build/run target, see Development Environment)

## Development Environment

This project builds and runs inside a Linux VM (vcan and CAN-bound code do not run on macOS directly).

```sh
limactl start --name=can-dev lima/can-dev.yaml   # one-time setup
limactl shell can-dev                            # enter the VM
cd ~/can-ecu-sim                                  # repo, mounted read-write from the host
```

`vcan0` is brought up automatically on every VM boot (see `lima/can-dev.yaml`). Edit files on macOS with your normal editor; build and run inside the VM shell.

## Decisions

Architectural choices get recorded here once made, with the reason. Keeps the rationale visible instead of buried in commit history.

| Decision | Choice | Reason |
|---|---|---|
| RTOS | TBD (Phase 2) | |
| Bus simulation backend | Linux SocketCAN (vcan) inside a Lima VM (Ubuntu, standard kernel) | Real Linux CAN stack, compatible with `can-utils`. Docker Desktop was ruled out: its LinuxKit kernel is built without `CONFIG_CAN_VCAN`, so `modprobe vcan` fails. Ubuntu's generic kernel ships `vcan` as a module. Lima also gives a real QEMU-backed Linux VM, reusable groundwork for Phase 2. |

## Roadmap

### Phase 1 - CAN Bus Simulator
- [x] Define CAN frame format (ID, DLC, data)
- [x] Virtual bus: multiple nodes can publish/subscribe to frames
- [x] CLI tool to send/receive/log frames
- [x] Basic tests for bus correctness (ordering, delivery)

### Phase 2 - ECU Firmware (RTOS on QEMU)
- [ ] Toolchain: cross-compile for QEMU target
- [ ] RTOS bring-up (boot, task scheduler running in QEMU)
- [ ] ECU task: periodic sensor read -> CAN tx
- [ ] ECU task: CAN rx -> actuator logic
- [ ] Demonstrate priority scheduling / timing behavior under load

### Phase 3 - Monitor GUI
- [ ] Dear ImGui app showing live CAN traffic
- [ ] ECU state visualization (task states, message rates)

## Success Criteria

- Two or more simulated ECUs exchange CAN messages over the bus simulator
- At least one ECU runs as RTOS firmware inside QEMU, not just a native binary
- System demonstrates a real-time constraint (e.g. periodic task meets its deadline, documented with measurements)
- Known limitations and design trade-offs are documented, not hidden
- GUI shows live system state for demo purposes

## Status

Phase 1 - in progress.
