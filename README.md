# klogforward

An unimaginatively named program to read messages from /dev/kmsg and
send them to a UDP syslog server elsewhere on the network.

Intended for use as part of some log collection system in which
syslogd *writes* to /dev/kmsg instead of to local files, e.g. Busybox
with `FEATURE_KMSG_SYSLOG` enabled.  You might want to set things up
this way e.g. on a device with no local storage where you nevertheless
wish to capture boot messages and other things that happened before
the network came up or while it was down.

https://apenwarr.ca/log/20190216 is my inspiration, and a jolly
interesting read.

## How to build it

   make

## How to use it

*Before starting*, disable or uninstall any of syslogd, sysklogd,
rsyslogd or journald that you have running on the target device.  It
is possible that there may be circumstances where it makes sense to
have some of these things running together, but until you know how it
all hangs together, you're probably just going to introduce some kind
of infinite log forwarding loop.

### Capturing application logs

Applications calling the libc `syslog()` function will send their
output to `/dev/log`. We use *klogcollect* to create that socket and
forward lines (which are expected to be syslog entries in some form)
from it into the printk ring buffer by way of `/dev/kmsg`.

    klogcollect /dev/log /dev/kmsg

### Forwarding to remote hosts

*klogforward* reads log entries from `/dev/kmsg` and sends them fia
UDP to a listening syslog server

    klogforward /dev/kmsg loghost 514

For any given log entry, if the facility is `kernel` (meaning that the
log entry actually came from a kernel call to `printk`) then it is
reformatted to be (mostly if not entirely) compliant with RFC 5424,
which entails adding a timestamp, the hostname, an APP-NAME of
`kernel` and null entries for PROCID, STRUCTURED-DATA and MSGID.

If any other facility (e.g. the log entries introduced with
`klogcollect`) then they are sent unchanged.  To the best of my
knowledge, musl's `syslog()` function writes log entries in
traditional BSD format (RFC 3164).

