#include <windows.h>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <psapi.h>

#include "ReClassNET_Plugin.hpp"

template<typename T>
class FlexibleBuffer
{
public:
	FlexibleBuffer(std::size_t initialSize)
		: data(initialSize)
	{

	}

	void Grow()
	{
		data.resize(data.size() * 2);
	}

	size_t GetSize() const
	{
		return data.size();
	}

	T* GetPointer()
	{
		return reinterpret_cast<T*>(data.data());
	}

	T* operator->()
	{
		return GetPointer();
	}

private:
	std::vector<uint8_t> data;
};
//---------------------------------------------------------------------------
std::vector<RC_Pointer> GetAvailableHandles(DWORD desiredAccess)
{
	using NTSTATUS = LONG;

	constexpr NTSTATUS STATUS_SUCCESS = 0x00000000;
	constexpr NTSTATUS STATUS_INFO_LENGTH_MISMATCH = 0xC0000004;

	struct SYSTEM_HANDLE_TABLE_ENTRY_INFO
	{
		ULONG ProcessId;
		BYTE ObjectTypeNumber;
		BYTE Flags;
		USHORT Handle;
		PVOID Object;
		DWORD GrantedAccess;
	};

	struct SYSTEM_HANDLE_INFORMATION
	{
		ULONG NumberOfHandles;
		SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[ANYSIZE_ARRAY];
	};

	struct UNICODE_STRING
	{
		USHORT Length;
		USHORT MaximumLength;
		PWSTR  Buffer;
	};

	struct OBJECT_TYPE_INFORMATION
	{
		UNICODE_STRING TypeName;
		ULONG Reserved[60];
	};

	enum class SYSTEM_INFORMATION_CLASS
	{
		SystemHandleInformation = 16
	};

	enum class OBJECT_INFORMATION_CLASS
	{
		ObjectBasicInformation = 0,
		ObjectTypeInformation = 2
	};

	using NtQuerySystemInformation_t = NTSTATUS(__stdcall *)(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
	using NtQueryObject_t = NTSTATUS(__stdcall *)(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);

	const auto moduleHandle = GetModuleHandleW(L"ntdll.dll");

	const auto NtQuerySystemInformation = reinterpret_cast<NtQuerySystemInformation_t>(GetProcAddress(moduleHandle, "NtQuerySystemInformation"));
	const auto NtQueryObject = reinterpret_cast<NtQueryObject_t>(GetProcAddress(moduleHandle, "NtQueryObject"));

	std::vector<RC_Pointer> handles;

	FlexibleBuffer<SYSTEM_HANDLE_INFORMATION> handleInformation(4096);
	while (true)
	{
		ULONG dummy;
		const auto status = NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS::SystemHandleInformation, handleInformation.GetPointer(), handleInformation.GetSize(), &dummy);
		if (status == STATUS_SUCCESS)
		{
			break;
		}
		if (status == STATUS_INFO_LENGTH_MISMATCH)
		{
			handleInformation.Grow();
		}
		else
		{
			return handles;
		}
	}

	const auto currentPid = GetCurrentProcessId();

	for (auto i = 0u; i != handleInformation->NumberOfHandles; i++)
	{
		auto& handleEntry = handleInformation->Handles[i];

		if (handleEntry.ProcessId == currentPid)
		{
			OBJECT_TYPE_INFORMATION objectTypeInfo;
			ULONG dummy;
			const auto status = NtQueryObject(reinterpret_cast<HANDLE>(handleEntry.Handle), OBJECT_INFORMATION_CLASS::ObjectTypeInformation, &objectTypeInfo, sizeof(objectTypeInfo), &dummy);
			if (status == STATUS_SUCCESS)
			{
				if (std::wcscmp(objectTypeInfo.TypeName.Buffer, L"Process") == 0)
				{
					if ((handleEntry.GrantedAccess & desiredAccess) == desiredAccess)
					{
						handles.push_back(reinterpret_cast<RC_Pointer>(handleEntry.Handle));
					}
				}
			}
		}
	}

	return handles;
}
//---------------------------------------------------------------------------
void EnumerateRemoteSectionsAndModules(RC_Pointer remoteId, const std::function<void(RC_Pointer, RC_Pointer, std::wstring&&)>& moduleCallback, const std::function<void(RC_Pointer, RC_Pointer, SectionType, SectionCategory, SectionProtection, std::wstring&&, std::wstring&&)>& sectionCallback)
{
	std::vector<EnumerateRemoteSectionData> sections;

	MEMORY_BASIC_INFORMATION memInfo = {};
	memInfo.RegionSize = 0x1000;
	size_t address = 0;
	while (VirtualQueryEx(remoteId, reinterpret_cast<LPCVOID>(address), &memInfo, sizeof(MEMORY_BASIC_INFORMATION)) != 0 && address + memInfo.RegionSize > address)
	{
		if (memInfo.State == MEM_COMMIT)
		{
			EnumerateRemoteSectionData section = {};
			section.BaseAddress = memInfo.BaseAddress;
			section.Size = memInfo.RegionSize;

			switch (memInfo.Protect & 0xFF)
			{
			case PAGE_EXECUTE:
				section.Protection = SectionProtection::Execute;
				break;
			case PAGE_EXECUTE_READ:
				section.Protection = SectionProtection::Execute | SectionProtection::Read;
				break;
			case PAGE_EXECUTE_READWRITE:
			case PAGE_EXECUTE_WRITECOPY:
				section.Protection = SectionProtection::Execute | SectionProtection::Read | SectionProtection::Write;
				break;
			case PAGE_NOACCESS:
				section.Protection = SectionProtection::NoAccess;
				break;
			case PAGE_READONLY:
				section.Protection = SectionProtection::Read;
				break;
			case PAGE_READWRITE:
			case PAGE_WRITECOPY:
				section.Protection = SectionProtection::Read | SectionProtection::Write;
				break;
			}
			if ((memInfo.Protect & PAGE_GUARD) == PAGE_GUARD)
			{
				section.Protection |= SectionProtection::Guard;
			}

			switch (memInfo.Type)
			{
			case MEM_IMAGE:
				section.Type = SectionType::Image;
				break;
			case MEM_MAPPED:
				section.Type = SectionType::Mapped;
				break;
			case MEM_PRIVATE:
				section.Type = SectionType::Private;
				break;
			}

			section.Category = section.Type == SectionType::Private ? SectionCategory::HEAP : SectionCategory::Unknown;

			sections.push_back(std::move(section));
		}
		address = reinterpret_cast<size_t>(memInfo.BaseAddress) + memInfo.RegionSize;
	}

	DWORD needed;
	if (EnumProcessModules(remoteId, nullptr, 0, &needed))
	{
		std::vector<HMODULE> modules(needed / sizeof(HMODULE));
		if (EnumProcessModules(remoteId, modules.data(), needed, &needed))
		{
			for (HMODULE curModule : modules)
			{
				MODULEINFO moduleInfo = {};
				wchar_t modulepath[MAX_PATH] = {};

				if (GetModuleInformation(remoteId, curModule, &moduleInfo, sizeof(moduleInfo)) &&
					GetModuleFileNameExW(remoteId, curModule, modulepath, MAX_PATH))
				{
					moduleCallback((RC_Pointer)moduleInfo.lpBaseOfDll, (RC_Pointer)moduleInfo.SizeOfImage, modulepath);


					auto it = std::lower_bound(std::begin(sections), std::end(sections), static_cast<LPVOID>(moduleInfo.lpBaseOfDll), [&sections](const auto& lhs, const LPVOID& rhs)
					{
						return lhs.BaseAddress < rhs;
					});

					IMAGE_DOS_HEADER DosHdr = {};
					IMAGE_NT_HEADERS NtHdr = {};

					ReadProcessMemory(remoteId, ((BYTE*)moduleInfo.lpBaseOfDll), &DosHdr, sizeof(IMAGE_DOS_HEADER), NULL);
					ReadProcessMemory(remoteId, ((BYTE*)moduleInfo.lpBaseOfDll) + DosHdr.e_lfanew, &NtHdr, sizeof(IMAGE_NT_HEADERS), NULL);

					std::vector<IMAGE_SECTION_HEADER> sectionHeaders(NtHdr.FileHeader.NumberOfSections);
					ReadProcessMemory(remoteId, ((BYTE*)moduleInfo.lpBaseOfDll) + DosHdr.e_lfanew + sizeof(IMAGE_NT_HEADERS), sectionHeaders.data(), NtHdr.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER), NULL);

					for (auto i = 0; i < NtHdr.FileHeader.NumberOfSections; ++i)
					{
						auto&& sectionHeader = sectionHeaders[i];

						auto sectionAddress = reinterpret_cast<size_t>(moduleInfo.lpBaseOfDll) + sectionHeader.VirtualAddress;
						for (auto j = it; j != std::end(sections); ++j)
						{
							if (sectionAddress >= reinterpret_cast<size_t>(j->BaseAddress) && sectionAddress < reinterpret_cast<size_t>(j->BaseAddress) + static_cast<size_t>(j->Size))
							{
								// Copy the name because it is not null padded.
								char buffer[IMAGE_SIZEOF_SHORT_NAME + 1] = { 0 };
								std::memcpy(buffer, sectionHeader.Name, IMAGE_SIZEOF_SHORT_NAME);

								if (std::strcmp(buffer, ".text") == 0 || std::strcmp(buffer, "code") == 0)
								{
									j->Category = SectionCategory::CODE;
								}
								else if (std::strcmp(buffer, ".data") == 0 || std::strcmp(buffer, "data") == 0 || std::strcmp(buffer, ".rdata") == 0 || std::strcmp(buffer, ".idata") == 0)
								{
									j->Category = SectionCategory::DATA;
								}

								MultiByteToUnicode(buffer, j->Name, IMAGE_SIZEOF_SHORT_NAME);
								std::memcpy(j->ModulePath, modulepath, std::min(MAX_PATH, PATH_MAXIMUM_LENGTH));

								break;
							}
						}
					}
				}
			}
		}

		for (auto&& section : sections)
		{
			sectionCallback(section.BaseAddress, reinterpret_cast<RC_Pointer>(section.Size), section.Type, section.Category, section.Protection, reinterpret_cast<const WCHAR*>(section.Name), reinterpret_cast<const WCHAR*>(section.ModulePath));
		}
	}
}
//---------------------------------------------------------------------------
