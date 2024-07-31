#!/bin/sh

_SAN_COMMON_FLAGS=handle_abort=true:abort_on_error=false
#ASAN_MORE=check_initialization_order=1
export ASAN_OPTIONS="${_SAN_COMMON_FLAGS}"
export ASAN_OPTIONS="${ASAN_OPTIONS}:strict_init_order=true:detect_stack_use_after_return=true"
export ASAN_OPTIONS="${ASAN_OPTIONS}:sleep_before_dying=3:print_stats=true:stack_trace_format=\"[frame=%n, function=%f, location=%S]\""
export LSAN_OPTIONS="${_SAN_COMMON_FLAGS}"
export MSAN_OPTIONS="${_SAN_COMMON_FLAGS}"
export TSAN_OPTIONS="${_SAN_COMMON_FLAGS}"
export UBSAN_OPTIONS="${_SAN_COMMON_FLAGS}:print_stacktrack=true"

export ASAN_SYMBOLIZER_PATH='/usr/bin/addr2line'
