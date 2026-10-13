# libSceAudioIn technical debt

### Silent stubs

- [sceAudioInInit](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - returns 0 without library-wide setup, as `sceAudioOutInit` does; each `sceAudioInOpen` opens its own SDL capture device

### Unknown function info

- [sceAudioInIsPadEmbeddedMic](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - name from its NID (`X+4jdIS75P0`); signature `int32_t(int32_t handle)` from the Ghost of Yotei (PPSA26344) call site in #2355. Astro Bot (PPSA21567) compares the result with 1 at both of its call sites and takes 1 to mean that the port captures from the pad's microphone. A port reports 1 when it captures from the host's default recording device and that device's name contains "DualSense", the match AudioOut2 uses to find the pad's speaker, and 0 otherwise, also without a device; unknown handles return `0x80260101` like the other AudioIn calls. The console's code for an unknown handle and whether the result follows a change of device after the port opened are unknown

### Functional

- [sceAudioInHqOpen, sceAudioInAsyncOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) open the same port as `sceAudioInOpen`, so `sceAudioInInput`, `sceAudioInGetSilentState` and `sceAudioInClose` accept their handles. The signatures agree across [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/fc5d2cc219c43c8c8f354153e7b3d0c04a5d506e/src/core/libraries/audio/audioin.cpp), [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/60977b29103452b9681ae10c7a9011d44965bf16/src/SharpProspero/Interop/Audio/AudioIn.cs) and [KytyPS5](https://github.com/KytyPS5/KytyPS5/blob/c86c8b4d9333a766cc8c4495f89f893bf35e6199/src/libs/audio.cpp). Both open it exactly as `sceAudioInOpen` does, with its checks, and AsyncOpen opens it synchronously; whether high-quality or asynchronous ports accept other types, sizes, rates or formats, or capture differently, is unknown. Host capture pacing and console behavior remain unverified.
