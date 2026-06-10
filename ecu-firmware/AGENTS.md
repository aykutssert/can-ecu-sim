# ecu-firmware conventions

Build setup specific to this directory. See root [AGENTS.md](../AGENTS.md) for
general rules.

## Toolchain

Zephyr SDK + west workspace live in `~/zephyrproject` inside the `can-dev` Lima
VM (outside this repo - not committed, see root README's Development
Environment section). Build commands run from `~/zephyrproject` with the venv
activated:

```sh
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject
```

## sensor_node (native_sim, connected to vcan0)

Periodic CAN tx (sensor) + CAN rx callback (actuator). Uses Zephyr's native
SocketCAN driver (`CONFIG_CAN_NATIVE_LINUX`) via the `socketcan-native-sim`
snippet, bound to `vcan0` - the same bus the `can-bus` CLI tools use.

```sh
west build -p -b native_sim/native/64 -S socketcan-native-sim \
    ~/can-ecu-sim/ecu-firmware/sensor_node -d ~/can-ecu-sim/ecu-firmware/build-native

~/can-ecu-sim/ecu-firmware/build-native/zephyr/zephyr.exe --can-if=vcan0
```

Protocol: `0x100` SENSOR_SPEED (tx, 2 bytes LE counter, every 100ms),
`0x200` ACTUATOR_CMD (rx, 1 byte, 0/nonzero).

Note: target is `native_sim/native/64`, not plain `native_sim` - the Lima VM
is aarch64 with a 64-bit userspace, and plain `native_sim` defaults to a
32-bit build that fails to configure on this host.

## scheduling_demo (qemu_cortex_m3)

Priority scheduling / timing demo: a high-priority periodic task measures its
own wake-up jitter against an absolute deadline while a low-priority thread
continuously busy-waits.

```sh
west build -p -b qemu_cortex_m3 \
    ~/can-ecu-sim/ecu-firmware/scheduling_demo -d ~/can-ecu-sim/ecu-firmware/build-qemu

west build -d ~/can-ecu-sim/ecu-firmware/build-qemu -t run
```
