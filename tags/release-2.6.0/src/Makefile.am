AUTOMAKE_OPTIONS=foreign

if WITH_LIFECYCLE
SUBDIRS = common message requester new_client clientv2 dataserver nameserver adminserver \
					rcserver monitor name_meta_server rootserver kv_meta_server checkserver \
					life_cycle_manager tools
else
SUBDIRS = common message requester new_client clientv2 dataserver nameserver adminserver \
					rcserver monitor name_meta_server rootserver kv_meta_server checkserver \
					tools
endif
