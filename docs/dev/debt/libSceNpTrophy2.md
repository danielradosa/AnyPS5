# libSceNpTrophy2 technical debt

### Silent stubs

- [libSceNpTrophy2](../../../core/libs/prx/libSceNpTrophy2/src/GameInfo.cpp): `sceNpTrophy2RegisterUnlockCallback` accepts a callback that is never called, as no trophy is ever unlocked, and `sceNpTrophy2UnregisterUnlockCallback` only returns success. `sceNpTrophy2ShowTrophyList` returns success without showing a list (there is no system trophy UI; seen on PPSA15552); its `(context)` signature follows [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/main/src/SharpProspero/Interop/Np/NpTrophy2.cs), as does `sceNpTrophy2GetRewardIcon`, which throws like the other icon getters as no icon files exist.
