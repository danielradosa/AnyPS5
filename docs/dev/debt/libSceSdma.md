# libSceSdma technical debt

### Unknown function info

- [libSceSdma pending exports](../../../core/libs/prx/libSceSdma/Export.cpp) - 26 named callable exports are listed in the [PS5 3.20 export table](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceSdma.c) and were cross-checked against the same module in PS5 firmware 13.60. Their signatures and behaviour are unverified; the declarations use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`. No prototype for any `sceSdma*` export is in orbital db_types.json (cc408367), OpenOrbis, the ps5-payload-sdk stubs, SharpProspero 5dfdb4b0, KytyPS5, sharpemu or shadPS4 (aerolib names only); OpenAGC's `sceAgcDriverSdmaCopyLinearBlocking(dst, src, size)` belongs to libSceAgcDriver and says nothing about this library's queue, window or tiled copies.
