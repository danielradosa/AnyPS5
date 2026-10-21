#include "prx/libSceLibreSsl/include/LibreSsl.hpp"
#include "prx/libc/include/General.hpp"

using namespace LibreSsl;

extern "C" {

int APS5_VABI X509_STORE_up_ref_nid_postfix(Store* store) {
    return LibreSslX509StoreUpRef_nid_no_patch(store);
}

StoreCtx* APS5_VABI X509_STORE_CTX_new_nid_postfix() {
    return LibreSslX509StoreCtxNew_nid_no_patch();
}

void APS5_VABI X509_STORE_CTX_free_nid_postfix(StoreCtx* ctx) {
    LibreSslX509StoreCtxFree_nid_no_patch(ctx);
}

int APS5_VABI X509_STORE_CTX_init_nid_postfix(StoreCtx* ctx, Store* store, X509Object* x509, Stack* chain) {
    return LibreSslX509StoreCtxInit_nid_no_patch(ctx, store, x509, chain);
}

}
