# libSceFontFt technical debt

### Functional

- [Font format support](../../../core/libs/prx/libSceFontFt/Export.cpp): format queries now take the font library, validate its magic and lock it while checking its actual FreeType modules. The host registers its default modules at creation rather than enabling formats individually, so format-selection state and guest allocator failures during format registration are not reproduced. Unsupported foreign drivers throw.

### Silent stubs

- [sceFontFtSupportFontFormats, sceFontFtSupportSystemFonts, sceFontFtSupportTrueTypeGx](../../../core/libs/prx/libSceFontFt/Export.cpp) (libSceFontFt) - validate and lock the library like the other format queries, then return success without requiring or loading a FreeType module; TrueType/OpenType already work through the existing driver and the `formats` flags of `sceFontFtSupportFontFormats` are ignored, as `LibrarySupport` does

### Unknown function info

- [Alias and glyph selection exports](../../../core/libs/prx/libSceFontFt/Export.cpp): alias initialization/termination take a handle pointer; alias registration takes a string and font ID or path. Glyph selection takes a 32-bit selector and returns a table pointer whose layout remains unresolved. Their protected implementations are unchanged.
