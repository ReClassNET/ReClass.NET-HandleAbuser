#include <filesystem>
namespace fs = std::filesystem;
#include <process.h>

#include "PipeStream/NamedPipeServerStream.hpp"
#include "PipeStream/Exceptions.hpp"
#include "MessageClient.hpp"

std::wstring CreatePipeName()
{
	fs::path name(L"ReClass.NET_HandleAbuser");

	wchar_t szFileName[MAX_PATH];
	GetModuleFileNameW(nullptr, szFileName, MAX_PATH);
	name /= fs::path(szFileName);

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
	client.RegisterMessage<EnumerateProcessHandlesRequest>();
	client.RegisterMessage<ClosePipeRequest>();

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
		catch (InvalidOperationException&)
		{

		}
		catch (IOException&)
		{

		}
		catch (...)
		{

		}
	}
}
//---------------------------------------------------------------------------
LONG CALLBACK VectoredExceptionHandler(PEXCEPTION_POINTERS exceptionPointers)
{
	if (exceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_INVALID_HANDLE)
	{
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}
//---------------------------------------------------------------------------
BOOL WINAPI DllMain(HMODULE handle, DWORD reason, PVOID reversed)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		AddVectoredExceptionHandler(NULL, VectoredExceptionHandler);

		_beginthread(PipeThread, 0, nullptr);

		return TRUE;
	}

	if (reason == DLL_PROCESS_DETACH)
	{
		RemoveVectoredExceptionHandler(VectoredExceptionHandler);
		
		return TRUE;
	}

	return FALSE;
}
//---------------------------------------------------------------------------
