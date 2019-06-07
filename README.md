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

    klogcollect /dev/log /dev/kmsg

This will read datagrams from `/dev/log`, which are expected to be
syslog entries in some form, and send them into the printk ring buffer
by way of `/dev/kmsg`.

    klogforward /dev/kmsg loghost 514

This will read log entries from `/dev/kmsg` and send them to a
listening UDP server at `loghost` port 514.

For any given log entry, if the facility is `kernel` (meaning that the
log entry actually came from a kernel call to `printk`) then they are
reformatted to be (mostly if not entirely) compliant with RFC 5424,
which entails adding a timestamp, the hostname, an APP-NAME of
`kernel` and null entries for PROCID, STRUCTURED-DATA and MSGID.  If
any other facility (e.g. the log entries introduced with `klogcollect`)
then they are sent unchanged.  To the best of my knowledge, musl's
`syslog()` function writes log entries in traditional BSD format (RFC
3164).

