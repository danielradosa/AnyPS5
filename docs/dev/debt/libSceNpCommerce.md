# libSceNpCommerce technical debt

### Silent stubs

- [libSceNpCommerce](../../../core/libs/prx/libSceNpCommerce/Export.cpp) - the PS Store icon show/hide calls and `sceNpCommerceSetPsStoreIconLayout` do nothing

### Unknown function info

- [sceNpCommerceDialogInitializeInternal](../../../core/libs/prx/libSceNpCommerce/Export.cpp) (libSceNpCommerce) - declared without parameters and forwarded to `sceNpCommerceDialogInitialize`, sharing its state and errors; the name comes from the firmware export database (upstream PR #6361), and whether the internal variant takes arguments or differs on the console is unknown
- [sceNpCommerceDialogOpen2](../../../core/libs/prx/libSceNpCommerce/Export.cpp): extended content combinations outside content mode 0 and content mode 1 with dialog mode 1 throw when otherwise valid. The native restriction reads process sysctl field 68; its relation to host process metadata is unresolved. Mode-specific meanings of the option and 64-bit argument remain unresolved.

- [sceNpCommerceDialogOpen](../../../core/libs/prx/libSceNpCommerce/Export.cpp): Product copying and service submission remain part of the cancelled UI stub.
