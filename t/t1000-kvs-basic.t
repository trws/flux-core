#!/bin/sh
#

test_description='Test basic kvs usage in flux session

Verify basic KVS operations against a running flux comms session.
This test script verifies operation of the kvs and should be run
before other tests that depend on kvs.
'

. `dirname $0`/sharness.sh

# Size the session to one more than the number of cores, minimum of 4
SIZE=$(($(nproc)+1))
test ${SIZE} -lt 4 && SIZE=4
test_under_flux ${SIZE}
echo "# $0: flux session size will be ${SIZE}"

TEST=$TEST_NAME
KEY=test.a.b.c

#
#
#
test_kvs_key() {
	flux kvs get "$1" >output
	echo "$2" >expected
	test_cmp output expected
	#if ! test "$OUTPUT" = "$2"; then
	#	test_debug say_color error "Error: Output \'$OUTPUT\" != \'$2\'"
	#	return false
	#fi
}

test_kvs_type () {
	flux kvs type "$1" >output
	echo "$2" >expected
	test_cmp output expected
}

test_expect_success 'kvs: get a nonexistent key' '
	test_must_fail flux kvs get NOT.A.KEY
'


test_expect_success 'kvs: integer put' '
	flux kvs put $KEY=42 
'
test_expect_success 'kvs: integer type' '
	test_kvs_type $KEY int
'
test_expect_success 'kvs: integer get' '
	test_kvs_key $KEY 42
'
test_expect_success 'kvs: unlink works' '
	flux kvs unlink $KEY &&
	  test_must_fail flux kvs get $KEY
'
test_expect_success 'kvs: value can be empty' '
	flux kvs put $KEY= &&
	  test_kvs_key $KEY "" &&
	  test_kvs_type $KEY string
'
KEY=$TEST.b.c.d
DIR=$TEST.b.c
test_expect_success 'kvs: string put' '
	flux kvs put $KEY="Hello world"
'
test_expect_success 'kvs: string type' '
	test_kvs_type $KEY string
'
test_expect_success 'kvs: string get' '
	test_kvs_key $KEY "Hello world"
'
test_expect_success 'kvs: boolean put (true)' '
	flux kvs put $KEY=true
'
test_expect_success 'kvs: boolean type' '
	test_kvs_type $KEY boolean
'
test_expect_success 'kvs: boolean get (true)' '
	test_kvs_key $KEY true
'
test_expect_success 'kvs: boolean put (false)' '
	flux kvs put $KEY=false
'
test_expect_success 'kvs: boolean type' '
	test_kvs_type $KEY boolean
'
test_expect_success 'kvs: boolean get (false)' '
	test_kvs_key $KEY false
'
test_expect_success 'kvs: put double' '
	flux kvs put $KEY=3.14159
'
test_expect_success 'kvs: double type' '
	test_kvs_type $KEY double
'
test_expect_success 'kvs: get double' '
	test_kvs_key $KEY 3.141590
'
test_expect_success 'kvs: array put' '
	flux kvs put $KEY="[1,3,5,7]"
'
test_expect_success 'kvs: array type' '
	test_kvs_type $KEY array
'
test_expect_success 'kvs: array get' '
	test_kvs_key $KEY "[ 1, 3, 5, 7 ]"
'
test_expect_success 'kvs: object put' '
	flux kvs put $KEY="{\"a\":42}"
'
test_expect_success 'kvs: object type' '
	test_kvs_type $KEY object
'
test_expect_success 'kvs: object get' '
	test_kvs_key $KEY "{ \"a\": 42 }"
'
test_expect_success 'kvs: try to retrieve key as directory should fail' '
	test_must_fail flux kvs dir $KEY
'
test_expect_success 'kvs: try to retrieve a directory as key should fail' '
	test_must_fail flux kvs get $DIR
'

test_empty_directory() {
	OUTPUT=`flux kvs dirsize $1` &&
	test "x$OUTPUT" = "x0"
}
test_expect_success 'kvs: empty directory remains after key removed' '
	flux kvs unlink $KEY &&
	test_empty_directory $DIR
'
test_expect_success 'kvs: remove directory' '
	flux kvs unlink $TEST
'
test_expect_success 'kvs: empty directory can be created' '
	flux kvs mkdir $DIR  &&
	test_empty_directory $DIR
'
test_expect_success 'kvs: put values in a directory then retrieve them' '
	flux kvs put $DIR.a=69 $DIR.b=70 $DIR.c=3.14 $DIR.d=\"snerg\" $DIR.e=true &&
	flux kvs dir $DIR | sort >output &&
	cat >expected <<EOF
$DIR.a = 69
$DIR.b = 70
$DIR.c = 3.140000
$DIR.d = snerg
$DIR.e = true
EOF
	test_cmp expected output
'
test_expect_success 'kvs: create a dir with keys and subdir' '
	flux kvs unlink $TEST &&
	flux kvs put $DIR.a=69 $DIR.b=70 $DIR.c.d.e.f.g=3.14 $DIR.d=\"snerg\" $DIR.e=true &&
	flux kvs dir -r $DIR | sort >output &&
	cat >expected <<EOF
$DIR.a = 69
$DIR.b = 70
$DIR.c.d.e.f.g = 3.140000
$DIR.d = snerg
$DIR.e = true
EOF
	test_cmp expected output
'

test_expect_success 'kvs: directory with multiple subdirs' '
	flux kvs unlink $TEST &&
	flux kvs put $DIR.a=69 $DIR.b.c.d.e.f.g=70 $DIR.c.a.b=3.14 $DIR.d=\"snerg\" $DIR.e=true &&
	flux kvs dir -r $DIR | sort >output &&
	cat >expected <<EOF
$DIR.a = 69
$DIR.b.c.d.e.f.g = 70
$DIR.c.a.b = 3.140000
$DIR.d = snerg
$DIR.e = true
EOF
	test_cmp expected output
'

test_expect_success 'kvs: cleanup' '
	flux kvs unlink $TEST
'
test_expect_success 'kvs: symlink: works' '
	TARGET=$TEST.a.b.c &&
	flux kvs put $TARGET=\"foo\" &&
	flux kvs link $TARGET $TEST.Q &&
	OUTPUT=$(flux kvs get $TEST.Q) &&
	test "$OUTPUT" = "foo"
'
test_expect_success 'kvs: symlink: path resolution when intermediate component is a symlink' '
	flux kvs unlink $TEST &&
	flux kvs put $TEST.a.b.c=42 &&
	flux kvs link $TEST.a.b $TEST.Z.Y &&
	OUTPUT=$(flux kvs get $TEST.Z.Y.c) &&
	test "$OUTPUT" = "42"
'
test_expect_success 'kvs: symlink: path resolution with intermediate symlink and nonexistent key' '
	flux kvs unlink $TEST &&
	flux kvs link $TEST.a.b $TEST.Z.Y &&
	test_must_fail flux kvs get $TEST.Z.Y
'
test_expect_success 'kvs: symlink: intermediate symlink points to another symlink' '
	flux kvs unlink $TEST &&
	flux kvs put $TEST.a.b.c=42 &&
	flux kvs link $TEST.a.b $TEST.Z.Y &&
	flux kvs link $TEST.Z.Y $TEST.X.W &&
	test_kvs_key $TEST.X.W.c 42
'

test_expect_success 'kvs: kvsdir_get_size works' '
	flux kvs mkdir $TEST.dirsize &&
	flux kvs put $TEST.dirsize.a=1 &&
	flux kvs put $TEST.dirsize.b=2 &&
	flux kvs put $TEST.dirsize.c=3 &&
	OUTPUT=$(flux kvs dirsize $TEST.dirsize) &&
	test "$OUTPUT" = "3"
'

# Keep the next two tests in order
test_expect_success 'kvs: symlink: dangling link' '
	flux kvs unlink $TEST &&
	flux kvs link $TEST.dangle $TEST.a.b.c
'
test_expect_success 'kvs: symlink: readlink on dangling link' '
	OUTPUT=$(flux kvs readlink $TEST.a.b.c) &&
	test "$OUTPUT" = "$TEST.dangle"
'
test_expect_success 'kvs: symlink: readlink works on non-dangling link' '
	flux kvs unlink $TEST &&
	flux kvs put $TEST.a.b.c="foo" &&
	flux kvs link $TEST.a.b.c $TEST.link &&
	OUTPUT=$(flux kvs readlink $TEST.link) &&
	test "$OUTPUT" = "$TEST.a.b.c"
'

# test synchronization based on commit sequence no.

test_expect_success 'kvs: put on rank 0, exists on all ranks' '
	flux kvs put $TEST.xxx=99 &&
	VERS=$(flux kvs version) &&
	flux exec sh -c "flux kvs wait ${VERS} && flux kvs exists $TEST.xxx"
'

test_expect_success 'kvs: unlink on rank 0, does not exist all ranks' '
	flux kvs unlink $TEST.xxx &&
	VERS=$(flux kvs version) &&
	flux exec sh -c "flux kvs wait ${VERS} && ! flux kvs exists $TEST.xxx"
'

# commit test

test_expect_success 'kvs: 8 threads/rank each doing 100 put,commits in a loop' '
	THREADS=8 &&
	flux exec ${FLUX_BUILD_DIR}/src/test/tcommit ${THREADS} 100 \
		$(basename ${SHARNESS_TEST_FILE})
'

# fence test

test_expect_success 'kvs: 8 threads/rank each doing 100 put,fence in a loop' '
	THREADS=8 &&
	flux exec ${FLUX_BUILD_DIR}/src/test/tcommit \
		--fence $((${SIZE}*${THREADS})) ${THREADS} 100 \
		$(basename ${SHARNESS_TEST_FILE})
'

# watch tests

test_expect_success 'kvs: watch-mt: multi-threaded kvs watch program' '
	${FLUX_BUILD_DIR}/t/kvs/watch mt 100 100 $TEST.a &&
	flux kvs unlink $TEST.a
'

test_expect_success 'kvs: watch-selfmod: watch callback modifies watched key' '
	${FLUX_BUILD_DIR}/t/kvs/watch selfmod $TEST.a &&
	flux kvs unlink $TEST.a
'

test_expect_success 'kvs: watch-unwatch unwatch works' '
	${FLUX_BUILD_DIR}/t/kvs/watch unwatch $TEST.a &&
	flux kvs unlink $TEST.a
'

test_expect_success 'kvs: watch-unwatchloop 1000 watch/unwatch ok' '
	${FLUX_BUILD_DIR}/t/kvs/watch unwatchloop $TEST.a &&
	flux kvs unlink $TEST.a
'

# regression tests

test_expect_success 'kvs: put x=foo, get x.y fails without panic (issue 441)' '
	flux kvs put ${TEST}.x=foo &&
	! flux kvs get ${TEST}.x.y
	flux kvs get ${TEST}.x   # fails if broker died
'

test_done
