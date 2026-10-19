# libSceScreenShot technical debt

### Silent stubs

- [sceScreenShotSetOverlayImage, sceScreenShotSetOverlayImageWithOrigin](../../../core/libs/prx/libSceScreenShot/Export.cpp) (libSceScreenShot) - prototypes `(const char* filePath, int32_t offsetX, int32_t offsetY)` and `(filePath, marginX, marginY, origin)` from [orbital](https://github.com/AlexAltea/orbital/blob/cc408367da6a6ebbf5ac1ac559fefeadc73b2831/tools/ida/db_types.json); the [shadPS4](https://github.com/shadps4-emu/shadPS4/blob/23c41333822fe8eca4c609b43119180af63ae4ac/src/core/libraries/screenshot/screenshot.cpp) PS4 functions of the same names return 0. The image is laid over system screenshots, which are never taken, so both return 0 without opening the file, like [sceShareSetScreenshotOverlayImage](../../../core/libs/prx/libSceShare/Export.cpp). The origin values and the errors for a missing file or a null path are unknown
