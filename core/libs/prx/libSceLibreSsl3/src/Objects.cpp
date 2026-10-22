#include "prx/libSceLibreSsl/include/LibreSsl.hpp"
#include "prx/libc/include/General.hpp"

using namespace LibreSsl;

extern "C" {

int APS5_VABI OBJ_cmp_nid_postfix(const Asn1Object* a, const Asn1Object* b) {
    return LibreSslObjCmp_nid_no_patch(a, b);
}

Asn1Object* APS5_VABI OBJ_txt2obj_nid_postfix(const char* s, int no_name) {
    return LibreSslObjTxt2Obj_nid_no_patch(s, no_name);
}

}
