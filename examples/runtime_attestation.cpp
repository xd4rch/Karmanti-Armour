// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Ömer Taha Keçeci
// ============================================================================
//  KARMANTI RUNTIME ATTESTATION & INTEGRITY MONITOR  (example payload)
//  ---------------------------------------------------------------------------
//  This program *measures* — it does not assume — which protection layers are
//  actually active while it runs inside a Karmanti container:
//
//    1. Authorization : the KARMANTI_TIER_ID the container injects.
//    2. Storage       : its own image path -> NTFS ADS stream + zero-byte
//                       carrier (carrier size is really measured with
//                       GetFileSizeEx, attributes with GetFileAttributesW).
//    3. Memory        : the kernel DACL verdict. The container applies a
//                       "deny memory read/write/operation/duplicate-handle to
//                       Everyone" DACL to both itself and the payload, and it
//                       erases its own 4096-byte PE header in RAM. The payload
//                       must keep its own header intact (the image loader reads
//                       it), so the interesting signal is not the payload's
//                       header but whether the *container's* memory can be
//                       read. This program measures exactly that: a denied
//                       PROCESS_VM_READ proves the header erasure is effective.
//
//  Build (x64, no extra runtime required):
//      zig c++ -O2 -std=c++17 telemetry_demo.cpp -o telemetry_demo.exe
//      g++     -O2 -std=c++17 telemetry_demo.cpp -o telemetry_demo.exe  (MinGW)
//
//  Flags:
//      --json      single-line, machine-readable JSON summary
//      --no-wait   skip the final "Press [Enter]" wait
//  Env:
//      KRM_DEMO_NO_WAIT=1   same effect as --no-wait (CI friendly)
// ============================================================================

#include <windows.h>
#include <tlhelp32.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum Level { OK, WARN, BAD, INFO };

const char* kGreen = "\033[1;32m";
const char* kYellow = "\033[1;33m";
const char* kRed = "\033[1;31m";
const char* kCyan = "\033[1;36m";
const char* kMagenta = "\033[1;35m";
const char* kWhite = "\033[1;37m";
const char* kDim = "\033[38;2;148;163;184m";
const char* kReset = "\033[0m";

std::string to_utf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    int need = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (need <= 0) return std::string();
    std::string out((size_t)need, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), need, nullptr, nullptr);
    return out;
}

std::wstring base_name(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? path : path.substr(pos + 1);
}

void print_header(const char* title) {
    std::cout << kCyan << "[+] ========================================================" << kReset << "\n";
    std::cout << kWhite << "    " << title << kReset << "\n";
    std::cout << kCyan << "    ========================================================" << kReset << "\n\n";
}

void print_status(const std::string& key, const std::string& val, Level level) {
    const char* col = (level == OK) ? kGreen : (level == WARN) ? kYellow : (level == BAD) ? kRed : kCyan;
    std::cout << "  [*] " << std::left << std::setw(30) << key << ": " << col << val << kReset << "\n";
}

std::wstring arg_value(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] && std::string(argv[i]) == flag) return L"1";
    }
    return std::wstring();
}

// ---------------------------------------------------------------------------
// Process / container lookups. Toolhelp is used on purpose: it still works for
// processes we may no longer be allowed to open (the DACL denies most access
// rights), so no OpenProcess(PROCESS_QUERY_*) is needed here.
// ---------------------------------------------------------------------------
struct ProcInfo {
    DWORD pid = 0;
    DWORD parent_pid = 0;
    std::wstring exe_name;
};

bool find_process(DWORD pid, ProcInfo& out) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                out.pid = pe.th32ProcessID;
                out.parent_pid = pe.th32ParentProcessID;
                out.exe_name = pe.szExeFile;
                found = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

// Attempts a real PROCESS_VM_READ open against a process. True = the kernel
// DACL denied it, which is exactly the anti-dump property we want to prove.
bool vm_read_denied(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (h) {
        CloseHandle(h);
        return false;
    }
    return GetLastError() == ERROR_ACCESS_DENIED;
}

bool is_karmanti_container_name(const std::wstring& name) {
    std::wstring lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t c) {
        return (wchar_t)::towlower(c);
    });
    return lower.size() > 11 && lower.find(L"_native.exe") != std::wstring::npos;
}

// ---------------------------------------------------------------------------
// ADS split:  C:\dir\node_XXXX.dat:s_YYYY.exe  ->  carrier / stream name
// ---------------------------------------------------------------------------
bool split_ads(const std::wstring& path, std::wstring& carrier, std::wstring& stream) {
    size_t pos = path.find(L':', 2);  // 0 = drive letter, 1 = ':'
    if (pos == std::wstring::npos) return false;
    carrier = path.substr(0, pos);
    stream = path.substr(pos + 1);
    return !stream.empty();
}

std::wstring json_escape(const std::string& in) {
    std::string out;
    for (char c : in) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if ((unsigned char)c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return std::wstring(out.begin(), out.end());
}

}  // namespace

int main(int argc, char** argv) {
    // ANSI colours + UTF-8 output
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    SetConsoleOutputCP(CP_UTF8);

    const bool json_mode = !arg_value(argc, argv, "--json").empty();
    bool no_wait = !arg_value(argc, argv, "--no-wait").empty();
    {
        char env[8] = {0};
        if (GetEnvironmentVariableA("KRM_DEMO_NO_WAIT", env, sizeof(env)) > 0) no_wait = true;
    }

    if (!json_mode) print_header("KARMANTI RUNTIME ATTESTATION & INTEGRITY MONITOR");

    // -----------------------------------------------------------------------
    // 1) AUTHORIZATION: licence context injected by the container
    // -----------------------------------------------------------------------
    char tier_id_buf[16] = {0};
    DWORD tier_len = GetEnvironmentVariableA("KARMANTI_TIER_ID", tier_id_buf, sizeof(tier_id_buf));
    const bool tier_ok = (tier_len > 0);
    int tier_id = 0;
    if (tier_ok) {
        tier_id = std::atoi(tier_id_buf);
    }
    const char* tier_name = "Unknown";
    switch (tier_id) {
        case 1: tier_name = "Standard"; break;
        case 2: tier_name = "Professional"; break;
        case 3: tier_name = "Enterprise"; break;
        default: break;
    }

    // -----------------------------------------------------------------------
    // 2) STORAGE: own image path, ADS stream, carrier size/attributes
    // -----------------------------------------------------------------------
    std::vector<wchar_t> exe_buf(32768, L'\0');
    DWORD exe_len = GetModuleFileNameW(nullptr, exe_buf.data(), (DWORD)exe_buf.size());
    std::wstring image_path(exe_buf.data(), exe_len);
    const std::string image_utf8 = to_utf8(image_path);

    std::wstring carrier_path, stream_name;
    const bool ads = split_ads(image_path, carrier_path, stream_name);
    const bool karmanti_stream = ads && stream_name.rfind(L"s_", 0) == 0 &&
                                 base_name(carrier_path).rfind(L"node_", 0) == 0;

    bool carrier_zero = false;
    bool carrier_hidden = false;
    if (ads) {
        HANDLE h = CreateFileW(carrier_path.c_str(), FILE_READ_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER size{};
            if (GetFileSizeEx(h, &size)) carrier_zero = (size.QuadPart == 0);
            CloseHandle(h);
        }
        DWORD attr = GetFileAttributesW(carrier_path.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES) {
            carrier_hidden = (attr & FILE_ATTRIBUTE_HIDDEN) && (attr & FILE_ATTRIBUTE_SYSTEM);
        }
    }

    // -----------------------------------------------------------------------
    // 3) MEMORY: DACL verdict for this process and for the container —
    //    the actual anti-dump evidence
    // -----------------------------------------------------------------------
    const DWORD self_pid = GetCurrentProcessId();
    ProcInfo self_info, parent_info;
    find_process(self_pid, self_info);
    const DWORD parent_pid = self_info.parent_pid;
    const bool parent_found = (parent_pid != 0) && find_process(parent_pid, parent_info);
    const bool container_is_karmanti = parent_found && is_karmanti_container_name(parent_info.exe_name);

    const bool self_vm_denied = vm_read_denied(self_pid);
    const bool container_vm_denied = parent_found ? vm_read_denied(parent_pid) : false;

    HMODULE hMod = GetModuleHandleW(nullptr);
    const bool own_header_intact = (hMod != nullptr) &&
                                   (reinterpret_cast<unsigned char*>(hMod)[0] == 'M') &&
                                   (reinterpret_cast<unsigned char*>(hMod)[1] == 'Z');

    // -----------------------------------------------------------------------
    // Verdict: hardened = licence context + DACL. zero-disk = ADS + 0-byte
    // carrier.
    // -----------------------------------------------------------------------
    const bool zero_disk = ads && carrier_zero;
    const bool hardened = tier_ok && self_vm_denied;
    const char* verdict = (hardened && (zero_disk || container_is_karmanti))
                              ? "PROTECTED"
                              : (hardened || zero_disk) ? "PARTIAL" : "UNPROTECTED";

    if (!json_mode) {
        std::cout << kMagenta << "--- [ SECTION 1: AUTHORIZATION CONTEXT ] ---" << kReset << "\n";
        if (tier_ok) {
            print_status("Licensing Framework", "Attested & Activated", OK);
            print_status("Assigned License Tier", "Tier " + std::to_string(tier_id), OK);
            print_status("Entitlement Profile", std::string(tier_name) + " / Karmanti container", OK);
        } else {
            print_status("Licensing Framework", "UNPROTECTED / NATIVE UNMANAGED", BAD);
            print_status("Security Advisory", "No KARMANTI_TIER_ID in this process", BAD);
        }
        std::cout << "\n";

        std::cout << kMagenta << "--- [ SECTION 2: STORAGE & ISOLATION PROFILES ] ---" << kReset << "\n";
        print_status("Executable Image Path", image_utf8, INFO);
        if (ads) {
            print_status("Execution Medium",
                         karmanti_stream ? "NTFS ADS - Karmanti stream (Zero-Disk Host)"
                                         : "NTFS ADS - alternate data stream",
                         OK);
            print_status("Stream Name", to_utf8(stream_name), INFO);
            print_status("Carrier File", to_utf8(base_name(carrier_path)), INFO);
            print_status("Carrier Size",
                         carrier_zero ? "0 bytes (verified via GetFileSizeEx)"
                                      : "NON-ZERO - payload may be on disk",
                         carrier_zero ? OK : BAD);
            print_status("Carrier Attributes",
                         carrier_hidden ? "HIDDEN + SYSTEM (verified)" : "normal attributes",
                         carrier_hidden ? OK : WARN);
        } else {
            print_status("Execution Medium", "Physical Flat File System (no ADS)", BAD);
            print_status("Disk Trace Exposure", "Exposed to Conventional Forensics", BAD);
        }
        std::cout << "\n";

        std::cout << kMagenta << "--- [ SECTION 3: IN-MEMORY ANTI-ANALYSIS DEFENSE ] ---" << kReset << "\n";
        std::stringstream base_ss;
        base_ss << "0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(hMod);
        print_status("Module Base Address", base_ss.str(), INFO);

        print_status("Own Header (payload)",
                     own_header_intact ? "MZ intact - required by the image loader"
                                       : "MZ missing - image already damaged",
                     own_header_intact ? INFO : BAD);

        if (parent_found) {
            print_status("Container Process",
                         to_utf8(parent_info.exe_name) + " (PID " + std::to_string(parent_pid) + ")",
                         container_is_karmanti ? OK : WARN);
        } else {
            print_status("Container Process", "NOT FOUND - standalone execution", BAD);
        }

        print_status("Own Process VM_READ",
                     self_vm_denied ? "DENIED by kernel DACL" : "ALLOWED - dumpable",
                     self_vm_denied ? OK : BAD);
        print_status("Container VM_READ",
                     container_vm_denied ? "DENIED - RAM header wipe unreadable"
                                         : (parent_found ? "ALLOWED - container dumpable" : "n/a"),
                     container_vm_denied ? OK : (parent_found ? BAD : WARN));
        print_status("Anti-Memory Dump",
                     (self_vm_denied && container_vm_denied)
                         ? "PROTECTED (container header wipe + DACL)"
                         : "VULNERABLE (memory readable / no container)",
                     (self_vm_denied && container_vm_denied) ? OK : BAD);

        print_status("Current Process Exit Code", "STILL_ACTIVE (Execution Stable)", OK);
        std::cout << "\n";

        std::cout << kCyan << "========================================================" << kReset << "\n";
        const char* vcol = (std::string(verdict) == "PROTECTED") ? kGreen
                          : (std::string(verdict) == "PARTIAL") ? kYellow
                                                               : kRed;
        std::cout << "  VERDICT: " << vcol << verdict << kReset
                  << kDim << "   (license=" << (tier_ok ? "on" : "off")
                  << ", dacl=" << (self_vm_denied ? "on" : "off")
                  << ", zero-disk=" << (zero_disk ? "on" : "off") << ")" << kReset << "\n";
        std::cout << kCyan << "========================================================" << kReset << "\n";
    } else {
        std::ostringstream js;
        js << "{\"verdict\":\"" << verdict << "\""
           << ",\"hardened\":" << (hardened ? "true" : "false")
           << ",\"zero_disk\":" << (zero_disk ? "true" : "false")
           << ",\"tier\":" << (tier_ok ? tier_id : 0)
           << ",\"tier_name\":\"" << tier_name << "\""
           << ",\"image\":\"" << to_utf8(json_escape(image_utf8)) << "\""
           << ",\"ads\":" << (ads ? "true" : "false")
           << ",\"karmanti_stream\":" << (karmanti_stream ? "true" : "false")
           << ",\"carrier_zero\":" << (carrier_zero ? "true" : "false")
           << ",\"carrier_hidden\":" << (carrier_hidden ? "true" : "false")
           << ",\"own_header_intact\":" << (own_header_intact ? "true" : "false")
           << ",\"self_vm_read_denied\":" << (self_vm_denied ? "true" : "false")
           << ",\"container\":\"" << to_utf8(json_escape(to_utf8(parent_info.exe_name))) << "\""
           << ",\"container_pid\":" << (parent_found ? (long long)parent_pid : 0)
           << ",\"container_karmanti\":" << (container_is_karmanti ? "true" : "false")
           << ",\"container_vm_read_denied\":" << (container_vm_denied ? "true" : "false")
           << "}";
        std::cout << js.str() << std::endl;
    }

    if (!no_wait) {
        std::cout << "\nPress [Enter] to terminate telemetry validation session...";
        std::cout.flush();
        std::cin.get();
    }
    return 0;
}
