# libSceAudiodec technical debt

### Unknown function info

- [sceAudiodecDecode](../../../core/libs/prx/libSceAudiodec/src/Codecs.cpp) (libSceAudiodec) - a raw M4AAC stream carries no channel configuration, so the decoder tries the mono/stereo configuration and then configurations 3 to 7 within `ui_max_channels` until one decodes the first access unit, and keeps that one. No reference documents how the library picks the layout (shadPS4 maps the channel count straight to the configuration; SharpProspero only describes the maximum). With the stereo configuration a mono raw stream decodes as 2 channels, so 2 is reported, not 1
- [sceAudiodecDecode](../../../core/libs/prx/libSceAudiodec/src/Codecs.cpp) (libSceAudiodec) - the HE-AAC handling follows one console measurement of an HE-AAC v2 ADTS stream: SBR output at 48 kHz in one channel with `ui_enable_heaac` 1, the 24 kHz mono core with 0. Not measured: whether the console, like FFmpeg, decodes a stream without SBR data in its first access unit as AAC-LC with `ui_enable_heaac` 1, and what it reports in `ui_sampling_freq` and `ui_heaac` with `ui_enable_heaac` 0 (here the rate of the returned PCM and 0)
- [libSceAudiodec](../../../core/libs/prx/libSceAudiodec/Export.cpp) answers the 24-bit PCM word size (`iBwPcm` 0) with `INVALID_WORD_LENGTH` (0x807F0010), as for any other unsupported word size: [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Interop/Audio/Audiodec.cs) lists only 16-bit and float output and notes that a third word size was offered but never existed. [KytyPS5](https://github.com/KytyPS5/KytyPS5) instead accepts it as 4-byte samples (32-bit for M4AAC); the console answer is unverified.

### Functional

- [sceAudiodecDecode](../../../core/libs/prx/libSceAudiodec/src/Codecs.cpp) (libSceAudiodec) - an ADTS stream with channel configuration 0 (layout in a program config element) is still parsed by FFmpeg, so `ui_enable_heaac` 0 does not remove its SBR and a mono one with Parametric Stereo comes out as stereo.
- [libSceAudiodec](../../../core/libs/prx/libSceAudiodec/Export.cpp) ATRAC9 decoding is covered only by configuration and error tests, as no ATRAC9 encoder is available for a fixture.
