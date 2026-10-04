#include "common/abi.h"
#include "common/common.h"
#include "common/emulatorConfig.h"
#include "common/stringUtils.h"
#include "kernel/fileSystem.h"
#include "kernel/memory.h"
#include "libs/errno.h"
#include "libs/libs.h"
#include "loader/symbolDatabase.h"

// Homebrew built on the console's own C library ships no libc of its own. On POSIX hosts the
// guest and host share the SysV ABI, so those imports can go straight to the host libc.
#if !defined(_WIN32)

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <strings.h>
#include <system_error>

namespace Libs::LibcHost {

LIB_VERSION("libc", 1, "libc", 1, 1);

static int g_is_threaded = 1;

namespace Host {

static KYTY_SYSV_ABI wchar_t* wcschr(const wchar_t* s, wchar_t c) {
	return const_cast<wchar_t*>(std::wcschr(s, c));
}

static KYTY_SYSV_ABI void* memchr(const void* s, int c, size_t n) {
	return const_cast<void*>(std::memchr(s, c, n));
}

static KYTY_SYSV_ABI double exp(double x) {
	return std::exp(x);
}

static KYTY_SYSV_ABI double exp2(double x) {
	return std::exp2(x);
}

static KYTY_SYSV_ABI float exp2f(float x) {
	return std::exp2(x);
}

static KYTY_SYSV_ABI double log10(double x) {
	return std::log10(x);
}

static KYTY_SYSV_ABI int isalnum(int c) {
	return std::isalnum(c);
}

static KYTY_SYSV_ABI int isspace(int c) {
	return std::isspace(c);
}

static KYTY_SYSV_ABI int tolower(int c) {
	return std::tolower(c);
}

static KYTY_SYSV_ABI int toupper(int c) {
	return std::toupper(c);
}

static KYTY_SYSV_ABI FILE* fopen(const char* path, const char* mode) {
	const auto real = LibKernel::FileSystem::GetRealFilename(path);
	return std::fopen(real.c_str(), mode);
}

// A guest descriptor is an index into the emulated file table, not a host descriptor.
static KYTY_SYSV_ABI FILE* fdopen(int /*fd*/, const char* /*mode*/) {
	*Posix::GetErrorAddr() = Posix::POSIX_EBADF;
	return nullptr;
}

static KYTY_SYSV_ABI int _Getmbcurmax() {
	return 1;
}

static KYTY_SYSV_ABI int access(const char* path, int /*mode*/) {
	std::error_code ec;
	if (std::filesystem::exists(LibKernel::FileSystem::GetRealFilename(path), ec)) {
		return 0;
	}
	*Posix::GetErrorAddr() = Posix::POSIX_ENOENT;
	return -1;
}

static KYTY_SYSV_ABI int rename(const char* from, const char* to) {
	return POSIX_CALL(LibKernel::FileSystem::KernelRename(from, to));
}

static KYTY_SYSV_ABI void* mmap(void* addr, size_t len, int prot, int /*flags*/, int fd,
                                int64_t /*offset*/) {
	constexpr size_t page = 0x4000;
	void*            out  = addr;
	if (fd != -1 || LibKernel::Memory::KernelMapFlexibleMemory(&out, (len + page - 1) & ~(page - 1),
	                                                           prot, 0) != 0) {
		*Posix::GetErrorAddr() = Posix::POSIX_ENOMEM;
		return reinterpret_cast<void*>(-1); // NOLINT(performance-no-int-to-ptr)
	}
	return out;
}

static KYTY_SYSV_ABI int sceKernelDebugOutText(int /*channel*/, const char* text) {
	if (text != nullptr) {
		std::fputs(text, stdout);
		std::fflush(stdout);
	}
	return 0;
}

// Report AGC as unavailable. Homebrew then falls back to drawing into the display buffers on
// the CPU, which needs no hand built command stream.
static KYTY_SYSV_ABI int sceAgcInit(uint32_t /*version*/) {
	return -1;
}

static KYTY_SYSV_ABI int sceUserServiceGetForegroundUser(int* user_id) {
	if (user_id != nullptr) {
		*user_id = Config::GetUserId();
	}
	return 0;
}

} // namespace Host

LIB_DEFINE(InitLibcHost_1) {
	LIB_FUNC("8Q60JLJ6Rv4", ::getc);
	LIB_FUNC("Noj9PsJrsa8", ::wmemmove);
	LIB_FUNC("WkkeywLJcgU", ::wcslen);
	LIB_FUNC("aZK8lNei-Qw", ::fputc);
	LIB_FUNC("iVhJZvAO2aQ", ::lldiv);
	LIB_FUNC("qVHpv0PxouI", ::mbrtowc);
	LIB_FUNC("y9OoA+P5cjk", ::wcrtomb);
	LIB_FUNC("2X5agFjKxMc", ::calloc);
	LIB_FUNC("uodLYyUip20", ::fclose);
	LIB_FUNC("LxcEU+ICu8U", ::feof);
	LIB_FUNC("KdP-nULpuGw", ::fgets);
	LIB_FUNC("fffwELXNVFA", ::fprintf);
	LIB_FUNC("lbB+UlZqVG0", ::fread);
	LIB_FUNC("tIhsqj0qsFE", ::free);
	LIB_FUNC("npLpPTaSuHg", ::fscanf);
	LIB_FUNC("rQFVBXp-Cxg", ::fseek);
	LIB_FUNC("Qazy8LmXTvw", ::ftell);
	LIB_FUNC("MpxhMh8QFro", ::fwrite);
	LIB_FUNC("gQX+4GDQjpM", ::malloc);
	LIB_FUNC("AEJdIVZTEmo", ::qsort);
	LIB_FUNC("Y7aJ1uydPMo", ::realloc);
	LIB_FUNC("vZMcAfsA31I", ::setbuf);
	LIB_FUNC("QMFyLoqNxIg", ::setvbuf);
	LIB_FUNC("tcVi5SivF7Q", ::sprintf);
	LIB_FUNC("1Pk0qZQGeWo", ::sscanf);
	LIB_FUNC("AV6ipCNa4Rw", ::strcasecmp);
	LIB_FUNC("kiZSXIWd9vg", ::strcpy);
	LIB_FUNC("q0F6yS-rCms", ::strcspn);
	LIB_FUNC("g7zzzLDYGw0", ::strdup);
	LIB_FUNC("pXvbDfchu6k", ::strncasecmp);
	LIB_FUNC("kHg45qPC6f0", ::strncat);
	LIB_FUNC("enqPGLfmVNU", ::strtok_r);
	LIB_FUNC("VOBg+iNwB-4", ::strtoll);
	LIB_FUNC("Q2V+iqvjgC0", ::vsnprintf);
	LIB_FUNC("yDnwZsMnX0Q", ::__srget);
	LIB_FUNC("Ezzq78ZgHPs", Host::wcschr);
	LIB_FUNC("8u8lPzUEq+U", Host::memchr);
	LIB_FUNC("NVadfnzQhHQ", Host::exp);
	LIB_FUNC("dnaeGXbjP6E", Host::exp2);
	LIB_FUNC("wuAQt-j+p4o", Host::exp2f);
	LIB_FUNC("WuMbPBKN1TU", Host::log10);
	LIB_FUNC("4uJJNi+C9wk", Host::isalnum);
	LIB_FUNC("wazw2x2m3DQ", Host::isspace);
	LIB_FUNC("PqF+kHW-2WQ", Host::tolower);
	LIB_FUNC("TYE4irxSmko", Host::toupper);
	LIB_FUNC("xeYO4u7uyJ0", Host::fopen);
	LIB_FUNC("qdlHjTa9hQ4", Host::fdopen);
	LIB_FUNC("U52BlHBvYvE", Host::_Getmbcurmax);
	LIB_FUNC("8vE6Z6VEYyk", Host::access);
	LIB_FUNC("NN01qLRhiqU", Host::rename);
	LIB_FUNC("BPE9s9vQQXo", Host::mmap);
	LIB_FUNC("9JYNqN6jAKI", Host::sceKernelDebugOutText);
	LIB_FUNC("kW3GLb7QfPg", Host::sceAgcInit);
	LIB_FUNC("eNb53LQJmIM", Host::sceUserServiceGetForegroundUser);
	LIB_OBJECT("zqJhBxAKfsc", &__stdoutp);
	LIB_OBJECT("as8Od-tH1BI", &__stderrp);
	LIB_OBJECT("bgAcsbcEznc", &__stdinp);
	LIB_OBJECT("q1OvUam0BJU", &g_is_threaded);
}

} // namespace Libs::LibcHost

#endif
