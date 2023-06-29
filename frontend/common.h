#ifndef COMMON_H
#define COMMON_H

#ifndef OS_INFO
#define OS_INFO unknown
#endif
#ifndef REP_VERSION
#define REP_VERSION latest
#endif
#ifndef REP_COMMIT_HASH
#define REP_COMMIT_HASH latest
#endif

#define STR1(m) #m
#define STR(m) STR1(m)

#endif