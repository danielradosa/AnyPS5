# libSceDiscMap technical debt

### Functional

- [libSceDiscMap](../../../core/libs/prx/libSceDiscMap/Export.cpp): loading disc bitmap metadata and resolving file byte ranges to bitmap blocks are not implemented. `sceDiscMapIsRequestOnHDD` and `ioKMruft1ek` implement the confirmed no-bitmap path: return `SCE_DISC_MAP_ERROR_NO_BITMAP_INFO` before validating any argument and leave every output unchanged. The supplied module confirms signed 64-bit byte offset/size, a 32-bit `IsRequestOnHDD` result and three 64-bit outputs for `ioKMruft1ek`. The latter outputs describe the bitmap selector, first block and block count. Its exported source name remains unavailable. `sceDiscMapGetPackageSize` gives the same answer and writes nothing. CUSA02636's bundled `libSceFios2.prx` imports `sceDiscMapIsRequestOnHDD` and `ioKMruft1ek`; what it does with the error is not known (shadPS4 returns the same, and [shadps4-game-compatibility #617](https://github.com/shadps4-compatibility/shadps4-game-compatibility/issues/617) lists CUSA02636 as in-game)

### Unknown function info

- [sceDiscMapGetPackageSize](../../../core/libs/prx/libSceDiscMap/Export.cpp) (libSceDiscMap) - the `(int64_t flags, int* out1, int* out2)` prototype and the `NO_BITMAP_INFO` answer follow the PS4 function in [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/disc_map/disc_map.cpp#L12-L14), the same answer as the other two calls; the meaning of the flags and outputs (and their widths) and the PS5 result are not measured
