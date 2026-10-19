# libSceVrTracker technical debt

### Silent stubs

- [sceVrTrackerInit, sceVrTrackerGetTime, sceVrTrackerGpuSubmit, sceVrTrackerGpuWait](../../../core/libs/prx/libSceVrTracker/Export.cpp) (libSceVrTracker) - prototypes from [orbital](https://github.com/AlexAltea/orbital/blob/cc408367da6a6ebbf5ac1ac559fefeadc73b2831/tools/ida/db_types.json); the parameter layouts, the memory sizes and alignment (onion 4 MiB, doubled with automatic Move calibration, garlic 48 MiB, work 16 MiB, all 64 KiB aligned), the compute pipe and queue limits (7 and 8) and the error codes follow the PS4 library in [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/vr_tracker/vr_tracker.cpp). Init checks the parameter as shadPS4 does and marks the library initialized without touching the memory; a null parameter answers `ARGUMENT_INVALID` (shadPS4 reads it unchecked), and shadPS4's check that each region is mapped for its whole size is left out. GetTime gives the process time. No camera frame can be valid (there is no camera library), so GpuSubmit answers `ARGUMENT_INVALID` without reading its parameter and GpuWait answers `NOT_EXECUTE_GPU_SUBMIT`, as in shadPS4. The library is never terminated (no `sceVrTrackerTerm` export). No PS5 title using it has been run

### Unknown function info

- [sceVrTrackerCpuProcess, sceVrTrackerGetResult](../../../core/libs/prx/libSceVrTracker/Export.cpp) (libSceVrTracker) - orbital names the prototypes `(const param*)` and `(const param*, result*)`, but shadPS4 has only stubs and no source documents the answer without a GPU result or a registered device (no `sceVrTrackerRegisterDevice` export exists here). They are declared without parameters and throw
