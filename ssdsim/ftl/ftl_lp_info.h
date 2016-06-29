#ifndef LDSTRUCT_H
#define LDSTRUCT_H

#include <limits.h>
#include <deque>
#include <vector>

#include "nf_common.h"
#include "phy/fm_macro_def.h"
#include "pbque.h"

//-- FTLのデータ構造総まとめ- --
//-- 略称定義
// HLBA: Host Logical Block Number : ホストアドレスでユニーク
// LPN : Logical Page Number       : ホストアドレスに1対1対応
// PPN : Physical Page Number      : 物理ページ番号でパッケージ内ユニーク
// PPO : Physical Page Offset      : 物理ブロック内の物理ページオフセット
// VPA : Virtual Page Address      : 仮想ページ番号   (32bit: PG#, 24bit: PG Offset Sector, 8bit: Length)


//-- マクロ定義
//#define VP_BITMAP_BYTES (PP_PER_PB / (8*sizeof(uchar)) )  // 物理ページの有効/無効判定ビットマップサイズ
#define FTL_LP_NOT_ASSIGNED    0xFFFFFFFF                 // リソース未割り当てLP用のID
#define FTL_PG_INVALID_ID      0xFFFFFFFF                 // 無効PG番号
#define FTL_REWR_END           0xFFFFFFFF                 // ReWrite終了

#define FTL_PG_BLOCK_NUM     32                           // ND1PのN+1数
#define FTL_PG_PARITY_NUM     1                           // NDxPのP数
#define FTL_SECTS_PER_LP     16                           // 1論理ページあたりのセクタ数
//#define FTL_CW_PER_PG        ((FTL_PG_BLOCK_NUM-FTL_PG_PARITY_NUM) * PP_PER_PB * CW_PER_PP ) // PGあたりのCW数
//#define FTL_PG_END_CW        FTL_CW_PER_PG                // 最終CW

#define FTL_OPENPG_NUM           1                           // 同時OpenPG数
#define FTL_OPENPG_WL_NUM        7                           // 同時OpenPG数,WL試作用

#define FTL_MAX_REWR_REQ      16                          // 最大同時要求ReWrite数
#define FTL_MAX_ERASE_REQ     4                           // 最大同時消去ブロック
#define FTL_MAX_ERASE_REG     256                         // 最大同時消去予約数


//-- 先行宣言
class  FM_INFO;
class  FtlInterface;
class  FtlAsynReqInterface;

//-- 型宣言
//状態
enum FTL_PG_STATUS
{
    FTL_PGS_FREE,
    FTL_PGS_OPEN,
    FTL_PGS_CLOSE,
    FTL_PGS_RCM
};
enum FTL_PB_STATUS
{
    FTL_PBS_FREE,   // 空き
    FTL_PBS_BIND,   // PG構成中
    FTL_PBS_INVALID // 無効
};

//アドレス空間
typedef uint32_t FTL_LP_GADDR;
typedef uint32_t FTL_PG_GADDR;
typedef uint32_t FTL_PB_GADDR;
typedef uint32_t FTL_PP_GADDR;
typedef uint32_t FTL_PG_OADDR; // pg offset address
typedef uint16_t FTL_PB_OADDR; // pb offset address

//-- IO要求種別
enum FTL_FMIO_TYPE
{
    FMT_HOST_WR, // ホスト要求
    FMT_HOST_RD, // ホスト要求
    FMT_RCM_RWR, // RCM
    FMT_REF_RWR, // RF
    FMT_BLK_ERS, // 消去リクエスト
    FMT_DEFAULT  // 無効値
};

//-- 構造体定義
// 拡張オプション
typedef struct {
    bool      enable_pg_composition;// pg構成設定のためのフラグ
    uint32_t  pg_pb_num;
    uint32_t  pg_parity_num;

    bool      enable_rcm_th;// rcm閾値設定のためのフラグ
    uint32_t  rcm_th;
} FTL_EXT_OPT;

// オープン情報
typedef struct {
   FTL_FMIO_TYPE type; // オープンタイプ
   uchar           id; // 登録ID（配列位置）
} FTL_OPENPG_INFO;

// ブロック消去予約
typedef struct {
    FTL_PB_GADDR pb_id;  // ブロックアドレス
    uint32_t     reg_id; // 登録チケットID
} FTL_ERASE_REQ;

// VPA構造体形式
typedef struct {
    FTL_PG_GADDR pgn;
    FTL_PG_OADDR ofs;
    uchar        len;
    uchar        rcm_count; // 連続RCM実施回数
} VPA_INFO;

// 逆引き情報
typedef struct {
    FTL_LP_GADDR lpn;
    uchar        len;
} REVERSE_INFO;

// CW位置情報
typedef struct {
    FTL_PB_GADDR pbn;
    FTL_PB_OADDR ppo;
    uchar    cwo;
    uchar    len;
} CW_INFO;

// 物理ブロック情報
typedef struct __PB_INFO {
    FTL_PB_GADDR  id;          // ユニークな物理ブロックID
    FTL_PB_STATUS status;      // ブロック状態

    FTL_PG_GADDR  pg_no;       // PG番号

    uint32_t  erase_count;     // 累計消去回数(劣化度)
    uint64_t  last_erase_time; // 最終消去時間

    //-- 制御用情報
    uint16_t  ce_no;  // 所属CE番号 ( 0～busあたりCE数 - 1 )
    uint16_t  bus_no; // 所属BUS番号( 通番 )

} PB_INFO;

// パリティグループ情報
typedef struct __PG_INFO {
    FTL_PG_GADDR  id;        // ユニークID
    FTL_PG_STATUS status;    // ステータス

    uchar         attr;      // オープン属性(hot/cold的な)，暫定的に1Bインデックスとする

    FTL_PG_OADDR  vs_num;    // PG内の有効セクタ数
    FTL_LP_GADDR  lp_num;    // PG内の有効論理ページ参照数

    FTL_OPENPG_INFO  opg_info;  // オープンPG情報
    FTL_PG_OADDR     next_ofs;  // 次書込先アドレス
    PB_INFO**        pb_list; // 構成ブロック情報

    std::deque<REVERSE_INFO> p2l_tbl; // 逆引き
    FTL_PG_OADDR  next_rewr_index;    // 次ReWriteインデックス
    FTL_PG_OADDR  next_rewr_pgsect;   // 次ReWritePG内オフセットセクタ#

    QUEUE  que; // close que用

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
