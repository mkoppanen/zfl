INCLUDES = -I$(top_builddir)/include
LDADD = $(top_builddir)/src/libzfl.la

noinst_PROGRAMS = test_base \
                  test_blob \
                  test_config \
                  test_config_json \
                  test_config_zpl \
                  test_device \
                  test_hash \
                  test_list \
                  test_msg \
                  test_rpc \
                  test_rpcd \
                  test_thread

test_base_SOURCES = test_base.c testutil.c
test_blob_SOURCES = test_blob.c testutil.c
test_config_SOURCES = test_config.c testutil.c
test_config_json_SOURCES = test_config_json.c testutil.c
test_config_zpl_SOURCES = test_config_zpl.c testutil.c
test_device_SOURCES = test_device.c testutil.c
test_hash_SOURCES = test_hash.c testutil.c
test_list_SOURCES = test_list.c testutil.c
test_msg_SOURCES = test_msg.c testutil.c
test_rpc_SOURCES = test_rpc.c testutil.c
test_rpcd_SOURCES = test_rpcd.c testutil.c
test_thread_SOURCES = test_thread.c testutil.c

TESTS = $(noinst_PROGRAMS)
