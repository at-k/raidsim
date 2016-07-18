#ifndef LDSTRUCT_H
#define LDSTRUCT_H

#include <limits.h>
#include <deque>
#include <vector>

#include "nf_common.h"
#include "phy/fm_macro_def.h"
#include "pbque.h"

//-- FTL data structure- --
//-- definition
// HLBA: Host Logical Block Address : host address
// LPN : Logical Page Number        : corresponding to host address
// PPN : Physical Page Number       : unique physical page number
// PPO : Physical Page Offset       : offset page number in physical block
// VPA : Virtual Page Address       : virtual page number (32bit: PG#, 24bit: PG Offset Sector, 8bit: Length)


//-- macro definition
#define FTL_LP_NOT_ASSIGNED    0xFFFFFFFF      // map value for not assigned logical page
#define FTL_PG_INVALID_ID      0xFFFFFFFF      // invalid PG number
#define FTL_REWR_END           0xFFFFFFFF      // end rewrite

#define FTL_PG_BLOCK_NUM      32               // Block RAID(N in NDxP)
#define FTL_PG_PARITY_NUM     1                // Block RAID(x in NDxP)
#define FTL_SECTS_PER_LP      16               // LP sector size

#define FTL_OPENPG_NUM        1	               // open pg number
#define FTL_OPENPG_WL_NUM     7                // open pg for special

#define FTL_MAX_REWR_REQ      16               // maximum GC request
#define FTL_MAX_ERASE_REQ     4                // maximum erase requesst
#define FTL_MAX_ERASE_REG     256              // maximum erase reservation


//-- declaration
class  FM_INFO;
class  FtlInterface;
class  FtlAsynReqInterface;

//-- type declaration
//status
enum FTL_PG_STATUS
{
    FTL_PGS_FREE,
    FTL_PGS_OPEN,
    FTL_PGS_CLOSE,
    FTL_PGS_RCM
};
enum FTL_PB_STATUS
{
    FTL_PBS_FREE,
    FTL_PBS_BIND,   // added to PG
    FTL_PBS_INVALID
};

//address space definition
typedef uint32_t FTL_LP_GADDR;
typedef uint32_t FTL_PG_GADDR;
typedef uint32_t FTL_PB_GADDR;
typedef uint32_t FTL_PP_GADDR;
typedef uint32_t FTL_PG_OADDR; // pg offset address
typedef uint16_t FTL_PB_OADDR; // pb offset address

//-- IO request
enum FTL_FMIO_TYPE
{
    FMT_HOST_WR, // host request write
    FMT_HOST_RD, // host request read
    FMT_RCM_RWR, // block reclamation
    FMT_REF_RWR, // RF
    FMT_BLK_ERS, // erase
    FMT_DEFAULT  // invalid
};

//-- struct definition
// ftl option
typedef struct {
    bool      enable_pg_composition;
    uint32_t  pg_pb_num;
    uint32_t  pg_parity_num;
    bool      enable_rcm_th;
    uint32_t  rcm_th;
} FTL_EXT_OPT;

// open pg information
typedef struct {
   FTL_FMIO_TYPE type; // open type
   uchar           id; // registered ID(to specify array position)
} FTL_OPENPG_INFO;

// block erase reservation information
typedef struct {
    FTL_PB_GADDR pb_id;  // block address
    uint32_t     reg_id; // reservation ID
} FTL_ERASE_REQ;

// virtual page address
typedef struct {
    FTL_PG_GADDR pgn;
    FTL_PG_OADDR ofs;
    uchar        len;
    uchar        rcm_count; // option for wearleveling
} VPA_INFO;

// reverse map
typedef struct {
    FTL_LP_GADDR lpn;
    uchar        len;
} REVERSE_INFO;

// physical block information
typedef struct __PB_INFO {
    FTL_PB_GADDR  id;          // unique block id
    FTL_PB_STATUS status;

    FTL_PG_GADDR  pg_no;       // belonging PG no

    uint32_t  erase_count;     // cumulative erase count
    uint64_t  last_erase_time;

    //-- physical position information
    uint16_t  ce_no;  // CE no
    uint16_t  bus_no; // bus no

} PB_INFO;

// PG(parity group) information
typedef struct __PG_INFO {
    FTL_PG_GADDR  id;        // unique ID
    FTL_PG_STATUS status;

    uchar         attr;      // open attribution

    FTL_PG_OADDR  vs_num;    // valid secotr num
    FTL_LP_GADDR  lp_num;    // valid LP num

    FTL_OPENPG_INFO  opg_info;  // reference for open pg information
    FTL_PG_OADDR     next_ofs;  // next write position
    PB_INFO**        pb_list;   // block information

    std::deque<REVERSE_INFO> p2l_tbl; // reverse map
    FTL_PG_OADDR  next_rewr_index;    // for gc: next copy target
    FTL_PG_OADDR  next_rewr_pgsect;   // for gc: next copy target sector

    QUEUE  que; // close que
} PG_INFO;

//-- 空きブロック選択用，前回までのブロック選択情報
typedef struct {
    uint16_t      cur_ce;  // 現在の選択CE
    uint16_t      ce_count;// 今までの連続選択回数
    uint16_t      next_select_bus_offset; // 前回選択したバスオフセット
} LAST_PB_INFO;

//-- キュー用型
typedef std::deque<PB_INFO*> PB_STL_QUE;
typedef std::deque<PG_INFO*> PG_STL_QUE;
typedef PB_STL_QUE::iterator PB_STL_ITR;
typedef PG_STL_QUE::iterator PG_STL_ITR;
typedef QUEUE_HEAD           PG_UTL_QUE;

//-- プール
//   空きブロックと無効ブロックの管理
typedef struct __POOL_INFO {

    FTL_PB_GADDR  pb_num;

    FTL_PB_GADDR  total_free_pb_count;    // 全空きブロック数
    FTL_PB_GADDR  total_invalid_pb_count; // キュー内の全無効ブロック数
    uint16_t  pre_free_select_ce;     // 前回空きブロック選択CE
    uint16_t  pre_invalid_select_ce;  // 前回無効ブロック選択CE

    //FTL_LP_GADDR  initial_vp_count;  // 初期有効ページ数
    //FTL_PB_GADDR  rcm_tgt_pb_num;    // RCM対象ブロック数

    // ブロック状態管理キュー
    uint16_t  que_bus_num;
    uint16_t  que_ce_num;

    PB_STL_QUE**  free_block_que;    // 空きブロックキュー[BUS][CE]
    PB_STL_QUE**  invalid_block_que; // 無効ブロックキュー[BUS][CE]

    std::deque<uint32_t>  erase_resv_que;         // 空き消去予約資源キュー
    PB_INFO*  erase_resv_list[FTL_MAX_ERASE_REG]; // 消去予約資源

} POOL_INFO;

//-- ActiveなPG情報管理
//   書き先管理，RCM対象管理，など
typedef struct __ACT_PG_INFO{

    // キュー
    //PG_STL_QUE    close_pg_que;      // 有効データブロックを持つPG
    PG_UTL_QUE    close_pg_que;
    PG_STL_QUE    rcm_pg_que;        // RCM対象のPG
    PG_STL_QUE    rcm_cmp_pg_que;    // RCM完了待ちのPG

    // オープンパリティグループ
    PG_INFO*  host_openpg[FTL_OPENPG_NUM]; // ホスト用
    PG_INFO*  rcm_openpg[FTL_OPENPG_NUM];  // リクラメーション用

    PG_INFO*  wl_openpg[FTL_OPENPG_WL_NUM]; // WL検証用オープンPG

} ACT_PG_INFO;

//-- FTLのデータ構造を一括管理
//
class LP_INFO {
public:
    LP_INFO();
    ~LP_INFO();

    bool InitFTL( FM_INFO* fm_info, uint64_t usr_area_sector_num,
        FtlInterface* ftlinterface, FtlAsynReqInterface* ftl_asyn_if , FTL_EXT_OPT* option = NULL );

    void InitPG( PG_INFO* pg);

	// pool & pg & pb
    FTL_PG_GADDR   pg_num;
    FTL_PB_GADDR   pb_num;

    POOL_INFO   pool;    // poolは1つ。
    ACT_PG_INFO apg;     // 書き先管理も1つ。
    PG_INFO*    pg_list;
    PB_INFO*    pb_list;

    // パリティグループ状態管理キュー
    PG_STL_QUE    free_pg_que;       // 空きPGキュー(論理資源なので特にソート不要)

    // 論理・物理ページ数
    FTL_LP_GADDR   lp_num;
    FTL_PP_GADDR   pp_num;
    VPA_INFO*      l2p_tbl;  // 論物

    long double    op_ratio;      // over provisioning ratio (ホスト視点)
    long double    real_op_ratio; // 内部RAID込の実質OP
    uint32_t       rcm_th;        // rcm開始閾値(更新領域に対する100分率)
    uint32_t       erase_th;      // 消去対象ブロック数の閾値

    uint32_t       pg_pb_num;     // pg辺りpb数
    uint32_t       pg_parity_num; // pg辺りparityブロック数
    uint32_t       pg_cw_num;     // pg辺りcw数

};

#endif
