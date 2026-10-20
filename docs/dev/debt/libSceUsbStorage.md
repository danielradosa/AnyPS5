# libSceUsbStorage technical debt

### Silent stubs

- [libSceUsbStorage](../../../core/libs/prx/libSceUsbStorage/Export.cpp) - USB storage device management tracks subsystem initialization state via atomic lifecycle guards (`sceUsbStorageInit` and `sceUsbStorageTerm`). When no physical or virtual USB storage device is mounted, device enumeration (`sceUsbStorageGetDeviceList`) reports 0 connected devices, device queries (`sceUsbStorageGetDeviceInfo`, `sceUsbStorageGetdentsOpen`) return `SCE_USB_STORAGE_ERROR_NOENT`, and path mapping (`sceUsbStorageRequestMap`, `sceUsbStorageRequestMapWSB`) formats default mount destinations (`/mnt/usb0`). Callback registration stubs validate argument and event bounds without dispatching asynchronous hardware notifications.

### Unknown function info

- [sceUsbStorageClearFakeWriteProtection, sceUsbStorageSetFakeMapLockForDebug, sceUsbStorageSetFakeWriteProtection](../../../core/libs/prx/libSceUsbStorage/Export.cpp) (libSceUsbStorage) - names and NIDs are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceUsbStorage.c), but no credible prototypes were found. The device id and flag parameters are assumed, and with no device to protect or lock all three return 0 without reading them.
- [sceUsbStorageRequestMapWSB](../../../core/libs/prx/libSceUsbStorage/Export.cpp) (libSceUsbStorage) - its prototype follows [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/f68923fe807f657348e6719ce658bcbd0a2b1c72/src/core/libraries/usbd/usbstorage.cpp#L245-L246), not measured on a console; it maps like `sceUsbStorageRequestMap`.
