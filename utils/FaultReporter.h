// Copyright (c) Chemical Language Foundation 2025.
//
// First-chance fault reporting for the compiler process — a *build-time* debug
// facility, off unless `CHEM_DEBUG_FAULT_REPORT` is defined (Windows only):
//
//     cmake -S . -B cmake-build-debug -DCHEM_DEBUG_FAULT_REPORT=ON
//     ./scripts/build.sh --tcc
//
// When enabled, a vectored exception handler reports every fatal fault before
// the process dies:
//
//   - the exception code, and for an access violation whether it was a read,
//     write or execute and which address was touched,
//   - the faulting instruction pointer, its offset inside the owning module and
//     the instruction bytes (a bad call target looks different from a bad load),
//   - a dbghelp-symbolized stack of the faulting thread, with frames outside
//     every loaded module marked `JIT` (TinyCC-hosted code has no PDB).
//
// Nothing is suppressed: the handler returns `EXCEPTION_CONTINUE_SEARCH`, so the
// normal handler still runs and the process still dies. It is purely extra
// output for a crash you are already debugging.
//
// TinyCC installs a *vectored* exception handler of its own for JIT-hosted code
// (it prints `RUNTIME ERROR: ...` and exits), and vectored handlers run
// last-installed-first — so the reporter is installed in `main` and re-registered
// by the lab right before each call into JIT-hosted code (`refresh()`), otherwise
// TinyCC's handler would win and a compiler-side fault would be reported as a bare
// `RUNTIME ERROR: invalid memory access` with no stack.
//
// Costs nothing when disabled: without `CHEM_DEBUG_FAULT_REPORT` this header
// includes no Windows headers, links no extra library and compiles to an empty
// `refresh()`.

#pragma once

#if defined(_WIN32) && defined(CHEM_DEBUG_FAULT_REPORT)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstring>

// only this translation unit needs the symbolizer, so only a diagnostic build
// links it
#pragma comment(lib, "dbghelp.lib")

namespace chem::fault_reporter {

    namespace detail {

        inline thread_local bool in_reporter = false;
        inline PVOID handler = nullptr;

        inline const char* access_kind(ULONG_PTR type) {
            switch (type) {
                case 0: return "read";
                case 1: return "write";
                case 8: return "execute";
                default: return "?";
            }
        }

        inline bool symbolicate(HANDLE process, DWORD64 addr, char* out, size_t cap) {
            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = {0};
            auto* sym = reinterpret_cast<SYMBOL_INFO*>(buffer);
            sym->SizeOfStruct = sizeof(SYMBOL_INFO);
            sym->MaxNameLen = MAX_SYM_NAME;
            DWORD64 displacement = 0;
            if (!SymFromAddr(process, addr, &displacement, sym)) return false;
            IMAGEHLP_LINE64 line = {};
            line.SizeOfStruct = sizeof(line);
            DWORD line_displacement = 0;
            if (SymGetLineFromAddr64(process, addr, &line_displacement, &line)) {
                std::snprintf(out, cap, "%s+0x%llx (%s:%lu)", sym->Name,
                              (unsigned long long) displacement, line.FileName,
                              (unsigned long) line.LineNumber);
            } else {
                std::snprintf(out, cap, "%s+0x%llx", sym->Name,
                              (unsigned long long) displacement);
            }
            return true;
        }

        inline void report_fault(EXCEPTION_POINTERS* info) {
            if (in_reporter) return; // a fault inside the reporter: stay quiet
            in_reporter = true;

            const EXCEPTION_RECORD* rec = info->ExceptionRecord;
            const CONTEXT* ctx = info->ContextRecord;
            std::fprintf(stderr, "\n==== CHEM_DEBUG_FAULT_REPORT ====\n");
            std::fprintf(stderr, "exception 0x%08lx at %p", (unsigned long) rec->ExceptionCode,
                         (void*) rec->ExceptionAddress);
            if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2) {
                std::fprintf(stderr, " (%s of 0x%llx)",
                             access_kind(rec->ExceptionInformation[0]),
                             (unsigned long long) rec->ExceptionInformation[1]);
            }
            std::fprintf(stderr, "\n");

            const HANDLE process = GetCurrentProcess();
            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
            SymInitialize(process, nullptr, TRUE);

            const DWORD64 rip = (DWORD64) ctx->Rip;
            HMODULE owner = nullptr;
            char owner_name[MAX_PATH] = {0};
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   (LPCWSTR) rip, &owner) && owner != nullptr) {
                GetModuleFileNameA(owner, owner_name, sizeof(owner_name));
            }
            char sym[1024] = {0};
            const bool has_sym = symbolicate(process, rip, sym, sizeof(sym));
            std::fprintf(stderr, "rip=0x%llx in %s", (unsigned long long) rip,
                         owner ? owner_name : "<JIT / not in any loaded module>");
            if (owner) std::fprintf(stderr, "+0x%llx", (unsigned long long) (rip - (DWORD64) owner));
            std::fprintf(stderr, " %s\n", has_sym ? sym : "???");
            std::fprintf(stderr, "rsp=0x%llx rbp=0x%llx\n", (unsigned long long) ctx->Rsp,
                         (unsigned long long) ctx->Rbp);

            // The faulting instruction bytes separate "called a bad pointer" from
            // "bad load/store", which decides where to look next.
            __try {
                unsigned char bytes[16];
                std::memcpy(bytes, (const void*) rip, sizeof(bytes));
                std::fprintf(stderr, "bytes=");
                for (unsigned char b : bytes) std::fprintf(stderr, "%02x", b);
                std::fprintf(stderr, "\n");
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                std::fprintf(stderr, "bytes=<unreadable>\n");
            }

            void* frames[64];
            const USHORT count = CaptureStackBackTrace(0, 64, frames, nullptr);
            std::fprintf(stderr, "stack (%u frames, innermost first):\n", (unsigned) count);
            for (USHORT i = 0; i < count; i++) {
                const DWORD64 frame = (DWORD64) frames[i];
                char frame_sym[1024] = {0};
                if (symbolicate(process, frame, frame_sym, sizeof(frame_sym))) {
                    std::fprintf(stderr, "  #%02u 0x%llx %s\n", (unsigned) i,
                                 (unsigned long long) frame, frame_sym);
                } else {
                    HMODULE frame_owner = nullptr;
                    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                           (LPCWSTR) frame, &frame_owner) && frame_owner) {
                        std::fprintf(stderr, "  #%02u 0x%llx module+0x%llx\n", (unsigned) i,
                                     (unsigned long long) frame,
                                     (unsigned long long) (frame - (DWORD64) frame_owner));
                    } else {
                        std::fprintf(stderr, "  #%02u 0x%llx JIT\n", (unsigned) i,
                                     (unsigned long long) frame);
                    }
                }
            }
            std::fprintf(stderr, "==== END CHEM_DEBUG_FAULT_REPORT ====\n");
            std::fflush(stderr);
            in_reporter = false;
        }

        inline LONG CALLBACK crash_bt_handler(EXCEPTION_POINTERS* info) {
            const DWORD code = info->ExceptionRecord->ExceptionCode;
            const bool fatal = code == EXCEPTION_ACCESS_VIOLATION ||
                               code == EXCEPTION_STACK_OVERFLOW ||
                               code == EXCEPTION_ILLEGAL_INSTRUCTION ||
                               code == EXCEPTION_INT_DIVIDE_BY_ZERO ||
                               code == EXCEPTION_PRIV_INSTRUCTION ||
                               code == EXCEPTION_IN_PAGE_ERROR;
            if (!fatal) return EXCEPTION_CONTINUE_SEARCH;
            report_fault(info);
            return EXCEPTION_CONTINUE_SEARCH;
        }

    } // namespace detail

    /**
     * Install the fault reporter, or move it to the front of the vectored chain
     * when it is already installed (see the header comment: TinyCC installs its
     * own handler and handlers run last-installed-first).
     */
    inline void refresh() {
        if (detail::handler != nullptr) {
            RemoveVectoredExceptionHandler(detail::handler);
        }
        detail::handler = AddVectoredExceptionHandler(1, detail::crash_bt_handler);
    }

} // namespace chem::fault_reporter

#else

namespace chem::fault_reporter {

    /** No-op: the fault reporter is not compiled in (see the header comment). */
    inline void refresh() {}

} // namespace chem::fault_reporter

#endif
