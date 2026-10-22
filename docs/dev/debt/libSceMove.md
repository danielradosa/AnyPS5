# libSceMove technical debt

### Functional

- [libSceMove](../../../core/libs/prx/libSceMove/Export.cpp): the host implements only disconnected controllers. The native module confirms initialization errors, disconnected return 1 without output writes, invalid-handle error 0x80EE0004, and initialization then argument then handle validation for queries. Reset-light-sphere still omits the device ioctl and its failure mapping.
- [sceMoveOpen and sceMoveClose](../../../core/libs/prx/libSceMove/Export.cpp): protected bodies remain unchanged. Native opening resolves a user/device through the input service, rejects duplicate device handles with 0x80EE0007, limits storage to eight slots with 0x80EE0005, and obtains handles from the device ioctl. The host generates monotonically increasing handles (`0x30B0100`, then every `0x100`, as in [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/f68923fe807f657348e6719ce658bcbd0a2b1c72/src/core/libraries/move/move.cpp)), accepts any user and arbitrary type/index values and returns 0x80EE0007 for duplicate slots. Closing an unknown handle returns the confirmed 0x80EE0004.
- [sceMoveReadStateRecent](../../../core/libs/prx/libSceMove/Export.cpp): the native module permits a null data array for SDK versions through 0x03ffffff while still requiring a count pointer and nonnegative timestamp. The host requires the data array for every SDK version. Connected state records and device-service failures remain unimplemented.

### Unknown function info

- [libSceMove](../../../core/libs/prx/libSceMove/Export.cpp) (libSceMove) - the export names are in sharpemu's PS5 name list ([scripts/ps5_names.txt](https://github.com/sharpemu/sharpemu/blob/85393c2ef87c101294229492595dcc32e9a3c92d/scripts/ps5_names.txt)) and their NIDs match the aerolib NID list; the signatures are PS4 ones from [orbital](https://github.com/AlexAltea/orbital/blob/cc408367da6a6ebbf5ac1ac559fefeadc73b2831/tools/ida/db_types.json#L1312-L1321), plus `sceMoveGetExtensionPortInfo(int32_t handle, void* data)` from shadPS4, unverified on a PS5
- [sceMoveOpen](../../../core/libs/prx/libSceMove/Export.cpp) (libSceMove) - accepts any user, as shadPS4 does, although the console's result for a user other than the initial user is not known. Closes when: measured on a PS5
