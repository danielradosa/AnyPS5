# libSceNpSnsFacebookDialog technical debt

### Unknown function info

- [sceNpSnsFacebookDialogClose, sceNpSnsFacebookDialogGetStatus](../../../core/libs/prx/libSceNpSnsFacebookDialog/Export.cpp) (libSceNpSnsFacebookDialog) - the `int32_t (void)` and `SceCommonDialogStatus (void)` prototypes come from [orbital](https://github.com/AlexAltea/orbital/blob/b321fcdbd700cc02da1cb1ad25412781d5ec73c6/tools/ida/db_types.json). The module declares no initialize or open call (the PS5 export table has only these two), so the dialog can never leave the common-dialog `NONE` state: `GetStatus` returns `NONE` (0), as [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/np/np_sns_facebook_dialog.cpp) answers its PS4 `UpdateStatus`, and `Close` returns `SCE_COMMON_DIALOG_ERROR_NOT_INITIALIZED` (`0x80B80003`), the answer shadPS4's profile dialog and the other common dialogs here give before `Initialize`. Not measured on a console.
