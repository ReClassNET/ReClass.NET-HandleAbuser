#include "MessageClient.hpp"
#include "PipeStream/MemoryStream.hpp"

MessageClient::MessageClient(PipeStream& _pipe)
	: pipe(_pipe)
{

}
//---------------------------------------------------------------------------
std::unique_ptr<IMessage> MessageClient::Receive()
{
	MemoryStream ms;
	std::vector<uint8_t> buffer(256);
	do
	{
		const auto length = pipe.Read(buffer.data(), 0, static_cast<int>(buffer.size()));
		ms.Write(buffer.data(), 0, length);
	} while (!pipe.IsMessageComplete());

	ms.SetPosition(0);

	BinaryReader br(ms);
	const auto type = static_cast<MessageType>(br.ReadInt32());

	const auto it = registeredMessages.find(type);
	if (it != std::end(registeredMessages))
	{
		auto message = it->second();
		message->ReadFrom(br);

		return message;
	}

	return nullptr;
}
//---------------------------------------------------------------------------
void MessageClient::Send(const IMessage& message) const
{
	MemoryStream ms;
	BinaryWriter bw(ms);

	bw.Write(static_cast<int>(message.GetMessageType()));
	message.WriteTo(bw);

	auto data = ms.ToArray();
	pipe.Write(data.data(), 0, static_cast<int>(data.size()));
}
//---------------------------------------------------------------------------
