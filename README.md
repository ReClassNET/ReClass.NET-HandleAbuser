# ReClass.NET HandleAbuser Plugin
A ReClass.NET plugin which uses the process handles of an other process (the "zombie" process) to access the data of the target. This can be used to trick some anticheat systems which block the creation of memory access handles to the protected process. To use the handles of the zombie process you need to inject a dynamic library into that process. The library will host a simple server which the plugin uses to send commands to the zombie. The zombie will execute the needed operations and send back the results to the plugin.

```
ReClass.NET <--------> Zombie <--------> Target
               Pipe             Handle
```

## Installation
- Download from https://github.com/KN4CK3R/ReClass.NET-HandleAbuser/releases
- Copy the dll files in the appropriate Plugin folder (ReClass.NET/x86/Plugins or ReClass.NET/x64/Plugins)
- Start ReClass.NET and check the plugins form if the HandleAbuser plugin is listed. Open the "Native" tab and switch all available methods to the HandleAbuser plugin.
- Inject the PipeServer.dll into the target.
- Attach to the process via its pipe and use ReClass.NET as normal.

## Compiling
If you want to compile the ReClass.NET HandleAbuser Plugin just fork the repository and create the following folder structure. If you don't use this structure you need to fix the project references.

```
..\ReClass.NET\
..\ReClass.NET\ReClass.NET\ReClass.NET.csproj
..\ReClass.NET-HandleAbuserPlugin
..\ReClass.NET-HandleAbuserPlugin\Plugin
..\ReClass.NET-HandleAbuserPlugin\Plugin\HandleAbuserPlugin.csproj
```