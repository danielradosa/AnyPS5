# libSceCompanionUtil technical debt

### Unknown function info

- [sceCompanionUtilGetEvent, sceCompanionUtilGetRemoteOskEvent, sceCompanionUtilInitialize, sceCompanionUtilOptParamInitialize and sceCompanionUtilTerminate](../../../core/libs/prx/libSceCompanionUtil/Export.cpp) (libSceCompanionUtil) - names and NIDs are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceCompanionUtil.c). Each returns the firmware module's single constant: `NO_EVENT` (0x80AD0008) for the event call, 0 for the others. [orbital](https://github.com/AlexAltea/orbital/blob/cc408367da6a6ebbf5ac1ac559fefeadc73b2831/tools/ida/db_types.json) gives PS4 prototypes with parameters; as no argument is read, they are declared without parameters, which is not an ABI claim
