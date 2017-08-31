using System;
using System.Diagnostics.Contracts;

namespace HandleAbuserPlugin
{
	internal class ProcessClientMapping
	{
		public IntPtr RemoteId { get; }
		public MessageClient Client { get; }

		public ProcessClientMapping(IntPtr remoteId, MessageClient client)
		{
			Contract.Requires(client != null);

			RemoteId = remoteId;
			Client = client;
		}

		public override bool Equals(object obj)
		{
			if (obj == null) return false;
			return obj.GetHashCode() == GetHashCode();
		}

		public override int GetHashCode()
		{
			return (269 + RemoteId.GetHashCode()) * 47 + Client.GetHashCode();
		}
	}
}
