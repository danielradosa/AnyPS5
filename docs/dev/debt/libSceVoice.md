# libSceVoice technical debt

### Unknown function info

- [sceVoiceResetPort](../../../core/libs/prx/libSceVoice/Export.cpp) (libSceVoice) - assumed to take the port id like the other port calls; the port is checked and nothing else is reset, since no audio flows through the ports
- [sceVoiceWriteToIPort](../../../core/libs/prx/libSceVoice/Export.cpp) (libSceVoice) - data written to an input port that is not connected is consumed and `size` is left as passed; writing to a connected input port throws, since data is not routed to output ports. The frame gap count is ignored. Console behavior is unverified
- [sceVoiceGetResourceInfo](../../../core/libs/prx/libSceVoice/Export.cpp) (libSceVoice) - the (info) signature and the 8-byte layout of four 16-bit port maxima (IN_VOICE, OUT_VOICE, IN_DEVICE, OUT_DEVICE) follow [fpPS4](https://github.com/red-prig/fpPS4/blob/main/src/ps4_libscevoice.pas) (PS4), whose values 2, 4, 0, 5 are the port type numbers rather than limits. The reported 7, 7, 1, 1 are the PS Vita FW 3.60 limits from [vita-headers](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/voice.h), whose libSceVoice uses the same 0x804E08xx error codes; the PS5 limits are unknown and sceVoiceCreatePort does not enforce them. The Vita structure has a fifth field (64 ports in total), which is not written. Like the other calls, it throws before sceVoiceInit
