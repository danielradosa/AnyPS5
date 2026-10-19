# libSceMove technical debt

### Silent stubs

- [sceMoveClose, sceMoveGetExtensionPortInfo, sceMoveReadStateLatest, sceMoveReadStateRecent, sceMoveResetLightSphere and sceMoveSetLightSphere](../../../core/libs/prx/libSceMove/Export.cpp) (libSceMove) - the library cannot be initialized: `sceMoveInit` and `sceMoveOpen` are not exported, though the PS5 has them ([sharpemu ps5_names.txt](https://github.com/sharpemu/sharpemu/blob/5052fae0718ee17eb4c7fae0a52b472738c64aaa/scripts/ps5_names.txt#L134806-L134809)), so every call returns `NOT_INIT` (0x80EE0001) before it reads an argument, as the PS4 module does before `sceMoveInit` in [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/move/move.cpp#L47-L112). After init shadPS4 answers as if no Move controller is connected. Closes when: sceMoveInit and sceMoveOpen are exported

### Unknown function info

- [sceMoveClose, sceMoveReadStateLatest, sceMoveReadStateRecent, sceMoveResetLightSphere and sceMoveSetLightSphere](../../../core/libs/prx/libSceMove/Export.cpp) (libSceMove) - signatures from [orbital](https://github.com/AlexAltea/orbital/blob/cc408367da6a6ebbf5ac1ac559fefeadc73b2831/tools/ida/db_types.json#L1312-L1319) and [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/move/move.cpp), which also gives `sceMoveGetExtensionPortInfo(int32_t handle, void* data)`; all are PS4 prototypes, unverified on a PS5
