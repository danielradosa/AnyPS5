# libScePlayerSelectionDialog technical debt

### Silent stubs

- [libScePlayerSelectionDialog](../../../core/libs/prx/libScePlayerSelectionDialog/Export.cpp) - the player selection dialog runs the common-dialog state machine but shows nothing, finishes immediately and reports the user as having cancelled with an empty player list; the result layout follows [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/main/src/SharpProspero/Interop/Dialog/PlayerSelectionDialog.cs)
