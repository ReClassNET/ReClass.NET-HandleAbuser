using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.IO;
using System.IO.Pipes;
using System.Text;

namespace HandleAbuserPlugin
{
	internal class MessageClient : IDisposable
	{
		private readonly PipeStream pipe;

		private readonly Dictionary<MessageType, Func<IMessage>> registeredMessages = new Dictionary<MessageType, Func<IMessage>>();

		public MessageClient(PipeStream pipe)
		{
			Contract.Requires(pipe != null);

			this.pipe = pipe;
		}

		public void Dispose()
		{
			pipe?.Dispose();
		}

		public void RegisterMessage<T>() where T : IMessage, new()
		{
			IMessage MessageCreator() => new T();

			registeredMessages.Add(MessageCreator().MessageType, MessageCreator);
		}

		public IMessage Receive()
		{
			using (var ms = new MemoryStream())
			{
				var buffer = new byte[256];
				do
				{
					var length = pipe.Read(buffer, 0, buffer.Length);
					ms.Write(buffer, 0, length);
				}
				while (!pipe.IsMessageComplete);

				ms.Position = 0;

				using (var br = new BinaryReader(ms, Encoding.Unicode, true))
				{
					var type = (MessageType)br.ReadInt32();

					if (registeredMessages.TryGetValue(type, out var createFn))
					{
						var message = createFn();
						message.ReadFrom(br);
						return message;
					}
				}
			}

			return null;
		}

		public void Send(IMessage message)
		{
			Contract.Requires(message != null);

			using (var ms = new MemoryStream())
			{
				using (var bw = new BinaryWriter(ms, Encoding.Unicode, true))
				{
					bw.Write((int)message.MessageType);
					message.WriteTo(bw);
				}

				var buffer = ms.ToArray();
				pipe.Write(buffer, 0, buffer.Length);
			}
		}
	}
}
