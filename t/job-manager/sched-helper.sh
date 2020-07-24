#!/bin/sh
#

# job-manager sched helper functions

JMGR_JOB_LIST=${FLUX_BUILD_DIR}/t/job-manager/list-jobs

# internal function to get job state via job manager
#
# if job is not found by list-jobs, but the clean event exists
# in the job's eventlog, return state as inactive
#
# arg1 - jobid
_jmgr_get_state() {
        local id=$1
        local state=$(${JMGR_JOB_LIST} | awk '$1 == "'${id}'" { print $2; }')
        test -z "$state" \
                && flux job wait-event --timeout=5 ${id} clean >/dev/null \
                && state=I
        echo $state
}

# verify if job is in specific state through job manager
#
# function will loop for up to 5 seconds in case state change is slow
#
# arg1 - jobid
# arg2 - single character expected state (e.g. R = running)
jmgr_check_state() {
        local id=$1
        local wantstate=$2
        for try in $(seq 1 10); do
                test $(_jmgr_get_state $id) = $wantstate && return 0
                sleep 0.5
        done
        return 1
}

# internal function to get job annotation key value via job manager
#
# arg1 - jobid
# arg2 - key
_jmgr_get_annotation() {
        local id=$1
        local key=$2
        local note="$(${JMGR_JOB_LIST} | grep ${id} | cut -f 6- | jq ."${key}")"
        echo $note
}

# verify if job contains specific annotation key & value through job manager
#
# function will loop for up to 5 seconds in case annotation update
# arrives slowly
#
# arg1 - jobid
# arg2 - key in annotation
# arg3 - value of key in annotation
#
# callers should set HAVE_JQ requirement
jmgr_check_annotation() {
        local id=$1
        local key=$2
        local value="$3"
        for try in $(seq 1 10); do
                test "$(_jmgr_get_annotation $id $key)" = "${value}" && return 0
                sleep 0.5
        done
        return 1
}

# verify if job contains specific annotation key through job manager
#
# arg1 - jobid
# arg2 - key in annotation
#
# callers should set HAVE_JQ requirement
jmgr_check_annotation_exists() {
        local id=$1
        local key=$2
        ${JMGR_JOB_LIST} | grep ${id} | cut -f 6- | jq -e ."${key}" > /dev/null
}

# verify that job contains no annotations through job manager
#
# arg1 - jobid
jmgr_check_no_annotations() {
        local id=$1
        test -z "$(${JMGR_JOB_LIST} | grep ${id} | cut -f 6-)" && return 0
        return 1
}