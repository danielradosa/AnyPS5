# libSceVrServiceDialog technical debt

### Functional

- [VR service dialog](../../../core/libs/prx/libSceVrServiceDialog/Export.cpp) (libSceVrServiceDialog) shows nothing, as no headset is ever connected: it runs the common-dialog state machine, `sceVrServiceDialogOpen` finishes at once and `sceVrServiceDialogGetResult` reports that the user canceled. Nothing documents this dialog's own answer; canceled is what [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/hmd/hmd_setup_dialog.cpp) answers for the sibling headset setup dialog without a headset. Prototypes from [orbital](https://github.com/AlexAltea/orbital/blob/cc408367da6a6ebbf5ac1ac559fefeadc73b2831/tools/ida/db_types.json). The result layout is unknown, so only its first 4-byte `result` field is written; the parameter is only checked for null, and the 0x80B8xxxx codes and their order follow the other common dialogs; not measured on hardware
