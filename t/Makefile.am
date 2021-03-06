AM_CFLAGS = \
	$(WARNING_CFLAGS) \
	$(CODE_COVERAGE_CFLAGS)

AM_LDFLAGS = \
	$(CODE_COVERAGE_LIBS)

AM_CPPFLAGS = \
        -I$(top_srcdir) \
	-I$(top_srcdir)/src/include \
	-I$(top_builddir)/src/common/libflux \
        $(ZMQ_CFLAGS)

#  Always set LUA_PATH such that builddir/?.lua is first so that the
#   build version of fluxometer.lua is found.
#
#  If we're testing with "installed" Flux, then also add path to Lua bindings
#   to LUA_CPATH, so that non-installed test modules can be found (at this
#   time, only lalarm.so)
#
AM_TESTS_ENVIRONMENT = \
	export LUA_PATH="$(builddir)/?.lua;$$LUA_PATH;;;";\
	test -n "$$FLUX_TEST_INSTALLED_PATH" && \
	export LUA_CPATH="$(abs_top_builddir)/src/bindings/lua/.libs/?.so;$$LUA_CPATH;;"; \
	export PYTHONPATH="$(abs_top_builddir)/src/bindings/python:$(abs_top_srcdir)/src/bindings/python:$(top_srcdir)/t/python/tap:$(PYTHON_SITE_PKG):$$PYTHONPATH";

TEST_EXTENSIONS = .t .py
T_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
        $(top_srcdir)/config/tap-driver.sh
PY_LOG_DRIVER = $(PYTHON) $(top_srcdir)/config/tap-driver.py

lua_SCRIPTS = \
        fluxometer.lua

install-data-local:
	$(INSTALL) -m644 fluxometer/conf.lua.installed \
		$(DESTDIR)$(luadir)/fluxometer/conf.lua
uninstall-local:
	$(RM) $(DESTDIR)$(luadir)/fluxometer/conf.lua

clean-local:
	rm -fr trash-directory.* test-results .prove *.broker.log */*.broker.log *.output python/__pycache__

#  Put long running tests here. They will be included first
#   in TESTS so that `make check` runs them first, hopefully resulting
#   in a reduced makespan overall.
LONGTESTSCRIPTS = \
	t5000-valgrind.t \
	t3100-flux-in-flux.t

# This list is included in both TESTS and dist_check_SCRIPTS.
TESTSCRIPTS = \
	$(LONGTESTSCRIPTS) \
	t0000-sharness.t \
	t0001-basic.t \
	t0002-request.t \
	t0003-module.t \
	t0004-event.t \
	t0005-exec.t \
	t0005-rexec.t \
	t0007-ping.t \
	t0008-attr.t \
	t0009-dmesg.t \
	t0010-generic-utils.t \
	t0011-content-cache.t \
	t0012-content-sqlite.t \
	t0013-config-file.t \
	t0014-runlevel.t \
	t0015-cron.t \
	t0016-cron-faketime.t \
	t0017-security.t \
	t0019-jobspec-schema.t \
	t0021-flux-jobspec.t \
	t0022-jj-reader.t \
	t1000-kvs.t \
	t1001-kvs-internals.t \
	t1003-kvs-stress.t \
	t1004-kvs-namespace.t \
	t1005-kvs-security.t \
	t1007-kvs-lookup-watch.t \
	t1008-kvs-eventlog.t \
	t1009-kvs-copy.t \
	t1101-barrier-basic.t \
	t1102-cmddriver.t \
	t1103-apidisconnect.t \
	t1105-proxy.t \
	t1106-ssh-connector.t \
	t2004-hydra.t \
	t2005-hwloc-basic.t \
	t2007-caliper.t \
	t2008-althash.t \
	t2010-kvs-snapshot-restore.t \
	t2100-aggregate.t \
	t2200-job-ingest.t \
	t2201-job-cmd.t \
	t2202-job-manager.t \
	t2203-job-manager-dummysched.t \
	t2204-job-info.t \
	t2205-job-info-security.t \
	t2206-job-manager-bulk-state.t \
	t2207-job-manager-wait.t \
	t2208-queue-cmd.t \
	t2210-job-manager-bugs.t \
	t2300-sched-simple.t \
	t2301-schedutil-outstanding-requests.t \
	t2400-job-exec-test.t \
	t2401-job-exec-hello.t \
	t2402-job-exec-dummy.t \
	t2403-job-exec-conf.t \
	t2500-job-attach.t \
	t2501-job-status.t \
	t2600-job-shell-rcalc.t \
	t2601-job-shell-standalone.t \
	t2602-job-shell.t \
	t2603-job-shell-initrc.t \
	t2604-job-shell-affinity.t \
	t2605-job-shell-output-redirection-standalone.t \
	t2606-job-shell-output-redirection.t \
	t2607-job-shell-input.t \
	t2608-job-shell-log.t \
	t2609-job-shell-events.t \
	t2610-job-shell-mpir.t \
	t2700-mini-cmd.t \
	t2800-jobs-cmd.t \
	t3000-mpi-basic.t \
	t3001-mpi-personalities.t \
	t4000-issues-test-driver.t \
	t9001-pymod.t \
	lua/t0001-send-recv.t \
	lua/t0002-rpc.t \
	lua/t0003-events.t \
	lua/t0004-getattr.t \
	lua/t0007-alarm.t \
	lua/t1000-reactor.t \
	lua/t1001-timeouts.t \
	lua/t1004-statwatcher.t \
	python/t0001-handle.py \
	python/t0002-wrapper.py \
	python/t0003-barrier.py \
	python/t0004-event.py \
	python/t0005-kvs.py \
	python/t0006-request.py \
	python/t0007-watchers.py \
	python/t0010-job.py \
	python/t0012-futures.py \
	python/t1000-service-add-remove.py

if HAVE_FLUX_SECURITY
TESTSCRIPTS += python/t0009-security.py
endif

# make check runs these TAP tests directly (both scripts and programs)
TESTS = \
	shmem/backtoback.t \
	$(TESTSCRIPTS)

EXTRA_DIST= \
	aggregate-results.sh \
	sharness.sh \
	sharness.d \
	$(T) \
	test-under-flux/expected.modcheck \
	test-under-flux/t_modcheck.t \
	test-under-flux/test.t \
	rc/rc1-kvs \
	rc/rc1-testenv \
	rc/rc1-job \
	rc/rc3-kvs \
	rc/rc3-testenv \
	rc/rc3-job \
	shell/input \
	shell/output \
	shell/initrc/tests \
	flux-jobs/tests \
	jobspec

dist_check_SCRIPTS = \
	$(TESTSCRIPTS) \
        issues/t0441-kvs-put-get.sh \
        issues/t0505-msg-handler-reg.lua \
        issues/t0821-kvs-segfault.sh \
	issues/t1760-kvs-use-after-free.sh \
	issues/t2281-service-add-crash.sh \
	issues/t2284-initial-program-format-chars.sh \
	issues/t2686-shell-input-race.sh \
	python/__init__.py \
	python/subflux.py \
	python/tap \
	scripts/event-trace.lua \
	scripts/event-trace-bypass.lua \
	scripts/waitfile.lua \
	scripts/t0004-event-helper.sh \
	scripts/tssh \
	valgrind/valgrind-workload.sh \
	valgrind/workload.d/job \
	kvs/kvs-helper.sh \
	job-manager/exec-service.lua \
	job-manager/drain-cancel.py \
	job-manager/bulk-state.py \
	job-manager/submit-wait.py \
	job-manager/submit-waitany.py \
	job-manager/submit-sliding-window.py \
	job-manager/wait-interrupted.py \
	job-attach/outputsleep.sh \
	job-exec/dummy.sh \
	job-info/list-id.py \
	schedutil/req_and_unload.py \
	ingest/fake-validate.sh \
	ingest/bad-validate.sh

check_PROGRAMS = \
	shmem/backtoback.t \
	loop/logstderr \
	loop/issue2337 \
	loop/issue2711 \
	kvs/torture \
	kvs/dtree \
	kvs/blobref \
	kvs/hashtest \
	kvs/watch_disconnect \
	kvs/commit \
	kvs/fence_api \
	kvs/transactionmerge \
	kvs/fence_namespace_remove \
	kvs/fence_invalid \
	kvs/lookup_invalid \
	kvs/commit_order \
	kvs/issue1760 \
	kvs/issue1876 \
	kvs/waitcreate_cancel \
	kvs/setrootevents \
	kvs/checkpoint \
	request/treq \
	request/rpc \
	barrier/tbarrier \
	reactor/reactorcat \
	rexec/rexec \
	rexec/rexec_ps \
	rexec/rexec_count_stdout \
	rexec/rexec_getline \
	job-manager/list-jobs \
	ingest/submitbench \
	sched-simple/jj-reader \
	shell/rcalc \
	shell/lptest \
	shell/mpir

if HAVE_MPI
check_PROGRAMS += \
	mpi/hello
endif

check_LTLIBRARIES = \
	module/parent.la \
	module/child.la \
	request/req.la \
	ingest/job-manager-dummy.la \
	job-manager/sched-dummy.la \
	shell/plugins/dummy.la \
	shell/plugins/conftest.la \
	shell/plugins/invalid-args.la \
	shell/plugins/getopt.la \
	shell/plugins/log.la \
	shell/plugins/test-event.la

dist_check_DATA = \
	hwloc-data/sierra2/0.xml \
	hwloc-data/sierra2/1.xml \
	hwloc-data/1N/shared/02-brokers/0.xml \
	hwloc-data/1N/shared/02-brokers/1.xml \
	hwloc-data/1N/nonoverlapping/02-brokers/0.xml \
	hwloc-data/1N/nonoverlapping/02-brokers/1.xml \
	valgrind/valgrind.supp \
	conf.d/bad-hosts/boot.toml \
	conf.d/bad-hosts2/boot.toml \
	conf.d/bad-nobootstrap/boot.toml \
	conf.d/bad-nomatch/boot.toml \
	conf.d/bad-toml/boot.toml \
	conf.d/good-nohosts/boot.toml \
	conf.d/good-emptyhosts/boot.toml \
	conf.d/good-ipc2/boot.toml \
	conf.d/good-tcp4/boot.toml

test_ldadd = \
        $(top_builddir)/src/common/libflux-internal.la \
        $(top_builddir)/src/common/libflux-core.la \
        $(top_builddir)/src/common/libtap/libtap.la \
	$(top_builddir)/src/common/libflux-optparse.la \
        $(ZMQ_LIBS) $(LIBPTHREAD)

test_cppflags = \
        -I$(top_srcdir)/src/common/libtap \
        $(AM_CPPFLAGS)

shmem_backtoback_t_SOURCES = shmem/backtoback.c
shmem_backtoback_t_CPPFLAGS = $(test_cppflags)
shmem_backtoback_t_LDADD = $(test_ldadd) $(LIBDL)

loop_logstderr_SOURCES = loop/logstderr.c
loop_logstderr_CPPFLAGS = $(test_cppflags)
loop_logstderr_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

loop_issue2337_SOURCES = loop/issue2337.c
loop_issue2337_CPPFLAGS = $(test_cppflags)
loop_issue2337_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

loop_issue2711_SOURCES = loop/issue2711.c
loop_issue2711_CPPFLAGS = $(test_cppflags)
loop_issue2711_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

mpi_hello_SOURCES = mpi/hello.c
mpi_hello_CPPFLAGS = $(MPI_CFLAGS)
mpi_hello_LDADD = $(MPI_CLDFLAGS) $(LIBRT)

kvs_torture_SOURCES = kvs/torture.c
kvs_torture_CPPFLAGS = $(test_cppflags)
kvs_torture_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_dtree_SOURCES = kvs/dtree.c
kvs_dtree_CPPFLAGS = $(test_cppflags)
kvs_dtree_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_blobref_SOURCES = kvs/blobref.c
kvs_blobref_CPPFLAGS = $(test_cppflags)
kvs_blobref_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_commit_SOURCES = kvs/commit.c
kvs_commit_CPPFLAGS = $(test_cppflags)
kvs_commit_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_fence_api_SOURCES = kvs/fence_api.c
kvs_fence_api_CPPFLAGS = $(test_cppflags)
kvs_fence_api_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_transactionmerge_SOURCES = kvs/transactionmerge.c
kvs_transactionmerge_CPPFLAGS = $(test_cppflags)
kvs_transactionmerge_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_fence_namespace_remove_SOURCES = kvs/fence_namespace_remove.c
kvs_fence_namespace_remove_CPPFLAGS = $(test_cppflags)
kvs_fence_namespace_remove_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_fence_invalid_SOURCES = kvs/fence_invalid.c
kvs_fence_invalid_CPPFLAGS = $(test_cppflags)
kvs_fence_invalid_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_lookup_invalid_SOURCES = kvs/lookup_invalid.c
kvs_lookup_invalid_CPPFLAGS = $(test_cppflags)
kvs_lookup_invalid_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_commit_order_SOURCES = kvs/commit_order.c
kvs_commit_order_CPPFLAGS = $(test_cppflags)
kvs_commit_order_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_watch_disconnect_SOURCES = kvs/watch_disconnect.c
kvs_watch_disconnect_CPPFLAGS = $(test_cppflags)
kvs_watch_disconnect_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_hashtest_SOURCES = kvs/hashtest.c
kvs_hashtest_CPPFLAGS = $(test_cppflags) $(SQLITE_CFLAGS)
kvs_hashtest_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL) $(LIBJUDY) $(SQLITE_LIBS)

kvs_issue1760_SOURCES = kvs/issue1760.c
kvs_issue1760_CPPFLAGS = $(test_cppflags)
kvs_issue1760_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_issue1876_SOURCES = kvs/issue1876.c
kvs_issue1876_CPPFLAGS = $(test_cppflags)
kvs_issue1876_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_waitcreate_cancel_SOURCES = kvs/waitcreate_cancel.c
kvs_waitcreate_cancel_CPPFLAGS = $(test_cppflags)
kvs_waitcreate_cancel_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_setrootevents_SOURCES = kvs/setrootevents.c
kvs_setrootevents_CPPFLAGS = $(test_cppflags)
kvs_setrootevents_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

kvs_checkpoint_SOURCES = kvs/checkpoint.c
kvs_checkpoint_CPPFLAGS = $(test_cppflags)
kvs_checkpoint_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

request_treq_SOURCES = request/treq.c
request_treq_CPPFLAGS = $(test_cppflags)
request_treq_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

request_rpc_SOURCES = request/rpc.c
request_rpc_CPPFLAGS = $(test_cppflags)
request_rpc_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

module_parent_la_SOURCES = module/parent.c
module_parent_la_CPPFLAGS = $(test_cppflags)
module_parent_la_LDFLAGS = $(fluxmod_ldflags) -module -rpath /nowher
module_parent_la_LIBADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

module_child_la_SOURCES = module/child.c
module_child_la_CPPFLAGS = $(test_cppflags)
module_child_la_LDFLAGS = $(fluxmod_ldflags) -module -rpath /nowher
module_child_la_LIBADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

barrier_tbarrier_SOURCES = barrier/tbarrier.c
barrier_tbarrier_CPPFLAGS = $(test_cppflags)
barrier_tbarrier_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

request_req_la_SOURCES = request/req.c
request_req_la_CPPFLAGS = $(test_cppflags)
request_req_la_LDFLAGS = $(fluxmod_ldflags) -module -rpath /nowher
request_req_la_LIBADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

shell_rcalc_SOURCES = shell/rcalc.c
shell_rcalc_CPPFLAGS = $(test_cppflags)
shell_rcalc_LDADD = \
        $(top_builddir)/src/shell/libshell.la \
        $(test_ldadd) $(LIBDL) $(LIBUTIL)

shell_lptest_SOURCES = shell/lptest.c
shell_lptest_CPPFLAGS = $(test_cppflags)
shell_lptest_LDADD = \
        $(test_ldadd) $(LIBDL) $(LIBUTIL)

shell_mpir_SOURCES = shell/mpir.c
shell_mpir_CPPFLAGS = $(test_cppflags)
shell_mpir_LDADD = \
	$(top_builddir)/src/shell/libmpir.la \
        $(test_ldadd) $(LIBDL) $(LIBUTIL)

reactor_reactorcat_SOURCES = reactor/reactorcat.c
reactor_reactorcat_CPPFLAGS = $(test_cppflags)
reactor_reactorcat_LDADD = \
	 $(test_ldadd) $(LIBDL) $(LIBUTIL)

rexec_rexec_SOURCES = rexec/rexec.c
rexec_rexec_CPPFLAGS = $(test_cppflags)
rexec_rexec_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

rexec_rexec_ps_SOURCES = rexec/rexec_ps.c
rexec_rexec_ps_CPPFLAGS = $(test_cppflags)
rexec_rexec_ps_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

rexec_rexec_count_stdout_SOURCES = rexec/rexec_count_stdout.c
rexec_rexec_count_stdout_CPPFLAGS = $(test_cppflags)
rexec_rexec_count_stdout_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

rexec_rexec_getline_SOURCES = rexec/rexec_getline.c
rexec_rexec_getline_CPPFLAGS = $(test_cppflags)
rexec_rexec_getline_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

ingest_job_manager_dummy_la_SOURCES = ingest/job-manager-dummy.c
ingest_job_manager_dummy_la_CPPFLAGS = $(test_cppflags)
ingest_job_manager_dummy_la_LDFLAGS = $(fluxmod_ldflags) -module -rpath /nowhere
ingest_job_manager_dummy_la_LIBADD = \
        $(test_ldadd) $(LIBDL) $(LIBUTIL)

ingest_submitbench_SOURCES = ingest/submitbench.c
ingest_submitbench_CPPFLAGS = $(test_cppflags)
ingest_submitbench_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

job_manager_list_jobs_SOURCES = job-manager/list-jobs.c
job_manager_list_jobs_CPPFLAGS = $(test_cppflags)
job_manager_list_jobs_LDADD = \
	$(test_ldadd) $(LIBDL) $(LIBUTIL)

job_manager_sched_dummy_la_SOURCES = job-manager/sched-dummy.c
job_manager_sched_dummy_la_CPPFLAGS = $(test_cppflags)
job_manager_sched_dummy_la_LDFLAGS = $(fluxmod_ldflags) -module -rpath /nowhere
job_manager_sched_dummy_la_LIBADD = \
        $(top_builddir)/src/common/libschedutil/libschedutil.la \
        $(test_ldadd) $(LIBDL) $(LIBUTIL)

sched_simple_jj_reader_SOURCES = sched-simple/jj-reader.c
sched_simple_jj_reader_CPPFLAGS = $(test_cppflags)
sched_simple_jj_reader_LDADD = \
	$(top_builddir)/src/modules/sched-simple/libjj.la \
	$(test_ldadd)

shell_plugins_dummy_la_SOURCES = shell/plugins/dummy.c
shell_plugins_dummy_la_CPPFLAGS = $(test_cppflags)
shell_plugins_dummy_la_LDFLAGS = -module -rpath /nowhere

shell_plugins_conftest_la_SOURCES = shell/plugins/conftest.c
shell_plugins_conftest_la_CPPFLAGS = $(test_cppflags)
shell_plugins_conftest_la_LDFLAGS = -module -rpath /nowhere

shell_plugins_invalid_args_la_SOURCES = shell/plugins/invalid-args.c
shell_plugins_invalid_args_la_CPPFLAGS = $(test_cppflags)
shell_plugins_invalid_args_la_LDFLAGS = -module -rpath /nowhere
shell_plugins_invalid_args_la_LIBADD = \
	$(top_builddir)/src/common/libtap/libtap.la \
        $(top_builddir)/src/common/libflux-core.la

shell_plugins_getopt_la_SOURCES = shell/plugins/getopt.c
shell_plugins_getopt_la_CPPFLAGS = $(test_cppflags)
shell_plugins_getopt_la_LDFLAGS = -module -rpath /nowhere
shell_plugins_getopt_la_LIBADD = \
	$(top_builddir)/src/common/libtap/libtap.la \
        $(top_builddir)/src/common/libflux-core.la

shell_plugins_log_la_SOURCES = shell/plugins/log.c
shell_plugins_log_la_CPPFLAGS = $(test_cppflags)
shell_plugins_log_la_LDFLAGS = -module -rpath /nowhere
shell_plugins_log_la_LIBADD = \
	$(top_builddir)/src/common/libtap/libtap.la \
        $(top_builddir)/src/common/libflux-core.la

shell_plugins_test_event_la_SOURCES = shell/plugins/test-event.c
shell_plugins_test_event_la_CPPFLAGS = $(test_cppflags)
shell_plugins_test_event_la_LDFLAGS = -module -rpath /nowhere
shell_plugins_test_event_la_LIBADD = \
        $(top_builddir)/src/common/libflux-core.la
