# libSceMbus technical debt

### Silent stubs

- [libSceMbus](../../../core/libs/prx/libSceMbus/Export.cpp) implements only the no-device path: initialization and termination succeed, no device events are reported, device queries return no entries and the device set generation stays 0

### Unknown function info

- [libSceMbus](../../../core/libs/prx/libSceMbus/Export.cpp) (`libSceDeviceService`) - signatures taken from PPSA01317, which calls `sceDeviceServiceInitialize(3, 0)`, polls `sceDeviceServiceGetEventState(1)` and queries again when bit 0 or 1 is set, and calls `sceDeviceServiceQueryDeviceInfo_(0x7001, 0, 0, infos, 1, &count, &unknown, 0x70)`, treating a negative result as an error and `count` as the number of entries. The host has no devices: the event state is 0 and a query returns 0 with `count` 0
- [sceDeviceServiceGetGeneration](../../../core/libs/prx/libSceMbus/Export.cpp) (libSceMbus) - the `int32_t (void)` signature follows [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Interop/Device/DeviceService.cs#L63-L65), which reads it as a generation counter that advances whenever the set of connected devices changes ([DeviceMonitor.cs](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Platform/DeviceMonitor.cs#L36-L48)). The host has no devices and the set never changes, so the counter stays at 0 and is never negative; the console's starting value is unknown, and initialization is not checked, as for the other calls
- [libSceMbus firmware 3.20 pending exports](../../../core/libs/prx/libSceMbus/PendingExports.cpp) - 115 symbol names are present in [the PS5 3.20 libSceMbus export list](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceMbus.c). Their signatures and console behaviour are unverified. The declarations use the existing no-parameter unknown-signature convention solely to resolve imports, and every call throws through `NotImplemented_nid_no_patch`, except `sceMbusTerm` (0) and `sceMbusAcquireControlList` (0x809B00FF), which return the firmware module's constant.
