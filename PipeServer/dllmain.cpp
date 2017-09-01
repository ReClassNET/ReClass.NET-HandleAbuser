#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#include <process.h>

#include "PipeStream/NamedPipeServerStream.hpp"
#include "MessageClient.hpp"

std::wstring CreatePipeName()
{
	fs::path name(L"ReClass.NET_HandleAbuser");
#ifdef _WIN64
	name.append(L"x64");
#else
	name.append(L"x86");
#endif

	wchar_t szFileName[MAX_PATH];
	GetModuleFileNameW(nullptr, szFileName, MAX_PATH);
	name.append(fs::path(szFileName).filename());

	return name.wstring();
}
//---------------------------------------------------------------------------
MessageClient CreatePipeServer(NamedPipeServerStream& pipe)
{
	MessageClient client(pipe);

	client.RegisterMessage<OpenProcessRequest>();
	client.RegisterMessage<CloseProcessRequest>();
	client.RegisterMessage<IsValidRequest>();
	client.RegisterMessage<ReadMemoryRequest>();
	client.RegisterMessage<WriteMemoryRequest>();
	client.RegisterMessage<EnumerateRemoteSectionsAndModulesRequest>();

	return client;
}
//---------------------------------------------------------------------------
void PipeThread(void*)
{
	const auto name = CreatePipeName();
	
	while (true)
	{
		try
		{
			NamedPipeServerStream pipe(name, PipeDirection::InOut, 1, PipeTransmissionMode::Message);
			pipe.WaitForConnection();

			auto server = CreatePipeServer(pipe);
			while (true)
			{
				auto message = server.Receive();
				if (message != nullptr)
				{
					if (!message->Handle(server))
					{
						break;
					}
				}
			}

			pipe.Disconnect();
		}
		catch (InvalidOperationException*)
		{

		}
		catch (IOException*)
		{

		}
		catch (...)
		{

		}
	}
}
//---------------------------------------------------------------------------
BOOL WINAPI DllMain(HMODULE handle, DWORD reason, PVOID reversed)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		_beginthread(PipeThread, 0, nullptr);

		return TRUE;
	}

	return FALSE;
}
//---------------------------------------------------------------------------
