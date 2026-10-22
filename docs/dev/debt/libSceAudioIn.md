# libSceAudioIn technical debt

### Silent stubs

- [sceAudioInInit](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - returns 0 without library-wide setup, as `sceAudioOutInit` does; each `sceAudioInOpen` opens its own SDL capture device

### Unknown function info

- [sceAudioInOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - formats 1 and 2 (16-bit mono and stereo) and 0x11 and 0x12 (float mono and stereo) are the ones [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Interop/Audio/AudioIn.cs#L19-L32) lists. Format 0x10 opens a float mono port, the same as 0x11, as [KytyPS5](https://github.com/KytyPS5/KytyPS5/blob/df5b9b92be7dfd463b47756b707d6d356a9f0550/src/libs/audio.cpp#L1039-L1046) decodes it; KytyPS5 is the only source that accepts it and states no origin for it, and SharpProspero does not list it, so the console's reading of 0x10 is unverified. Format 0 returns `0x80260106`, as in KytyPS5, which [moved](https://github.com/KytyPS5/KytyPS5/commit/3cf426662e59135393182006a76393676be5200e) 16-bit mono from 0 to 1; [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/audio/audioin.h#L21) takes 0 as 16-bit mono but accepts only 0 and 2, the PS4 formats. `sceAudioInAsyncOpen` shares these formats; `sceAudioInHqOpen` accepts only format 2. No title seen passes 0 or 0x10: PPSA21567 calls `sceAudioInOpen` once, at 0xff2bd6, with type 0, index 0, 256 samples, 16000 Hz and format 1, and on the other branch of the same function `sceAudioInHqOpen` at 0xff2b70 with type 1, index 0, 128 samples, 48000 Hz and format 2; PPSA10112 calls `sceAudioInOpen` once, at 0x48eab92, with type 0, index 0, 256 samples, 16000 Hz and format 1. Both pass the user id first and keep the result as the handle for `sceAudioInInput`; PPSA10112 takes a negative result as failure
- [sceAudioInIsPadEmbeddedMic](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - name from its NID (`X+4jdIS75P0`); signature `int32_t(int32_t handle)` from the PPSA26344 call site in #2355. PPSA21567 compares the result with 1 at both of its call sites and takes 1 to mean that the port captures from the pad's microphone. A port reports 1 when it captures from the host's default recording device and that device's name contains "DualSense", the match AudioOut2 uses to find the pad's speaker, and 0 otherwise, also without a device; unknown handles return `0x80260101` like the other AudioIn calls. The console's code for an unknown handle and whether the result follows a change of device after the port opened are unknown. The PS5 module uses the low handle byte as a port index and returns whether its resolved device type equals 8, propagating negative device errors; host capture devices have no such classification, so the device name stands in for it
- [sceAudioInDeviceIdOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInDeviceIdAsyncOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetUsbGain](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInIsSharedDevice](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInDeviceIdHqOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInDeviceHqOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetConnections](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetPortConnections](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetPortStatuses](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInGetHandleStatusInfo](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInDeviceOpenEx](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInChangeAppModuleState](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetSparkParam](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetSparkSideTone](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetCompressorPreGain](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature; the PS5 12.00 module returns 0 without reading any argument, and so does this one
- [sceAudioInSetNoiseGateThreshold](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature; the PS5 12.00 module returns 0 without reading any argument, and so does this one
- [sceAudioInSetMicLevel](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetDeviceMuteStatus](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetDevConnection](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInCountPorts](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetUserMute](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicCreate](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicDestroy](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicWrite](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicGetStatus](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInVmicSetMute](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetMode](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInSetMode2](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInExtCtrl](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature
- [sceAudioInInputs](../../../core/libs/prx/libSceAudioIn/Export.cpp) (libSceAudioIn) - unknown signature

### Functional

- [sceAudioInHqOpen, sceAudioInAsyncOpen](../../../core/libs/prx/libSceAudioIn/Export.cpp) - PS5 wrapper validation requires 48000 Hz followed by exactly 128 samples for both calls; HqOpen then requires format 2. The host uses the same SDL capture path for both, so native asynchronous scheduling and high-quality processing are not reproduced. Common port validation, device selection and host capture pacing remain incomplete.
- [sceAudioInGetGain, sceAudioInGetRerouteCount](../../../core/libs/prx/libSceAudioIn/Export.cpp) - GetGain writes 1.0f before handle validation and returns 0x80260105 for a null output, as confirmed by the PS5 module. Native handle bit checks and uninitialized/inactive port errors differ from the host port table. Native reroute counters follow device reassignment; the host opens each SDL device once and reports zero.
