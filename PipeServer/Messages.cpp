#include "Messages.hpp"
#include "MessageClient.hpp"

#include <psapi.h>

extern std::vector<RC_Pointer> GetAvailableHandles(DWORD desiredAccess);
extern void EnumerateRemoteSectionsAndModules(RC_Pointer remoteId, const std::function<void(RC_Pointer, RC_Pointer, std::wstring&&)>&, const std::function<void(RC_Pointer, RC_Pointer, SectionType, SectionCategory, SectionProtection, std::wstring&&, std::wstring&&)>&);

bool OpenProcessRequest::Handle(MessageClient& client)
{
	client.Send(StatusResponse(GetRemoteId(), true));

	return true;
}
//---------------------------------------------------------------------------
bool CloseProcessRequest::Handle(MessageClient& client)
{
	client.Send(StatusResponse(GetRemoteId(), true));

	return true;
}
//---------------------------------------------------------------------------
bool IsValidRequest::Handle(MessageClient& client)
{
	const auto success = WaitForSingleObject(GetRemoteId(), 0) == WAIT_TIMEOUT;

	client.Send(StatusResponse(GetRemoteId(), success));

	return true;
}
//---------------------------------------------------------------------------
bool ReadMemoryRequest::Handle(MessageClient& client)
{
	std::vector<uint8_t> buffer(GetSize());
	buffer.resize(GetSize());

	SIZE_T numberOfBytesRead;
	if (ReadProcessMemory(GetRemoteId(), GetAddress(), buffer.data(), buffer.size(), &numberOfBytesRead) && numberOfBytesRead == buffer.size())
	{
		client.Send(ReadMemoryResponse(GetRemoteId(), std::move(buffer)));
	}
	else
	{
		client.Send(StatusResponse(GetRemoteId(), false));
	}

	return true;
}
//---------------------------------------------------------------------------
bool WriteMemoryRequest::Handle(MessageClient& client)
{
	const auto& data = GetData();

	SIZE_T numberOfBytesWritten;
	const auto success = WriteProcessMemory(GetRemoteId(), GetAddress(), data.data(), data.size(), &numberOfBytesWritten) && numberOfBytesWritten == data.size();

	client.Send(StatusResponse(GetRemoteId(), success));

	return true;
}
//---------------------------------------------------------------------------
bool EnumerateRemoteSectionsAndModulesRequest::Handle(MessageClient& client)
{
	EnumerateRemoteSectionsAndModules(
		GetRemoteId(),
		[&](auto p1, auto p2, auto p3) { client.Send(EnumerateRemoteModuleResponse(GetRemoteId(), p1, p2, std::move(p3))); },
		[&](auto p1, auto p2, auto p3, auto p4, auto p5, auto p6, auto p7) { client.Send(EnumerateRemoteSectionResponse(GetRemoteId(), p1, p2, p3, p4, p5, std::move(p6), std::move(p7))); }
	);
	
	// Report enumeration complete to client.
	client.Send(StatusResponse(GetRemoteId(), true));

	return true;
}
//---------------------------------------------------------------------------
bool EnumerateProcessHandlesRequest::Handle(MessageClient& client)
{
	auto handles = GetAvailableHandles(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE);

	for (auto handle : handles)
	{
		std::wstring path;
		path.resize(300);

		GetModuleFileNameExW(handle, nullptr, const_cast<LPWSTR>(path.data()), 300);

		client.Send(EnumerateProcessHandlesResponse(handle, std::move(path)));
	}

	return true;
}
//---------------------------------------------------------------------------
