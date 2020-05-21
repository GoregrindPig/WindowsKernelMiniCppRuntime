This is a small C++ runtime for Windows kernel modules incorporated into several commercial projects.

A C language is not only choise for Windows kernel module development. In contrast to thier Linux counterparts, the Windows drivers could be written in C++ since ages. In Windows kernel however there is no centralized C++ runtime support. Back in early and mid 2000th Compuware Driver Studio company used to provided such runtime but they don't anymore. Nowadays the implementations might be different from one solution to another. Herein I would like to provide my own solution compiled as static library. It can be used from either Visual Studio IDE or WDK build utility. Currently it's not complied to C++11 or higher standards because Windows Vista and Windows 7 WDK tools don't support it.

A runtime organized as a static library.
In the Additional Incude Directory of VS project option we might see following statement "$(WDKROOT)\inc\ddk;$(WDKROOT)\inc\crt;$(WDKROOT)\inc\api". So the WDKROOT environment variable shall be set to make compilation work properly. 
For this following steps required
I recommend using System applet of Windows Control Panel. Here are the steps to follow
1. Move to Start Menu -> Control Panel -> System applet.
2. Choose Advanced System Settings option.
3. In System Properties dialog select Advanced tab.
4. Press Environment Variables... button to activate Environment Variables modal dialog.
5. Press New.. button in the System Variables section.
6. In the New System Variable dialog specify WDKROOT variable name and a variable path corresponding to root path of your WDK, e.g. D:\WinDDK\7600.16385.1. Press OK button.
