# libSceErrorDialog technical debt

### Silent stubs

- [libSceErrorDialog](../../../core/libs/prx/libSceErrorDialog/Export.cpp) - the error dialog runs the state machine but shows nothing; the error code passed to `sceErrorDialogOpen` is only logged

### Unknown function info

- [sceErrorDialogParamInitialize](../../../core/libs/prx/libSceErrorDialog/Export.cpp) (libSceErrorDialog) - zeroes the 16-byte parameter and stores its size in the first word, the layout `sceErrorDialogOpen` checks, as the PS4 SDK's inline initializer does; that PS5 exports it under this name comes from the firmware export database (upstream PR #6361), and its return value is assumed
