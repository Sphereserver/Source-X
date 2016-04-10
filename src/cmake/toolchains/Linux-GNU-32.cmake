SET (TOOLCHAIN 1)
MESSAGE (STATUS "Toolchain: Linux-GNU-32.cmake.")

SET (C_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-switch")
SET (CXX_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-switch")
SET (C_ARCH_OPTS "-march=i686 -m32")
SET (CXX_ARCH_OPTS "-march=i686 -m32")
SET (C_OPTS "-s -fno-omit-frame-pointer -ffast-math -O3 -fno-expensive-optimizations -std=c11")
SET (CXX_OPTS "-s -fno-omit-frame-pointer -ffast-math -fpermissive -O3 -std=c++11")
SET (C_SPECIAL "-fexceptions -fnon-call-exceptions")
SET (CXX_SPECIAL "-fexceptions -fnon-call-exceptions")
SET (CMAKE_C_FLAGS "${C_WARNING_FLAGS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL}")
SET (CMAKE_CXX_FLAGS "${CXX_WARNING_FLAGS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL}")

# Optimization flags set to max.
SET_TARGET_PROPERTIES (spheresvr		PROPERTIES	COMPILE_FLAGS -O3 )
#SET_TARGET_PROPERTIES (spheresvrNightly	PROPERTIES	COMPILE_FLAGS -O3 )
# Force dynamic linking.
SET (CMAKE_EXE_LINKER_FLAGS "-dynamic")

LINK_DIRECTORIES ("/usr/lib")