#pragma once

#include <string>

#include "ReClassNET_Plugin.hpp"

#include "PipeStream/BinaryReader.hpp"
#include "PipeStream/BinaryWriter.hpp"

class MessageClient;

enum class MessageType
{
	StatusResponse = 1,
	OpenProcessRequest = 2,
	CloseProcessRequest = 3,
	IsValidRequest = 4,
	ReadMemoryRequest = 5,
	ReadMemoryResponse = 6,
	WriteMemoryRequest = 7,
	EnumerateRemoteSectionsAndModulesRequest = 8,
	EnumerateRemoteSectionResponse = 9,
	EnumerateRemoteModuleResponse = 10,
	EnumerateProcessHandlesRequest = 11,
	EnumerateProcessHandlesResponse = 12,
	ClosePipeRequest = 13
};
//---------------------------------------------------------------------------
class IMessage
{
public:
	virtual ~IMessage() = default;

	virtual MessageType GetMessageType() const = 0;

	virtual void ReadFrom(BinaryReader& br) = 0;
	virtual void WriteTo(BinaryWriter& bw) const = 0;

	virtual bool Handle(MessageClient& client)
	{
		return true;
	}
};
//---------------------------------------------------------------------------
class StatusResponse : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::StatusResponse; }

	RC_Pointer GetRemoteId() const { return remoteId; }
	bool GetSuccess() const { return success; }

	StatusResponse() = default;

	StatusResponse(RC_Pointer _remoteId, bool _success)
		: remoteId(_remoteId),
		  success(_success)
	{

	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
		success = reader.ReadBoolean();
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
		writer.Write(success);
	}

private:
	RC_Pointer remoteId = nullptr;
	bool success = false;
};
//---------------------------------------------------------------------------
class OpenProcessRequest : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::OpenProcessRequest; }

	RC_Pointer GetRemoteId() const { return remoteId; }

	OpenProcessRequest() = default;

	OpenProcessRequest(RC_Pointer _remoteId)
		: remoteId(_remoteId)
	{
		
	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
	}

	bool Handle(MessageClient& client) override;

private:
	RC_Pointer remoteId = nullptr;
};
//---------------------------------------------------------------------------
class CloseProcessRequest : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::CloseProcessRequest; }

	RC_Pointer GetRemoteId() const { return remoteId; }

	CloseProcessRequest() = default;

	CloseProcessRequest(RC_Pointer _remoteId)
		: remoteId(_remoteId)
	{
		
	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
	}

	bool Handle(MessageClient& client) override;

private:
	RC_Pointer remoteId = nullptr;
};
//---------------------------------------------------------------------------
class IsValidRequest : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::IsValidRequest; }

	RC_Pointer GetRemoteId() const { return remoteId; }

	IsValidRequest() = default;

	IsValidRequest(RC_Pointer _remoteId)
		: remoteId(_remoteId)
	{
		
	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
	}

	bool Handle(MessageClient& client) override;

private:
	RC_Pointer remoteId = nullptr;
};
//---------------------------------------------------------------------------
class ReadMemoryRequest : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::ReadMemoryRequest; }

	RC_Pointer GetRemoteId() const { return remoteId; }
	RC_Pointer GetAddress() const { return address; }
	int GetSize() const { return size; }

	ReadMemoryRequest() = default;

	ReadMemoryRequest(RC_Pointer _remoteId, RC_Pointer _address, int _size)
		: remoteId(_remoteId),
		  address(_address),
		  size(_size)
	{

	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
		address = reader.ReadIntPtr();
		size = reader.ReadInt32();
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
		writer.Write(address);
		writer.Write(size);
	}

	bool Handle(MessageClient& client) override;

private:
	RC_Pointer remoteId = nullptr;
	RC_Pointer address = nullptr;
	int size = 0;
};
//---------------------------------------------------------------------------
class ReadMemoryResponse : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::ReadMemoryResponse; }

	RC_Pointer GetRemoteId() const { return remoteId; }
	const std::vector<uint8_t>& GetData() const { return data; }

	ReadMemoryResponse() = default;

	ReadMemoryResponse(RC_Pointer _remoteId, std::vector<uint8_t>&& _data)
		: remoteId(_remoteId),
		  data(std::move(_data))
	{

	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
		const auto size = reader.ReadInt32();
		data = reader.ReadBytes(size);
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
		writer.Write(static_cast<int>(data.size()));
		writer.Write(data.data(), 0, static_cast<int>(data.size()));
	}

private:
	RC_Pointer remoteId = nullptr;
	std::vector<uint8_t> data;
};
//---------------------------------------------------------------------------
class WriteMemoryRequest : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::WriteMemoryRequest; }

	RC_Pointer GetRemoteId() const { return remoteId; }
	RC_Pointer GetAddress() const { return address; }
	const std::vector<uint8_t>& GetData() const { return data; }

	WriteMemoryRequest() = default;

	WriteMemoryRequest(RC_Pointer _remoteId, RC_Pointer _address, std::vector<uint8_t>&& _data)
		: remoteId(_remoteId),
		  address(_address),
		  data(std::move(_data))
	{

	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
		address = reader.ReadIntPtr();
		const auto size = reader.ReadInt32();
		data = reader.ReadBytes(size);
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
		writer.Write(address);
		writer.Write(static_cast<int>(data.size()));
		writer.Write(data.data(), 0, static_cast<int>(data.size()));
	}

	bool Handle(MessageClient& client) override;

private:
	RC_Pointer remoteId = nullptr;
	RC_Pointer address = nullptr;
	std::vector<uint8_t> data;
};
//---------------------------------------------------------------------------
class EnumerateRemoteSectionsAndModulesRequest : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::EnumerateRemoteSectionsAndModulesRequest; }

	RC_Pointer GetRemoteId() const { return remoteId; }

	EnumerateRemoteSectionsAndModulesRequest() = default;

	EnumerateRemoteSectionsAndModulesRequest(RC_Pointer _remoteId)
		: remoteId(_remoteId)
	{
		
	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
	}

	bool Handle(MessageClient& client) override;

private:
	RC_Pointer remoteId = nullptr;
};
//---------------------------------------------------------------------------
class EnumerateRemoteSectionResponse : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::EnumerateRemoteSectionResponse; }

	RC_Pointer GetRemoteId() const { return remoteId; }
	RC_Pointer GetBaseAddress() const { return baseAddress; }
	RC_Pointer GetRegionSize() const { return size; }
	SectionType GetType() const { return type; }
	SectionCategory GetCategory() const { return category; }
	SectionProtection GetProtection() const { return protection; }
	const std::wstring& GetName() const { return name; }
	const std::wstring& GetModulePath() const { return modulePath; }

	EnumerateRemoteSectionResponse() = default;

	EnumerateRemoteSectionResponse(RC_Pointer _remoteId, RC_Pointer _baseAddress, RC_Pointer _size, SectionType _type, SectionCategory _category, SectionProtection _protection, std::wstring&& _name, std::wstring&& _modulePath)
		: remoteId(_remoteId),
		  baseAddress(_baseAddress),
		  size(_size),
		  type(_type),
		  category(_category),
		  protection(_protection),
		  name(std::move(_name)),
		  modulePath(std::move(_modulePath))
	{

	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
		baseAddress = reader.ReadIntPtr();
		size = reader.ReadIntPtr();
		type = static_cast<SectionType>(reader.ReadInt32());
		category = static_cast<SectionCategory>(reader.ReadInt32());
		protection = static_cast<SectionProtection>(reader.ReadInt32());
		name = reader.ReadString();
		modulePath = reader.ReadString();
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
		writer.Write(baseAddress);
		writer.Write(size);
		writer.Write(static_cast<int>(type));
		writer.Write(static_cast<int>(category));
		writer.Write(static_cast<int>(protection));
		writer.Write(name);
		writer.Write(modulePath);
	}

private:
	RC_Pointer remoteId = nullptr;
	RC_Pointer baseAddress = nullptr;
	RC_Pointer size = nullptr;
	SectionType type = SectionType::Unknown;
	SectionCategory category = SectionCategory::Unknown;
	SectionProtection protection = SectionProtection::NoAccess;
	std::wstring name;
	std::wstring modulePath;
};
//---------------------------------------------------------------------------
class EnumerateRemoteModuleResponse : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::EnumerateRemoteModuleResponse; }

	RC_Pointer GetRemoteId() const { return remoteId; }
	RC_Pointer GetBaseAddress() const { return baseAddress; }
	RC_Pointer GetRegionSize() const { return size; }
	const std::wstring& GetModulePath() const { return modulePath; }

	EnumerateRemoteModuleResponse() = default;

	EnumerateRemoteModuleResponse(RC_Pointer _remoteId, RC_Pointer _baseAddress, RC_Pointer _regionSize, std::wstring&& _modulePath)
		: remoteId(_remoteId),
		  baseAddress(_baseAddress),
		  size(_regionSize),
		  modulePath(std::move(_modulePath))
	{

	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
		baseAddress = reader.ReadIntPtr();
		size = reader.ReadIntPtr();
		modulePath = reader.ReadString();
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
		writer.Write(baseAddress);
		writer.Write(size);
		writer.Write(modulePath);
	}

private:
	RC_Pointer remoteId = nullptr;
	RC_Pointer baseAddress = nullptr;
	RC_Pointer size = nullptr;
	std::wstring modulePath;
};
//---------------------------------------------------------------------------
class EnumerateProcessHandlesRequest : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::EnumerateProcessHandlesRequest; }

	void ReadFrom(BinaryReader& reader) override
	{
		
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		
	}

	bool Handle(MessageClient& client) override;
};
//---------------------------------------------------------------------------
class EnumerateProcessHandlesResponse : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::EnumerateProcessHandlesResponse; }

	RC_Pointer GetRemoteId() const { return remoteId; }
	const std::wstring& GetPath() const { return path; }

	EnumerateProcessHandlesResponse() = default;

	EnumerateProcessHandlesResponse(RC_Pointer _remoteId, std::wstring&& _path)
		: remoteId(_remoteId),
		  path(_path)
	{

	}

	void ReadFrom(BinaryReader& reader) override
	{
		remoteId = reader.ReadIntPtr();
		path = reader.ReadString();
	}

	void WriteTo(BinaryWriter& writer) const override
	{
		writer.Write(remoteId);
		writer.Write(path);
	}

private:
	RC_Pointer remoteId = nullptr;
	std::wstring path;
};
//---------------------------------------------------------------------------
class ClosePipeRequest : public IMessage
{
public:
	MessageType GetMessageType() const override { return MessageType::ClosePipeRequest; }

	void ReadFrom(BinaryReader& reader) override
	{

	}

	void WriteTo(BinaryWriter& writer) const override
	{

	}

	bool Handle(MessageClient& client) override { return false; }
};
//---------------------------------------------------------------------------
// 