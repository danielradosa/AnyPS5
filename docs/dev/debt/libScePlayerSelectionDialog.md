# libScePlayerSelectionDialog technical debt

### Silent stubs

- [libScePlayerSelectionDialog](../../../core/libs/prx/libScePlayerSelectionDialog/Export.cpp) - the player selection dialog runs the common-dialog state machine but shows nothing, finishes immediately and reports the user as having cancelled with an empty player list; the result layout follows [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/main/src/SharpProspero/Interop/Dialog/PlayerSelectionDialog.cs)
- [libScePlayerSelectionDialog](../../../core/libs/prx/libScePlayerSelectionDialog/Export.cpp) - the player-selection dialog runs the common-dialog state machine but shows nothing and reports the user as having cancelled. `GetResult` writes only the first `int32` (`USER_CANCELED`); the rest of `ScePlayerSelectionDialogResult` (player list pointer and length) is not filled. EA SPORTS FC 27 (PPSA34015) fails to load `libScePlayerSelectionDialog.prx` without `Initialize`, `Open`, `Close`, `GetStatus`, `UpdateStatus` and `GetResult` (`unresolved ELF import` / `GetLastError: 127`)
