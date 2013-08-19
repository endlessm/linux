all: hv_kvp_daemon hv_vss_daemon

hv_kvp_daemon: hv_kvp_daemon.c
hv_vss_daemon: hv_vss_daemon.c

clean:
	rm -f  hv_kvp_daemon hv_vss_daemon

install:
	install hv_kvp_daemon /usr/sbin/
	install hv_vss_daemon /usr/sbin/
