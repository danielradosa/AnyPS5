# libSceVrSetupDialog technical debt

### Silent stubs

- [libSceVrSetupDialog](../../../core/libs/prx/libSceVrSetupDialog/Export.cpp) - the VR setup dialog runs the common-dialog state machine but shows nothing and reports the user as having cancelled, as no headset is supported (see libSceHmd2). Initialization does not check the native process eligibility record, and the 46-byte setup payload remains opaque. Open validates the recovered 56-byte parameter header. GetResult writes one 32-bit result after initialization, including when returning the not-finished error; its error codes and validation order are recovered. PPSA12544 imports the library. `sceVrSetupDialogGetStatus` (NID `qRUACYyUNFA`) is present in the firmware export database (aerolib), but direct import by PPSA12544 has not been explicitly confirmed.

### Functional

- [VR setup dialog](../../../core/libs/prx/libSceVrSetupDialog/Export.cpp) (libSceVrSetupDialog) shows nothing: `sceVrSetupDialogOpen` finishes at once and the result reports that the user canceled. The host has no headset, and on a PS4 the headset setup dialog finishes with that result when the user cancels it, or with 0 at once when the headset is ready ([ALVR-PS4](https://github.com/nicolas-riera/ALVR-PS4/blob/3889e303b9365422397c6570f0fde27aabf4b25f/docs/psvr-technical-reference.md))
