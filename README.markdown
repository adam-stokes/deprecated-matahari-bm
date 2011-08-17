# Matahari

-------------------------------------------------------------------------------

## Matahari Agents Build and Install Instructions

### Building from source

*Step 1* - Install Build Dependencies

Matahari has build dependencies on the following packages:

1. pcre-devel
2. glib2-devel
3. qmf
4. qmf-devel
5. qpid-cpp-client
6. qpid-cpp-server
7. qpid-cpp-client-devel
8. qpid-cpp-server-devel
9. sigar
10. sigar-devel

Matahari requires that the following packages are installed at runtime
for certain pieces of functionality to work:

1. puppet, version 2.6.6 or above, required for the sysconfig agent
2. dmidecode, required for the host agent

These packages may be available in your distribution.  In Fedora 14, they can
be installed via the yum command.

*Step 2* - Build Matahari

    user% make
    user% cd linux.build
    user% make
    root# make install

### Installing on Fedora 14 or later

Matahari is pre-packaged for Fedora 14 and later.

    root# yum install matahari

### Windows instructions

On your Fedora box first:

    yum install mingw32-matahari

    copy the /usr/share/matahari*/*iso to Windows machine.( or burn iso )
    load iso/cd; run setup

Please see: https://fedorahosted.org/matahari/

## If you get stuck

Join the appropriate mailing lists for help with building or installing
Matahari:

- [Matahari](https://fedorahosted.org/mailman/listinfo/matahari)
- [Cloud APIs](http://www.redhat.com/mailman/listinfo/cloud-apis)
