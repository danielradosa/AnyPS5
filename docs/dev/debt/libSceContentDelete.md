# libSceContentDelete technical debt

### Functional

- [sceContentDeleteById](../../../core/libs/prx/libSceContentDelete/Export.cpp) and `sceContentDeleteByPath`: content database access is not implemented. The host content library is the `/_ce` directory that [libSceContentExport](../../../core/libs/prx/libSceContentExport/Export.cpp) copies exported files into and whose guest paths it returns: deleting by such a path removes the file and the `<name>.thumbnail.<extension>` thumbnails exported with it. No content ids are handed out (libSceContentSearch reports an empty library), so after initialization every id, a path outside `/_ce`, a path below a subdirectory or with `..`, a thumbnail and a missing file throw; the backend result for content that is not in the library is unknown. Null paths after initialization throw instead of reproducing the original unchecked string access.
- [sceContentDeleteInitialize](../../../core/libs/prx/libSceContentDelete/Export.cpp): validates the confirmed 16 KiB heap requirement and lifecycle errors, but does not create the original allocator, mutex and database client. Initialization-parameter fields other than the 64-bit heap size at offset 8 remain unresolved.

### Unknown function info

- [sceContentDeleteByPath, sceContentDeleteById](../../../core/libs/prx/libSceContentDelete/Export.cpp) (libSceContentDelete) - the `(const char* contentPath)` and `(int64_t contentId)` signatures follow [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Interop/Content/ContentDelete.cs). PPSA06465 imports `sceContentDeleteById`. SharpProspero's only error, `0x809D5006` (a permission the process does not hold), is never returned
- [sceContentDeleteGlobalMstat](../../../core/libs/prx/libSceContentDelete/Export.cpp), `sceContentDeleteMemoryInitialize` and `sceContentDeleteMemoryTerminate`: neither exports nor callers were found in the supplied decompiled library corpus; their signatures remain unresolved.
