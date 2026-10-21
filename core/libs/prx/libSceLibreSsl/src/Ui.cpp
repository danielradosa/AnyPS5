#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libc/include/General.hpp"

#include <new>
#include <stdexcept>
#include <string>

namespace LibreSsl {

struct UiMethod {
    std::string name;
    void* opener = nullptr;
    void* writer = nullptr;
    void* flusher = nullptr;
    void* reader = nullptr;
    void* closer = nullptr;
};

}

using namespace LibreSsl;

extern "C" {

UiMethod* APS5_VABI UI_create_method_nid_postfix(char* name) {
    if (!name) throw std::invalid_argument("UI_create_method: null name");
    return new (std::nothrow) UiMethod{name};
}

void APS5_VABI UI_destroy_method_nid_postfix(UiMethod* ui_method) {
    delete ui_method;
}

int APS5_VABI UI_method_set_opener_nid_postfix(UiMethod* method, void* opener) {
    if (!method) return -1;
    method->opener = opener;
    return 0;
}

int APS5_VABI UI_method_set_writer_nid_postfix(UiMethod* method, void* writer) {
    if (!method) return -1;
    method->writer = writer;
    return 0;
}

int APS5_VABI UI_method_set_reader_nid_postfix(UiMethod* method, void* reader) {
    if (!method) return -1;
    method->reader = reader;
    return 0;
}

int APS5_VABI UI_method_set_closer_nid_postfix(UiMethod* method, void* closer) {
    if (!method) return -1;
    method->closer = closer;
    return 0;
}

void* APS5_VABI UI_method_get_opener_nid_postfix(UiMethod* method) {
    return method ? method->opener : nullptr;
}

void* APS5_VABI UI_method_get_writer_nid_postfix(UiMethod* method) {
    return method ? method->writer : nullptr;
}

void* APS5_VABI UI_method_get_reader_nid_postfix(UiMethod* method) {
    return method ? method->reader : nullptr;
}

void* APS5_VABI UI_method_get_closer_nid_postfix(UiMethod* method) {
    return method ? method->closer : nullptr;
}

}
