SET (C_WARNING_OPTS
		"-Wall -Wextra -Wno-unknown-pragmas -Wno-format -Wno-switch -Wno-implicit-fallthrough\
		-Wno-parentheses -Wno-misleading-indentation -Wno-strict-aliasing -Wno-unused-result\
		-Wno-error=unused-but-set-variable -Wno-implicit-function-declaration") # this line is for warnings issued by 3rd party C code

SET (C_OPTS			"-std=c11   -pthread -fexceptions -fnon-call-exceptions")
SET (C_SPECIAL		"-pipe")
