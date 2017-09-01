using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using ReClassNET.Core;
using ReClassNET.Debugger;
using ReClassNET.Util;

namespace HandleAbuserPlugin
{
	public partial class HandleAbuserPluginExt
	{
		/// <summary>Queries if the process is valid. If the pipe isn't broken we can assume the process is valid.</summary>
		/// <remarks><![CDATA[
		/// Protocoll:
		/// -> IsValidRequest
		/// <- StatusResponse
		/// ]]></remarks>
		/// <param name="process">The process to check.</param>
		/// <returns>True if the process is valid, false if not.</returns>
		public bool IsProcessValid(IntPtr process)
		{
			lock (sync)
			{
				var mapping = GetProcessMappingById(process);
				if (mapping != null)
				{
					try
					{
						mapping.Client.Send(new IsValidRequest(mapping.RemoteId));
						var message = mapping.Client.Receive() as StatusResponse;

						return message.Success;
					}
					catch (Exception ex)
					{
						LogErrorAndRemoveClient(process, ex);
					}
				}

				return false;
			}
		}

		/// <summary>Opens the pipe to the target process.</summary>
		/// <remarks><![CDATA[
		/// Protocoll:
		/// -> OpenProcessRequest
		/// <- StatusResponse
		/// ]]></remarks>
		/// <param name="pid">The process id.</param>
		/// <param name="desiredAccess">The desired access. (ignored)</param>
		/// <returns>A plugin internal handle to the process.</returns>
		public IntPtr OpenRemoteProcess(IntPtr pid, ProcessAccess desiredAccess)
		{
			lock (sync)
			{
				var mapping = GetProcessMappingById(pid);
				if (mapping != null)
				{
					return pid;
				}

				return IntPtr.Zero;
			}
		}

		/// <summary>Closes the pipe to the remote process.</summary>
		/// <param name="process">The process to close.</param>
		public void CloseRemoteProcess(IntPtr process)
		{
			// Not needed
		}

		/// <summary>Reads memory of the remote process through the pipe.</summary>
		/// <remarks><![CDATA[
		/// Protocoll:
		/// -> ReadMemoryRequest
		/// <- StatusResponse, if an error occured
		/// <- ReadMemoryResponse, if no error occured
		/// ]]></remarks>
		/// <param name="process">The process to read from.</param>
		/// <param name="address">The address to read from.</param>
		/// <param name="buffer">The buffer to read into.</param>
		/// <param name="offset">The offset into the buffer.</param>
		/// <param name="size">The size of the memory to read.</param>
		/// <returns>True if it succeeds, false if it fails.</returns>
		public bool ReadRemoteMemory(IntPtr process, IntPtr address, ref byte[] buffer, int offset, int size)
		{
			lock (sync)
			{
				var mapping = GetProcessMappingById(process);
				if (mapping != null)
				{
					try
					{
						mapping.Client.Send(new ReadMemoryRequest(mapping.RemoteId, address, size));
						var response = mapping.Client.Receive();
						if (response is StatusResponse statusMessage)
						{
							return statusMessage.Success;
						}
						if (response is ReadMemoryResponse memoryResponse)
						{
							if (memoryResponse.Data.Length == size)
							{
								Array.Copy(memoryResponse.Data, 0, buffer, offset, size);

								return true;
							}
						}
					}
					catch (Exception ex)
					{
						LogErrorAndRemoveClient(process, ex);
					}
				}

				return false;
			}
		}

		/// <summary>Writes memory to the remote process.</summary>
		/// <remarks><![CDATA[
		/// Protocoll:
		/// -> WriteMemoryRequest
		/// <- StatusResponse
		/// ]]></remarks>
		/// <param name="process">The process to write to.</param>
		/// <param name="address">The address to write to.</param>
		/// <param name="buffer">The memory to write.</param>
		/// <param name="offset">The offset into the buffer.</param>
		/// <param name="size">The size of the memory to write.</param>
		/// <returns>True if it succeeds, false if it fails.</returns>
		public bool WriteRemoteMemory(IntPtr process, IntPtr address, ref byte[] buffer, int offset, int size)
		{
			lock (sync)
			{
				var mapping = GetProcessMappingById(process);
				if (mapping != null)
				{
					try
					{
						var data = new byte[size];
						Array.Copy(buffer, offset, data, 0, size);

						mapping.Client.Send(new WriteMemoryRequest(mapping.RemoteId, address, data));
						var message = mapping.Client.Receive() as StatusResponse;
						return message.Success;
					}
					catch (Exception ex)
					{
						LogErrorAndRemoveClient(process, ex);
					}
				}

				return false;
			}
		}

		/// <summary>Enumerates all pipes started by the ReClass.NET PipeServer.</summary>
		/// <param name="callbackProcess">The callback which gets called for every process.</param>
		public void EnumerateProcesses(EnumerateProcessCallback callbackProcess)
		{
			if (callbackProcess == null)
			{
				return;
			}

			lock (sync)
			{
				processToClientMapping.Clear();

				var validClients = new List<MessageClient>();

				foreach (var pipePath in GetPipes())
				{
					var client = GetOrCreateClientForPipe(pipePath);

					validClients.Add(client);

					try
					{
						client.Send(new EnumerateProcessHandlesRequest());

						var abusedProcessName = Path.GetFileName(pipePath);

						while (true)
						{
							var message = client.Receive();
							if (message is StatusResponse)
							{
								break;
							}

							if (message is EnumerateProcessHandlesResponse phr)
							{
								var mapping = new ProcessClientMapping(phr.RemoteId, client);
								processToClientMapping.Add(mapping.GetHashCode(), mapping);

								var targetProcessName = Path.GetFileName(phr.Path);

								var data = new EnumerateProcessData
								{
									Id = (IntPtr)mapping.GetHashCode(),
									Name = $"{abusedProcessName} -> {targetProcessName}",
									Path = phr.Path
								};

								callbackProcess(ref data);
							}
						}
					}
					catch (Exception ex)
					{
						host.Logger.Log(ex);
					}
				}

				foreach (var dormant in clients.Values.Except(validClients))
				{
					dormant.Dispose();
				}

				clients.RemoveWhere(kv => !validClients.Contains(kv.Value));
			}
		}

		/// <summary>Enumerate all sections and modules of the remote process.</summary>
		/// <remarks><![CDATA[
		/// Protocoll:
		/// -> EnumerateRemoteSectionsAndModulesRequest
		/// <- EnumerateRemoteSectionResponse [*]
		/// <- EnumerateRemoteModuleResponse [*]
		/// <- StatusResponse
		/// 
		/// Both callback messages can arrive in random order and count. The enumeration is finished if the StatusResponse was received.
		/// ]]></remarks>
		/// <param name="process">The process.</param>
		/// <param name="callbackSection">The callback which gets called for every section.</param>
		/// <param name="callbackModule">The callback which gets called for every module.</param>
		public void EnumerateRemoteSectionsAndModules(IntPtr process, EnumerateRemoteSectionCallback callbackSection, EnumerateRemoteModuleCallback callbackModule)
		{
			if (callbackSection == null && callbackModule == null)
			{
				return;
			}

			lock (sync)
			{
				var mapping = GetProcessMappingById(process);
				if (mapping != null)
				{
					try
					{
						mapping.Client.Send(new EnumerateRemoteSectionsAndModulesRequest());

						while (true)
						{
							var message = mapping.Client.Receive();
							if (message is StatusResponse)
							{
								break;
							}

							if (message is EnumerateRemoteSectionResponse callbackSectionMessage)
							{
								var data = new EnumerateRemoteSectionData
								{
									BaseAddress = callbackSectionMessage.BaseAddress,
									Size = callbackSectionMessage.Size,
									Type = callbackSectionMessage.Type,
									Category = callbackSectionMessage.Category,
									Protection = callbackSectionMessage.Protection,
									Name = callbackSectionMessage.Name,
									ModulePath = callbackSectionMessage.ModulePath
								};

								callbackSection?.Invoke(ref data);
							}
							else if (message is EnumerateRemoteModuleResponse callbackModuleMessage)
							{
								var data = new EnumerateRemoteModuleData
								{
									BaseAddress = callbackModuleMessage.BaseAddress,
									Size = callbackModuleMessage.Size,
									Path = callbackModuleMessage.Path
								};

								callbackModule?.Invoke(ref data);
							}
						}
					}
					catch (Exception ex)
					{
						LogErrorAndRemoveClient(process, ex);
					}
				}
			}
		}

		public void ControlRemoteProcess(IntPtr process, ControlRemoteProcessAction action)
		{
			// Not supported.
		}

		public bool AttachDebuggerToProcess(IntPtr id)
		{
			// Not supported.

			return false;
		}

		public void DetachDebuggerFromProcess(IntPtr id)
		{
			// Not supported.
		}

		public bool AwaitDebugEvent(ref DebugEvent evt, int timeoutInMilliseconds)
		{
			// Not supported.

			return false;
		}

		public void HandleDebugEvent(ref DebugEvent evt)
		{
			// Not supported.
		}

		public bool SetHardwareBreakpoint(IntPtr id, IntPtr address, HardwareBreakpointRegister register, HardwareBreakpointTrigger trigger, HardwareBreakpointSize size, bool set)
		{
			// Not supported.

			return false;
		}
	}
}
