#!/use/bin/sh
clang-tidy -p ../build/compile_commands.json -config-file='../.clang-tidy' ../src/**/*.cpp ../src/**/*.h
