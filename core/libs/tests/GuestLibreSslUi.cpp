#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>
#include <exception>

struct GuestUiMethod;
struct GuestUi;
struct GuestUiString;

using Opener = int (APS5_VABI*)(GuestUi*);
using Writer = int (APS5_VABI*)(GuestUi*, GuestUiString*);

extern "C" {
GuestUiMethod* APS5_VABI UI_create_method_nid_postfix(char*);
void APS5_VABI UI_destroy_method_nid_postfix(GuestUiMethod*);
int APS5_VABI UI_method_set_opener_nid_postfix(GuestUiMethod*, Opener);
int APS5_VABI UI_method_set_writer_nid_postfix(GuestUiMethod*, Writer);
int APS5_VABI UI_method_set_reader_nid_postfix(GuestUiMethod*, Writer);
int APS5_VABI UI_method_set_closer_nid_postfix(GuestUiMethod*, Opener);
Opener APS5_VABI UI_method_get_opener_nid_postfix(GuestUiMethod*);
Writer APS5_VABI UI_method_get_writer_nid_postfix(GuestUiMethod*);
Writer APS5_VABI UI_method_get_reader_nid_postfix(GuestUiMethod*);
Opener APS5_VABI UI_method_get_closer_nid_postfix(GuestUiMethod*);
}

static void Require(bool value) {
    if (!value) std::abort();
}

template <typename TCall>
static bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static int APS5_VABI Open(GuestUi*) {
    return 1;
}

static int APS5_VABI Close(GuestUi*) {
    return 1;
}

static int APS5_VABI Write(GuestUi*, GuestUiString*) {
    return 1;
}

static int APS5_VABI Read(GuestUi*, GuestUiString*) {
    return 1;
}

int main() {
    char name[] = "test method";
    GuestUiMethod* method = UI_create_method_nid_postfix(name);
    Require(method != nullptr);
    Require(!UI_method_get_opener_nid_postfix(method) && !UI_method_get_writer_nid_postfix(method));
    Require(!UI_method_get_reader_nid_postfix(method) && !UI_method_get_closer_nid_postfix(method));
    Require(UI_method_set_opener_nid_postfix(method, Open) == 0 && UI_method_get_opener_nid_postfix(method) == Open);
    Require(UI_method_set_writer_nid_postfix(method, Write) == 0 && UI_method_get_writer_nid_postfix(method) == Write);
    Require(UI_method_set_reader_nid_postfix(method, Read) == 0 && UI_method_get_reader_nid_postfix(method) == Read);
    Require(UI_method_set_closer_nid_postfix(method, Close) == 0 && UI_method_get_closer_nid_postfix(method) == Close);
    Require(UI_method_set_opener_nid_postfix(method, nullptr) == 0 && UI_method_get_opener_nid_postfix(method) == nullptr);
    Require(UI_method_set_opener_nid_postfix(nullptr, Open) == -1 && UI_method_set_writer_nid_postfix(nullptr, Write) == -1);
    Require(UI_method_set_reader_nid_postfix(nullptr, Read) == -1 && UI_method_set_closer_nid_postfix(nullptr, Close) == -1);
    Require(UI_method_get_opener_nid_postfix(nullptr) == nullptr && UI_method_get_writer_nid_postfix(nullptr) == nullptr);
    Require(UI_method_get_reader_nid_postfix(nullptr) == nullptr && UI_method_get_closer_nid_postfix(nullptr) == nullptr);
    UI_destroy_method_nid_postfix(method);
    UI_destroy_method_nid_postfix(nullptr);
    Require(Throws([] { UI_create_method_nid_postfix(nullptr); }));
    return 0;
}
