# libScePs2EmuMenuDialog technical debt

### Silent stubs

- [libScePs2EmuMenuDialog](../../../core/libs/prx/libScePs2EmuMenuDialog/Export.cpp) - the PS2 emulator menu dialog runs the common-dialog state machine but shows nothing and reports the user as having cancelled, as there is no host PS2 emulation UI. The result layout beyond the leading `int32_t` result code is not known, so nothing else is written.
