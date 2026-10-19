# libSceMove technical debt

### Functional

- [libSceMove](../../../core/libs/prx/libSceMove/Export.cpp) (libSceMove) - no Move controller is emulated, and the answers follow the PS4 module in [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/move/move.cpp#L15-L121), not a PS5 measurement. Before `sceMoveInit`, and after `sceMoveTerm`, every call returns `NOT_INIT` (0x80EE0001); a second `sceMoveInit` returns `ALREADY_INIT`. `sceMoveOpen` checks neither the user nor the port and returns a new handle each time, from 0x30b0100 up in steps of 0x100. No call checks the handle. The reads, `sceMoveGetDeviceInfo`, `sceMoveGetExtensionPortInfo`, `sceMoveSetLightSphere` and `sceMoveSetVibration` return `NO_CONTROLLER_CONNECTED` (1) after their argument checks, and `sceMoveResetLightSphere` and `sceMoveClose` succeed. Closes when: measured on a PS5

### Unknown function info

- [libSceMove](../../../core/libs/prx/libSceMove/Export.cpp) (libSceMove) - the exports are in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceMove.c); the signatures are PS4 ones from [orbital](https://github.com/AlexAltea/orbital/blob/cc408367da6a6ebbf5ac1ac559fefeadc73b2831/tools/ida/db_types.json#L1312-L1321), plus `sceMoveGetExtensionPortInfo(int32_t handle, void* data)` from shadPS4, unverified on a PS5
