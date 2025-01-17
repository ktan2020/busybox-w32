### Status

Things may work for you, or may not.  Things may never work because of huge differences between Linux and Windows.  Or things may work in future, if you report the problem to https://github.com/rmyorston/busybox-w32.

### Building

You need a MinGW compiler and a POSIX environment (so that `make menuconfig` works).  I cross compile from Linux.  On Fedora or RHEL/CentOS+EPEL installing the mingw32-gcc package will pull in everything needed.

To start, run `make mingw32_defconfig`.  You can then customize your build with `make menuconfig`.

In particular you may need to adjust the compiler by going to Busybox Settings -> Build Options -> Cross Compiler Prefix

Then just `make`.

### Limitations

 - Use forward slashes in paths:  Windows doesn't mind and the shell will be happier.
 - Don't do wild things with Windows drive or UNC notation.
 - Wildcard expansion is disabled by default, though it can be turned on at compile time.  This only affects command line arguments to the binary:  the BusyBox shell has full support for wildcards.
 - Handling of users, groups and permissions is totally bogus.  The system only admits to knowing about the current user and always returns the same hardcoded uid, gid and permission values.
 - Some crufty old Windows code (Windows XP, cmd.exe) doesn't like forward slashes in environment variables.  The -X shell option (which must be the first argument) prevents busybox-w32 from changing backslashes to forward slashes.  If Windows programs don't run from the shell it's worth trying it.
 - Currently only 32-bit builds of BusyBox work.  If you want to install 32-bit BusyBox in a system directory on a 64-bit version of Windows you should put it in `C:\Windows\SysWOW64`, not `C:\Windows\System32`.  On 64-bit systems the latter is for 64-bit binaries.
