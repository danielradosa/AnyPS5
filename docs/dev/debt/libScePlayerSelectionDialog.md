# libScePlayerSelectionDialog technical debt

### Silent stubs

- [libScePlayerSelectionDialog](../../../core/libs/prx/libScePlayerSelectionDialog/Export.cpp) shows no selection UI and immediately reports cancellation. Account discovery, selection and successful-result list delivery are not implemented.
- [libScePlayerSelectionDialog](../../../core/libs/prx/libScePlayerSelectionDialog/Export.cpp) - EA SPORTS FC 27 (PPSA34015) fails to load `libScePlayerSelectionDialog.prx` without `Initialize`, `Open`, `Close`, `GetStatus`, `UpdateStatus` and `GetResult` (`unresolved ELF import` / `GetLastError: 127`)

### Unknown function info

- [scePlayerSelectionDialogParamInitialize](../../../core/libs/prx/libScePlayerSelectionDialog/Export.cpp) (libScePlayerSelectionDialog) - fills the 216-byte `ScePlayerSelectionDialogParam` as the `InitializeParam` helper of [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/main/src/SharpProspero/Interop/Dialog/PlayerSelectionDialog.cs) does: everything zero, the common-dialog base parameter's size 48 (its `size`, 36 reserved bytes and `magic` layout from [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/main/src/core/libraries/system/commondialog.h)) and magic `0xC0D1A109` plus the base parameter's address, the structure size 216, the invalid user id -1 and 256 selectable players. That the PS5 export does what that helper does, its `int` return and the `ARG_NULL` (0x80B8000D) answer for a null parameter (upstream PR #6696) are not verified
