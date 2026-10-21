# libSceHmdSetupDialog technical debt

### Functional

- [HMD setup dialog](../../../core/libs/prx/libSceHmdSetupDialog/Export.cpp) (libSceHmdSetupDialog) shows nothing, as no headset is ever connected: it runs the common-dialog state machine, `sceHmdSetupDialogOpen` finishes at once and `sceHmdSetupDialogGetResult` reports that the user canceled, the answer [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/hmd/hmd_setup_dialog.cpp) gives without a headset (the dialog asks for one and the user backs out). Prototypes from [orbital](https://github.com/AlexAltea/orbital/blob/cc408367da6a6ebbf5ac1ac559fefeadc73b2831/tools/ida/db_types.json) and shadPS4's hmd_setup_dialog.h. Only the 4-byte `result` field of the 36-byte result is written, the parameter is only checked for null, and the 0x80B8xxxx codes and their order follow the other common dialogs; not measured on hardware
