#include "Ngs2Test.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

template <typename TException, typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const TException&) {
        return true;
    }
    return false;
}

std::vector<Ngs2ReportCpuLoadData> reports;
std::vector<std::uintptr_t> reportUserData;

void APS5_VABI OnCpuLoad(const Ngs2ReportDataHeader* header, std::uintptr_t userData) {
    Require(header != nullptr && header->size == sizeof(Ngs2ReportCpuLoadData));
    reports.push_back(*reinterpret_cast<const Ngs2ReportCpuLoadData*>(header));
    reportUserData.push_back(userData);
}

Ngs2Handle selfRemovingHandle = 0;

void APS5_VABI OnCpuLoadOnce(const Ngs2ReportDataHeader* header, std::uintptr_t userData) {
    OnCpuLoad(header, userData);
    Require(sceNgs2ReportUnregisterHandler(selfRemovingHandle) == SCE_NGS2_OK);
}

void Render(uintptr_t system) {
    std::vector<std::int16_t> out(Grain * 2, -1);
    const Ngs2RenderBufferInfo info{out.data(), out.size() * sizeof(std::int16_t), SCE_NGS2_WAVEFORM_TYPE_PCM_I16L, 2};
    Require(sceNgs2SystemRender(system, &info, 1) == SCE_NGS2_OK);
}

void RequireCpuLoad(const Ngs2ReportCpuLoadData& report, uintptr_t system) {
    Require(report.header.size == sizeof(Ngs2ReportCpuLoadData) && report.header.handle == system && report.header.type == SCE_NGS2_REPORT_TYPE_CPU_LOAD && report.header.result == SCE_NGS2_OK);
    Require(std::isfinite(report.total_ratio) && report.total_ratio >= 0.0f && report.process_ratio == report.total_ratio);
    Require(report.flush_ratio == 0.0f && report.feedback_ratio == 0.0f);
}

}

int main() {
    Ngs2Handle handle = 0x1234;
    Require(Throws<std::invalid_argument>([&] { sceNgs2ReportRegisterHandler(SCE_NGS2_REPORT_TYPE_CPU_LOAD, nullptr, 0, &handle); }));
    Require(Throws<std::invalid_argument>([&] { sceNgs2ReportRegisterHandler(SCE_NGS2_REPORT_TYPE_COUNT, OnCpuLoad, 0, &handle); }));
    Require(Throws<std::runtime_error>([&] { sceNgs2ReportRegisterHandler(0, OnCpuLoad, 0, &handle); }));
    Require(handle == 0x1234);
    Require(sceNgs2ReportRegisterHandler(SCE_NGS2_REPORT_TYPE_CPU_LOAD, OnCpuLoad, 0x77, nullptr) == SCE_NGS2_ERROR_INVALID_OUT_ADDRESS);
    Require(Throws<std::invalid_argument>([] { sceNgs2ReportUnregisterHandler(0x1234); }));

    const auto system = CreateSystem();
    Render(system);
    Require(reports.empty());

    Require(sceNgs2ReportRegisterHandler(SCE_NGS2_REPORT_TYPE_CPU_LOAD, OnCpuLoad, 0x77, &handle) == SCE_NGS2_OK && handle != 0x1234 && handle != 0);
    Ngs2Handle second = 0;
    Require(sceNgs2ReportRegisterHandler(SCE_NGS2_REPORT_TYPE_CPU_LOAD, OnCpuLoad, 0x88, &second) == SCE_NGS2_OK && second != 0 && second != handle);
    Render(system);
    Require(reports.size() == 2 && reportUserData.size() == 2 && reportUserData[0] == 0x77 && reportUserData[1] == 0x88);
    for (const auto& report : reports) RequireCpuLoad(report, system);

    Require(sceNgs2ReportUnregisterHandler(handle) == SCE_NGS2_OK);
    Require(Throws<std::invalid_argument>([&] { sceNgs2ReportUnregisterHandler(handle); }));
    reports.clear();
    reportUserData.clear();
    Render(system);
    Require(reports.size() == 1 && reportUserData.size() == 1 && reportUserData[0] == 0x88);
    RequireCpuLoad(reports[0], system);
    Require(sceNgs2ReportUnregisterHandler(second) == SCE_NGS2_OK);

    Require(sceNgs2ReportRegisterHandler(SCE_NGS2_REPORT_TYPE_CPU_LOAD, OnCpuLoadOnce, 0x99, &selfRemovingHandle) == SCE_NGS2_OK);
    reports.clear();
    reportUserData.clear();
    Render(system);
    Render(system);
    Require(reports.size() == 1 && reportUserData.size() == 1 && reportUserData[0] == 0x99);
    Require(Throws<std::invalid_argument>([] { sceNgs2ReportUnregisterHandler(selfRemovingHandle); }));
    Require(sceNgs2SystemDestroy(system, nullptr) == SCE_NGS2_OK);
}
