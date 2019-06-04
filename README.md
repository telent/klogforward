# logforward

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
