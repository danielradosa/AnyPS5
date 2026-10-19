# libSceSysUtil technical debt

### Silent stubs

- [sceSysUtilSendSystemNotificationWithText](../../../core/libs/prx/libSceSysUtil/Export.cpp) (libSceSysUtil) - AnyPS5 has no system notification UI, so the text is written to the host's standard error as `[notification] <text>`, the line [sceKernelSendNotificationRequest](../../../core/libs/prx/libkernel/System/src/Notification.cpp) writes, and the call returns success

### Unknown function info

- [sceSysUtilSendSystemNotificationWithText](../../../core/libs/prx/libSceSysUtil/Export.cpp) (libSceSysUtil) - the `(int32_t type, const char* message)` signature follows [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Payload/Services/Notification.cs#L99-L115), which calls it with a notification type code and a NUL-terminated UTF-8 text and treats a negative result as an error. The type codes and the console's limits on the text are unknown, so the type is not read and any text is accepted; a null text throws, as the console's code for it is unknown
