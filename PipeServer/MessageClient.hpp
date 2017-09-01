#pragma once

#include <unordered_map>
#include <memory>
#include <functional>

#include "PipeStream/PipeStream.hpp"
#include "Messages.hpp"

class MessageClient
{
public:
	MessageClient(PipeStream& pipe);

	template<typename T>
	void RegisterMessage()
	{
		auto MessageCreator = []() { return std::make_unique<T>(); };

		registeredMessages[MessageCreator()->GetMessageType()] = MessageCreator;
	}

	std::unique_ptr<IMessage> Receive();

	void Send(const IMessage& message) const;

private:
	PipeStream& pipe;

	std::unordered_map<MessageType, std::function<std::unique_ptr<IMessage>()>> registeredMessages;
};
