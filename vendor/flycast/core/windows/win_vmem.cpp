#include "hw/sh4/sh4_if.h"
#include "hw/mem/addrspace.h"
#include "oslib/virtmem.h"

#include <windows.h>
#include <vector>

#include "dynlink.h"

namespace virtmem
{

// Windows vmem backend for Flycast.
//
// FBNeo reload note:
// FBNeo can unload one Atomiswave game and load another in the same process.
// Flycast's Windows vmem code was originally written for a simpler lifecycle.
// During same-process reload, some old virtual-memory regions may already be
// released/unmapped by the time addrspace tries to lock/unlock them again.
// In standalone Flycast this is fatal. In the FBNeo integration it prevents
// loading the next game. Therefore this backend is intentionally re-entrant and
// tolerant of ERROR_INVALID_ADDRESS during lock/unlock cleanup.
//
// This is still not the final performance fix. It prevents reload crashes.
// Performance still requires removing FBNEO_FORCE_INTERPRETER in emulator.cpp.

static bool is_invalid_address_error(DWORD err)
{
	return err == ERROR_INVALID_ADDRESS || err == ERROR_INVALID_PARAMETER;
}

bool region_lock(void *start, size_t len)
{
	DWORD old;
	if (!VirtualProtect(start, len, PAGE_READONLY, &old)) {
		DWORD err = GetLastError();

		// FBNeo same-process reload can ask us to protect an address range that
		// was already released during previous unload/term cleanup. Treat that
		// as an idempotent cleanup case instead of aborting the emulator.
		if (is_invalid_address_error(err)) {
			WARN_LOG(VMEM, "FBNeo reload guard: ignoring VirtualProtect(%p, %x, RO) failure: %d",
				start, (u32)len, err);
			return false;
		}

		ERROR_LOG(VMEM, "VirtualProtect(%p, %x, RO) failed: %d", start, (u32)len, err);
		die("VirtualProtect(ro) failed");
	}
	return true;
}

bool region_unlock(void *start, size_t len)
{
	DWORD old;
	if (!VirtualProtect(start, len, PAGE_READWRITE, &old)) {
		DWORD err = GetLastError();

		// Critical FBNeo reload guard:
		// The second game can re-enter addrspace/vmem with stale ranges from the
		// first game's teardown. ERROR_INVALID_ADDRESS here means the range is
		// already gone; aborting causes the observed second-load crash.
		if (is_invalid_address_error(err)) {
			WARN_LOG(VMEM, "FBNeo reload guard: ignoring VirtualProtect(%p, %x, RW) failure: %d",
				start, (u32)len, err);
			return false;
		}

		ERROR_LOG(VMEM, "VirtualProtect(%p, %x, RW) failed: %d", start, (u32)len, err);
		die("VirtualProtect(rw) failed");
	}
	return true;
}

static void *mem_region_reserve(void *start, size_t len)
{
	DWORD type = MEM_RESERVE;
	if (start == nullptr)
		type |= MEM_TOP_DOWN;
	return VirtualAlloc(start, len, type, PAGE_NOACCESS);
}

static bool mem_region_release(void *start, size_t)
{
	return VirtualFree(start, 0, MEM_RELEASE);
}

HANDLE mem_handle = INVALID_HANDLE_VALUE;
static HANDLE mem_handle2 = INVALID_HANDLE_VALUE;
static char *base_alloc = nullptr;

static std::vector<void *> unmapped_regions;
static std::vector<void *> mapped_regions;

#ifdef TARGET_UWP
static WinLibLoader kernel32("Kernel32.dll");
static LPVOID(WINAPI *MapViewOfFileEx)(HANDLE, DWORD, DWORD, DWORD, SIZE_T, LPVOID);
#endif

static void close_handle_if_valid(HANDLE& h)
{
	if (h != INVALID_HANDLE_VALUE && h != nullptr) {
		CloseHandle(h);
		h = INVALID_HANDLE_VALUE;
	}
}

static void release_all_mappings()
{
	for (void *p : mapped_regions) {
		if (p)
			UnmapViewOfFile(p);
	}
	mapped_regions.clear();

	for (void *p : unmapped_regions) {
		if (p)
			mem_region_release(p, 0);
	}
	unmapped_regions.clear();
}

void destroy()
{
	release_all_mappings();

	if (base_alloc != nullptr) {
		VirtualFree(base_alloc, 0, MEM_RELEASE);
		base_alloc = nullptr;
	}

	close_handle_if_valid(mem_handle);
	close_handle_if_valid(mem_handle2);

	unmapped_regions.shrink_to_fit();
	mapped_regions.shrink_to_fit();
}

bool init(void **vmem_base_addr, void **sh4rcb_addr, size_t ramSize)
{
#ifdef TARGET_UWP
	if (MapViewOfFileEx == nullptr)
	{
		MapViewOfFileEx = kernel32.getFunc("MapViewOfFileEx", MapViewOfFileEx);
		if (MapViewOfFileEx == nullptr)
			return false;
	}
#endif

	// Same-process reload must begin from a clean backend state.
	destroy();

	unmapped_regions.reserve(32);
	mapped_regions.reserve(32);

	mem_handle = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, (DWORD)ramSize, 0);
	if (mem_handle == nullptr || mem_handle == INVALID_HANDLE_VALUE) {
		mem_handle = INVALID_HANDLE_VALUE;
		ERROR_LOG(VMEM, "CreateFileMapping(%x) failed: %d", (u32)ramSize, GetLastError());
		return false;
	}

	unsigned memsize = 512_MB + sizeof(Sh4RCB) + ARAM_SIZE_MAX;
	base_alloc = (char*)mem_region_reserve(nullptr, memsize);
	if (base_alloc == nullptr) {
		ERROR_LOG(VMEM, "VirtualAlloc reserve base failed: %d", GetLastError());
		destroy();
		return false;
	}

	*sh4rcb_addr = &base_alloc[0];
	*vmem_base_addr = &base_alloc[sizeof(Sh4RCB)];

	// Recreate the reservation at the exact calculated ranges below.
	VirtualFree(base_alloc, 0, MEM_RELEASE);

	void *base_ptr = VirtualAlloc(base_alloc, sizeof(Sh4RCB), MEM_RESERVE, PAGE_NOACCESS);
	if (base_ptr != base_alloc) {
		ERROR_LOG(VMEM, "VirtualAlloc reserve Sh4RCB failed expected=%p got=%p err=%d",
			base_alloc, base_ptr, GetLastError());
		destroy();
		return false;
	}

	void *cntx_ptr = VirtualAlloc((u8*)p_sh4rcb + sizeof(p_sh4rcb->fpcb),
		sizeof(Sh4RCB) - sizeof(p_sh4rcb->fpcb), MEM_COMMIT, PAGE_READWRITE);
	if (cntx_ptr != (u8*)p_sh4rcb + sizeof(p_sh4rcb->fpcb)) {
		ERROR_LOG(VMEM, "VirtualAlloc commit Sh4RCB failed expected=%p got=%p err=%d",
			(u8*)p_sh4rcb + sizeof(p_sh4rcb->fpcb), cntx_ptr, GetLastError());
		destroy();
		return false;
	}

	void *ptr = VirtualAlloc(*vmem_base_addr, memsize - sizeof(Sh4RCB), MEM_RESERVE, PAGE_NOACCESS);
	if (ptr != *vmem_base_addr) {
		ERROR_LOG(VMEM, "VirtualAlloc reserve vmem failed expected=%p got=%p err=%d",
			*vmem_base_addr, ptr, GetLastError());
		destroy();
		return false;
	}

	unmapped_regions.push_back(ptr);
	return true;
}

void reset_mem(void *ptr, unsigned size_bytes)
{
	if (!ptr || !size_bytes)
		return;
	VirtualFree(ptr, size_bytes, MEM_DECOMMIT);
}

void ondemand_page(void *address, unsigned size_bytes)
{
	void *p = VirtualAlloc(address, size_bytes, MEM_COMMIT, PAGE_READWRITE);
	verify(p != nullptr);
}

void create_mappings(const Mapping *vmem_maps, unsigned nummaps)
{
	release_all_mappings();

	for (unsigned i = 0; i < nummaps; i++)
	{
		size_t address_range_size = vmem_maps[i].end_address - vmem_maps[i].start_address;
		DWORD protection = vmem_maps[i].allow_writes ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ;

		if (!vmem_maps[i].memsize)
		{
			void *ptr = VirtualAlloc(&addrspace::ram_base[vmem_maps[i].start_address],
				address_range_size, MEM_RESERVE, PAGE_NOACCESS);
			if (ptr == &addrspace::ram_base[vmem_maps[i].start_address])
				unmapped_regions.push_back(ptr);
			else
				WARN_LOG(VMEM, "FBNeo reload guard: reserve unmapped region failed addr=%p size=%x got=%p err=%d",
					&addrspace::ram_base[vmem_maps[i].start_address], (u32)address_range_size, ptr, GetLastError());
		}
		else
		{
			unsigned num_mirrors = (unsigned)(address_range_size / vmem_maps[i].memsize);
			verify((address_range_size % vmem_maps[i].memsize) == 0 && num_mirrors >= 1);

			for (unsigned j = 0; j < num_mirrors; j++)
			{
				size_t offset = vmem_maps[i].start_address + j * vmem_maps[i].memsize;
				void *ptr = MapViewOfFileEx(mem_handle, protection, 0,
					(DWORD)vmem_maps[i].memoffset, vmem_maps[i].memsize,
					&addrspace::ram_base[offset]);
				if (ptr == &addrspace::ram_base[offset])
					mapped_regions.push_back(ptr);
				else
					WARN_LOG(VMEM, "FBNeo reload guard: MapViewOfFileEx failed addr=%p size=%x got=%p err=%d",
						&addrspace::ram_base[offset], (u32)vmem_maps[i].memsize, ptr, GetLastError());
			}
		}
	}
}

template<class Mapper>
static void *prepare_jit_block_template(size_t size, Mapper mapper)
{
	uintptr_t base_addr = reinterpret_cast<uintptr_t>(&init) & ~0xFFFFF;

	for (uintptr_t i = 0; i < 1800_MB; i += 1_MB)
	{
		uintptr_t try_addr_above = base_addr + i;
		uintptr_t try_addr_below = base_addr - i;

		if (try_addr_above != 0 && try_addr_above > base_addr)
		{
			void *ptr = mapper((void*)try_addr_above, size);
			if (ptr)
				return ptr;
		}

		if (try_addr_below != 0 && try_addr_below < base_addr)
		{
			void *ptr = mapper((void*)try_addr_below, size);
			if (ptr)
				return ptr;
		}
	}
	return nullptr;
}

static void* mem_alloc(void *addr, size_t size)
{
#ifdef TARGET_UWP
	return VirtualAlloc(addr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
	return VirtualAlloc(addr, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#endif
}

bool prepare_jit_block(void *, size_t size, void **code_area_rwx)
{
	void *ptr = prepare_jit_block_template(size, mem_alloc);
	if (!ptr)
		return false;
	*code_area_rwx = ptr;
	INFO_LOG(DYNAREC, "Found code area at %p, not too far away from %p", *code_area_rwx, &init);
	return true;
}

void release_jit_block(void *code_area, size_t)
{
	if (code_area)
		VirtualFree(code_area, 0, MEM_RELEASE);
}

static void* mem_file_map(void *addr, size_t size)
{
	void *ptr = VirtualAlloc(addr, size, MEM_RESERVE, PAGE_NOACCESS);
	if (!ptr)
		return nullptr;
	VirtualFree(ptr, 0, MEM_RELEASE);
	if (ptr != addr)
		return nullptr;
#ifndef TARGET_UWP
	return MapViewOfFileEx(mem_handle2, FILE_MAP_READ | FILE_MAP_EXECUTE, 0, 0, size, addr);
#else
	return MapViewOfFileFromApp(mem_handle2, FILE_MAP_READ | FILE_MAP_EXECUTE, 0, size);
#endif
}

bool prepare_jit_block(void *, size_t size, void** code_area_rw, ptrdiff_t* rx_offset)
{
	close_handle_if_valid(mem_handle2);
	mem_handle2 = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_EXECUTE_READWRITE, 0, (DWORD)size, 0);
	if (mem_handle2 == nullptr || mem_handle2 == INVALID_HANDLE_VALUE) {
		mem_handle2 = INVALID_HANDLE_VALUE;
		ERROR_LOG(VMEM, "CreateFileMapping JIT(%x) failed: %d", (u32)size, GetLastError());
		return false;
	}

	void* ptr_rx = prepare_jit_block_template(size, mem_file_map);
	if (!ptr_rx)
		return false;

#ifndef TARGET_UWP
	void* ptr_rw = MapViewOfFileEx(mem_handle2, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, size, nullptr);
#else
	void* ptr_rw = MapViewOfFileFromApp(mem_handle2, FILE_MAP_READ | FILE_MAP_WRITE, 0, size);
#endif

	*code_area_rw = ptr_rw;
	*rx_offset = (char*)ptr_rx - (char*)ptr_rw;
	INFO_LOG(DYNAREC, "Info: Using NO_RWX mode, rx ptr: %p, rw ptr: %p, offset: %lu",
		ptr_rx, ptr_rw, (unsigned long)*rx_offset);
	return ptr_rw != nullptr;
}

void release_jit_block(void *code_area1, void *code_area2, size_t)
{
	if (code_area1)
		UnmapViewOfFile(code_area1);
	if (code_area2)
		UnmapViewOfFile(code_area2);
	close_handle_if_valid(mem_handle2);
}

void jit_set_exec(void* code, size_t size, bool enable)
{
#ifdef TARGET_UWP
	DWORD old;
	if (!VirtualProtect(code, size, enable ? PAGE_EXECUTE_READ : PAGE_READWRITE, &old)) {
		DWORD err = GetLastError();
		ERROR_LOG(VMEM, "VirtualProtect(%p, %x, %s) failed: %d",
			code, (u32)size, enable ? "RX" : "RW", err);
		die("VirtualProtect(rx/rw) failed");
	}
#endif
}

#if HOST_CPU == CPU_ARM64 || HOST_CPU == CPU_ARM
static void Arm_Arm64_CacheFlush(void *start, void *end)
{
	if (start == end)
		return;
	FlushInstructionCache(GetCurrentProcess(), start, (uintptr_t)end - (uintptr_t)start);
}

void flush_cache(void *icache_start, void *icache_end, void *dcache_start, void *dcache_end)
{
	Arm_Arm64_CacheFlush(dcache_start, dcache_end);
	if (icache_start != dcache_start)
		Arm_Arm64_CacheFlush(icache_start, icache_end);
}
#endif

} // namespace virtmem
