INCLUDE_DIRECTORIES (
lib/mariadb/
lib/flat_containers/
lib/object_ptr/
lib/parallel_hashmap/
lib/regex/
)

# LibEv files
SET (libev_SRCS
lib/libev/wrapper_ev.c
)
SOURCE_GROUP (lib\\libev FILES ${libev_SRCS})

# SQLite files
SET (sqlite_SRCS
lib/sqlite/sqlite3.c
lib/sqlite/sqlite3.h
)
SOURCE_GROUP (lib\\sqlite FILES ${sqlite_SRCS})

# ZLib files
SET (zlib_SRCS
lib/zlib/adler32.c
lib/zlib/compress.c
lib/zlib/crc32.c
lib/zlib/crc32.h
lib/zlib/deflate.c
lib/zlib/deflate.h
lib/zlib/gzclose.c
lib/zlib/gzguts.h
lib/zlib/gzlib.c
lib/zlib/gzread.c
lib/zlib/gzwrite.c
lib/zlib/infback.c
lib/zlib/inffast.c
lib/zlib/inffast.h
lib/zlib/inffixed.h
lib/zlib/inflate.c
lib/zlib/inflate.h
lib/zlib/inftrees.c
lib/zlib/inftrees.h
lib/zlib/trees.c
lib/zlib/trees.h
lib/zlib/uncompr.c
lib/zlib/zconf.h
lib/zlib/zlib.h
lib/zlib/zutil.c
lib/zlib/zutil.h
)
SOURCE_GROUP (lib\\zlib FILES ${zlib_SRCS})


SET (LIB_SRCS
	${libev_SRCS}
	${sqlite_SRCS}
	${zlib_SRCS}
)
