# libSceFontFt technical debt

### Silent stubs

- [libSceFontFt](../../../core/libs/prx/libSceFontFt/Export.cpp) - `sceFontFtSupport*` return success and do not load extra FreeType modules; TrueType/OpenType already work through the existing driver, other format flags are ignored as `LibrarySupport` does
