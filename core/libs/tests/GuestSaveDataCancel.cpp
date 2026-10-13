#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <system_error>
#include <vector>

extern "C" {
int APS5_VABI sceSaveDataInitialize3(const void*);
int APS5_VABI sceSaveDataTerminate(void);
int APS5_VABI sceSaveDataCreateTransactionResource(std::uint32_t);
int APS5_VABI sceSaveDataDeleteTransactionResource(std::int32_t);
int APS5_VABI sceSaveDataMount3(const SaveDataMount3*, SaveDataMountResult*);
int APS5_VABI sceSaveDataUmount2(std::uint32_t, const SaveDataMountPoint*);
int APS5_VABI sceSaveDataPrepare(const SaveDataMountPoint*, const SaveDataPrepareParam*);
int APS5_VABI sceSaveDataCommit(const SaveDataCommitParam*);
int APS5_VABI sceSaveDataCancel(std::int32_t);
int APS5_VABI sceSaveDataSetParam(const SaveDataMountPoint*, std::uint32_t, const void*, std::size_t);
int APS5_VABI sceSaveDataGetParam(const SaveDataMountPoint*, std::uint32_t, void*, std::size_t, std::size_t*);
}

namespace {

constexpr int SaveDataErrorParameter = static_cast<int>(0x809F0000u);
constexpr int SaveDataErrorNotInitialized = static_cast<int>(0x809F0001u);
constexpr int SaveDataErrorBusy = static_cast<int>(0x809F0003u);
constexpr int SaveDataErrorNotMounted = static_cast<int>(0x809F0004u);
constexpr int SaveDataErrorBadMounted = static_cast<int>(0x809F000Du);
constexpr int SaveDataErrorResourceFull = static_cast<int>(0x809F001Au);
constexpr int SaveDataErrorResourceBusy = static_cast<int>(0x809F001Bu);
constexpr int SaveDataErrorResourceInvalid = static_cast<int>(0x809F001Cu);
constexpr std::uint32_t MountReadOnly = 1;
constexpr std::uint32_t MountReadWrite = 2;
constexpr std::uint32_t MountCreate = 4;
constexpr std::uint32_t PrepareDefault = 0;
constexpr std::uint32_t PrepareEnableCancel = 2;
constexpr std::uint32_t PrepareIgnoreCancelOnHost = 4;
constexpr std::uint32_t ParamTypeTitle = 1;

int failures = 0;

void Check(bool condition, const std::string& what) {
    if (condition) return;
    std::fprintf(stderr, "savedata cancel check failed: %s\n", what.c_str());
    ++failures;
}

void Write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream(path, std::ios::binary) << text;
}

std::string Read(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

int Mount(const char* name, std::uint32_t mode, std::int32_t resource, SaveDataMountResult& result) {
    SceSaveDataDirName dirName{};
    std::memcpy(dirName.data, name, std::strlen(name) + 1);
    SaveDataMount3 mount{};
    mount.dir_name = &dirName;
    mount.mount_mode = mode;
    mount.resource = resource;
    result = {};
    return sceSaveDataMount3(&mount, &result);
}

int Prepare(const SaveDataMountPoint& point, std::int32_t resource, std::uint32_t mode) {
    SaveDataPrepareParam param{};
    param.resource = resource;
    param.prepare_mode = mode;
    return sceSaveDataPrepare(&point, &param);
}

int Commit(std::int32_t resource) {
    SaveDataCommitParam param{};
    param.resource = resource;
    return sceSaveDataCommit(&param);
}

std::string Title(const SaveDataMountPoint& point) {
    char title[128]{};
    std::size_t got = 0;
    if (sceSaveDataGetParam(&point, ParamTypeTitle, title, sizeof(title), &got) != 0) return "<error>";
    return title;
}

int SetTitle(const SaveDataMountPoint& point, const char* title) {
    return sceSaveDataSetParam(&point, ParamTypeTitle, title, std::strlen(title) + 1);
}

}

int main() {
    const auto root = std::filesystem::temp_directory_path() / ("anyps5-savedata-cancel-" + std::to_string(std::random_device{}()));
    std::filesystem::create_directories(root);
    const auto previous = std::filesystem::current_path();
    std::filesystem::current_path(root);
    const auto save = root / "_sd" / "slot";

    Check(sceSaveDataCancel(1) == SaveDataErrorNotInitialized, "cancel before initialization");
    Check(sceSaveDataCreateTransactionResource(0) == SaveDataErrorNotInitialized, "resource creation before initialization");
    Check(sceSaveDataInitialize3(nullptr) == 0, "SaveData initializes");
    Check(sceSaveDataCancel(12345) == SaveDataErrorResourceInvalid, "cancel of an unknown resource");
    Check(sceSaveDataDeleteTransactionResource(12345) == SaveDataErrorResourceInvalid, "deletion of an unknown resource");

    const int resource = sceSaveDataCreateTransactionResource(0);
    Check(resource > 0, "a transaction resource is created");
    SaveDataMountResult mounted{};
    Check(Mount("slot", MountCreate | MountReadWrite, resource, mounted) == 0, "the save is created");
    const SaveDataMountPoint point = mounted.mount_point;
    Write(save / "data.bin", "before");
    Check(SetTitle(point, "Before") == 0, "the title is set before the update");

    Check(sceSaveDataPrepare(nullptr, nullptr) == SaveDataErrorParameter, "prepare without arguments");
    Check(Prepare(point, resource, PrepareIgnoreCancelOnHost) == SaveDataErrorParameter, "ignoring cancel on host needs the cancel mode");
    Check(Prepare(point, resource, 1) == SaveDataErrorParameter, "an unknown prepare mode");
    Check(Prepare(point, 12345, PrepareDefault) == SaveDataErrorResourceInvalid, "prepare with an unknown resource");
    SaveDataMountPoint unmounted{};
    std::memcpy(unmounted.data, "/savedata9", sizeof("/savedata9"));
    Check(Prepare(unmounted, resource, PrepareDefault) == SaveDataErrorNotMounted, "prepare of an unmounted point");

    Check(Prepare(point, resource, PrepareDefault) == 0, "an update starts without cancel");
    Check(sceSaveDataCancel(resource) == SaveDataErrorResourceInvalid, "an update started without the cancel mode is not cancelled");
    Check(Commit(resource) == 0, "the update without cancel commits");
    Check(Commit(resource) == SaveDataErrorResourceInvalid, "a second commit has no update");
    Check(sceSaveDataCommit(nullptr) == SaveDataErrorParameter, "commit without a parameter");

    Check(Prepare(point, resource, PrepareEnableCancel) == 0, "a cancellable update starts");
    Check(Prepare(point, resource, PrepareEnableCancel) == SaveDataErrorBusy, "the save already has an update");
    Check(sceSaveDataDeleteTransactionResource(resource) == SaveDataErrorResourceBusy, "a resource in an update is not deleted");
    Write(save / "data.bin", "after");
    Write(save / "extra.bin", "new");
    Check(SetTitle(point, "After") == 0, "the title changes during the update");
    Check(Title(point) == "After", "the changed title is read back");
    Check(sceSaveDataCancel(resource) == 0, "the update is cancelled");
    Check(Read(save / "data.bin") == "before", "the changed file is restored");
    Check(!std::filesystem::exists(save / "extra.bin"), "the added file is removed");
    Check(Title(point) == "Before", "the title is restored");
    Check(sceSaveDataCancel(resource) == SaveDataErrorResourceInvalid, "a second cancel has no update");
    Check(Commit(resource) == SaveDataErrorResourceInvalid, "a cancelled update does not commit");

    Check(Prepare(point, resource, PrepareEnableCancel | PrepareIgnoreCancelOnHost) == 0, "a cancellable update starts again");
    Write(save / "data.bin", "kept");
    Check(Commit(resource) == 0, "the cancellable update commits");
    Check(sceSaveDataCancel(resource) == SaveDataErrorResourceInvalid, "a committed update is not cancelled");
    Check(Read(save / "data.bin") == "kept", "committed data stays");

    Check(sceSaveDataUmount2(0, &point) == 0, "the save unmounts");
    Check(Mount("slot", MountReadOnly, resource, mounted) == 0, "the save mounts read-only");
    Check(Prepare(mounted.mount_point, resource, PrepareDefault) == SaveDataErrorBadMounted, "a read-only mount cannot be updated");
    Check(sceSaveDataUmount2(0, &mounted.mount_point) == 0, "the read-only save unmounts");

    Check(sceSaveDataDeleteTransactionResource(resource) == 0, "the resource is deleted");
    Check(sceSaveDataDeleteTransactionResource(resource) == SaveDataErrorResourceInvalid, "a deleted resource is invalid");
    std::vector<int> resources;
    for (int i = 0; i < 16; ++i) resources.push_back(sceSaveDataCreateTransactionResource(0));
    bool allCreated = true;
    for (int created : resources) allCreated = allCreated && created > 0;
    Check(allCreated, "sixteen resources are created");
    Check(sceSaveDataCreateTransactionResource(0) == SaveDataErrorResourceFull, "a seventeenth resource is refused");

    Check(sceSaveDataTerminate() == 0, "SaveData terminates");
    Check(sceSaveDataCancel(resources.front()) == SaveDataErrorNotInitialized, "cancel after termination");
    Check(sceSaveDataInitialize3(nullptr) == 0, "SaveData initializes again");
    Check(sceSaveDataDeleteTransactionResource(resources.front()) == SaveDataErrorResourceInvalid, "termination deletes the resources");
    Check(sceSaveDataTerminate() == 0, "SaveData terminates again");

    std::filesystem::current_path(previous);
    std::error_code error;
    std::filesystem::remove_all(root, error);
    if (failures != 0) return 1;
    std::printf("savedata cancel tests passed\n");
    return 0;
}
