#ifndef COMMON_DEF__H
#define COMMON_DEF__H


// generic type definition
#ifndef WIN32
#include <inttypes.h>
typedef unsigned char          uchar;

#else
typedef unsigned char          uchar;
typedef unsigned short int     uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long int uint64_t;

#endif

// including utility
//#include "../Util/Logger.h"
//#include "../Util/RandUtil.h"

// location indicator
#define _STR(x)      #x
#define _STR2(x)     _STR(x)
#define HERE         __FILE__ "(" _STR2(__LINE__) ")"

// simple error trace
#define ERR_AND_RTN { printf( HERE "error\n"); return false; }

// signature definition for logger
#define LOG_TYPE_ERROR  "error"
#define LOG_TYPE_RESULT "result"
#define LOG_TYPE_LOG    "log"
#define LOG_TYPE_TRACE  "trace"
#define LOG_TYPE_WL "wl"

//--statistics
//
typedef struct
{
    uint64_t cur_sim_time;
    //uint64_t pre_sim_time;

    uint64_t host_io_count;
    //uint64_t pre_host_io_count;

    uint64_t host_rd_count;
    uint64_t host_wr_count;
    uint64_t host_rd_sect;
    uint64_t host_wr_sect;
    /*
    uint64_t host_io_comp_count;
    uint64_t host_rd_comp_count;
    uint64_t host_wr_comp_count;
    */
    uint64_t host_io_resp_time;
    uint64_t host_rd_resp_time;
    uint64_t host_wr_resp_time;

    uint64_t host_io_worst_resp_time;
    uint64_t host_rd_worst_resp_time;
    uint64_t host_wr_worst_resp_time;

    uint64_t fm_io_count;

    uint64_t fm_page_rd_count;
    uint64_t fm_page_wr_count;
    uint64_t fm_page_copy_count;
    uint64_t fm_erase_count;

    uint64_t fm_host_fb_cons_count; // 空き消費量(ホスト用)
    uint64_t fm_rc_fb_cons_count;   // 空き消費量(RCM/RF用)

    uint64_t fb_count; // 空きブロック数
    uint64_t ib_count; // 無効ブロック数

    uint64_t ce_util_time;
    uint64_t bus_util_time;

    uint64_t ce_multi_count;         // CEまとめ成功回数
    uint64_t rewrite_ce_multi_count; // CEまとめ成功回数@ReWrite

} STATIS_INFO;

extern STATIS_INFO  g_statis;        // 前回出力時からの差分統計情報
extern STATIS_INFO  g_ttl_statis;    // 開始からの統計情報

#endif
