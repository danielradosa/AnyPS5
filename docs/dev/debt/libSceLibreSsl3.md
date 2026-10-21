# libSceLibreSsl3 technical debt

### Unknown function info

- [libSceLibreSsl3 pending exports](../../../core/libs/prx/libSceLibreSsl3/Export.cpp) - 84 named callable exports are listed in the [PS5 3.20 export table](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceLibreSsl3.c) and were cross-checked against the same module in PS5 firmware 13.60. Their signatures and behaviour are unverified; the declarations use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.
- [MD5, SHA1 and CRYPTO_memcmp](../../../core/libs/prx/libSceLibreSsl3/src/Digest.cpp) (libSceLibreSsl3) - follow LibreSSL's [md5.c, sha1.c](https://github.com/libressl/openbsd/tree/bc9483725c5815efb64b6d9acc52fc5fba139212/src/lib/libcrypto) and `CRYPTO_memcmp`, which returns the OR of the differing bytes in 3.0 through current; `MD5` and `SHA1` hash through libSceLibreSsl's `MD5_*` and `SHA1_*`. A null digest buffer throws: LibreSSL up to 3.8 returns a static buffer, later versions do not check it. Nothing is checked on the console
