# libReactNative.Modules.Vsh technical debt

### Unknown function info

- [RemotePlayGetConnectionStatus](../../../core/libs/prx/libReactNative.Modules.Vsh/Export.cpp) (libReactNative.Modules.Vsh) - assumed to wrap [sceRemoteplayGetConnectionStatus](../../../core/libs/prx/libSceRemoteplay/Export.cpp), whose `(userId, status)` signature it is declared with; no source documents the wrapper. Like it, it reports every user as disconnected (0), as the host has no Remote Play session, and a null `status` throws
