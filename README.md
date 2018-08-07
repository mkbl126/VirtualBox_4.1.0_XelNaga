# VirtualBox_4.1.0_XelNaga
An implementation of hypervisor monitor via int 2e hook, based on VirtualBox 4.1.0 source code.

A research project on VirtualBox hypervisor, aimed to monitor the program behaviors in GuestOS.
This allows monitoring in the hypervisor level, without any additional hooks installed in the GuestOS.

We intercepted the old style system call manner, called via int 2e, in the hypervisor. 
So that the APis transfered from user space to kernel could be captured. 
Further more the arguments for APIs would be analyzed, extracted and resolved.
Until then, a complete API call behavior is monitored in the hyperver level, with a log emit in log files.

Mostly the monitored behaviors are Nt-like api calls, such as NtCreateThread, NtWriteFile, and so on.

Nearly all the changes are made in the file of TRPMAll.cpp, located in /src/VBox/VMM/VMMAll
Please refer to the file for more details.

Now the intercepting code cound work for a GuestOS with Windows XP or Windows 2000 installed. 
I guess the old windows systems with int 2e transfering manner would agree with that.
And the resolving code is only supported for Windows XP, since the kernel objects various in different versions of Windows.

Use the vbox_build.sh to build the VirtualBox_4.1.0_XelNaga code. 
But before that, many dependent libs should be installed. It's a tough work.

And some examples of logs are stored in the folder of Logs.

Enjoy.


