@echo off
REM SET ASAN_SYMBOLIZER_PATH="C:\Program Files\LLVM\bin\llvm-symbolizer.exe"
REM ASAN_MORE=check_initialization_order=1:
@echo on

SET _SAN_COMMON_FLAGS=symbolize=1:handle_abort=true:abort_on_error=false
SET ASAN_OPTIONS=%_SAN_COMMON_FLAGS%:strict_init_order=true:detect_stack_use_after_return=true
SET ASAN_OPTIONS=%ASAN_OPTIONS%:sleep_before_dying=3:print_stats=true:stack_trace_format="[frame=%%n, function=%%f, location=%%S]"
SET LSAN_OPTIONS=%_SAN_COMMON_FLAGS%
SET MSAN_OPTIONS=%_SAN_COMMON_FLAGS%
SET TSAN_OPTIONS=%_SAN_COMMON_FLAGS%
SET UBSAN_OPTIONS="${_SAN_COMMON_FLAGS}:print_stacktrack=true"
SET ASAN_SYMBOLIZER_PATH="C:\LLVM\bin\llvm-symbolizer.exe"
