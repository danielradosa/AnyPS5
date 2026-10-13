# libReactNative.Modules.Vsh.WebAppLauncher technical debt

### Silent stubs

- [libReactNative.Modules.Vsh.WebAppLauncher](../../../core/libs/prx/libReactNative.Modules.Vsh.WebAppLauncher/Export.cpp) - `ErrorDialogOpen` and `ErrorDialogClose` reuse the 16-byte parameter layout, state model and error codes from `libSceErrorDialog`, but the VSH ABI and behaviour are unverified and no UI is shown. `ShareGetCurrentStatus` uses the validation and zeroed 16-byte status model from `libSceShare`; `ShareInitialize` and `ShareTerminate` succeed without implementing capture or sharing. Those VSH-specific behaviours are unverified. `SystemServiceParamGetInt` follows `libSceSystemService` defaults, including `ANYPS5_LANGUAGE`; `SystemServiceParamGetString` only supports the fixed system name `PS5`, while other parameters and buffers of 1 to 64 bytes remain throwing paths. Runtime behaviour in a PS5 title is unverified.
