using System;
using System.Diagnostics.Contracts;
using System.IO;

namespace HandleAbuserPlugin
{
	static class Extensions
	{
		public static IntPtr ReadIntPtr(this BinaryReader br)
		{
			Contract.Requires(br != null);

#if WIN64
			return (IntPtr)br.ReadInt64();
#else
			return (IntPtr)br.ReadInt32();
#endif
		}

		public static void Write(this BinaryWriter bw, IntPtr value)
		{
			Contract.Requires(bw != null);

#if WIN64
			bw.Write(value.ToInt64());
#else
			bw.Write(value.ToInt32());
#endif
		}
	}
}
