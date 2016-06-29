#ifndef FM_ACCESS__H
#define FM_ACCESS__H

#include <deque>
#include "fm_arch_info.h"
#include "ftl/ftl_lp_info.h"

// FMアクセスのための情報
// トランザクション管理も兼ねる
typedef struct __FMIO_INFO
{
    uint64_t       io_id;    // 通し番号
    uint64_t       hio_id;   // ホストIO起因の場合は対応IDを格納。
    FM_OPCODE      op_code;  // write or read or erase
    FTL_FMIO_TYPE  req_type; // 要求発行元
    
    uint32_t  lp_no;  // op_code = ERASE時は無効
    uint32_t  pb_no;  // op_code = ERASE時のみ有効
    FMADDR   fm_addr; // 最終物理アドレス
    CEADDR   ce_addr; // 最終CEアドレス

    uint16_t* hostio_comp_wait_count;  // ホストIOが複数ページに分割された場合の共通カウント
    uint64_t  start_time; // 開始時間（エンキュー時間）

    //-- rcm支援
    bool         is_read_end;
    __FMIO_INFO* pair;
    
} FMIO_INFO;

// 予約するFM資源の種別
typedef enum {
    FM_DMA_DOWN_RSC, // DMA登り
    FM_DMA_UP_RSC,   // DMA下り
    FM_CE_RSC,       // CE Busy
    FM_NONE_RSC      // その他 N/A
} RSC_TYPE;

// 資源予約のための情報
typedef struct  
{
    RSC_TYPE type;            // ロック対象のリソース種別
    uint16_t dma_no;          // DMA番号
    CEADDR   ce_addr;         // CEアドレス
    uint64_t release_time_us; // 次回アイドルになる時刻(注意：相対時間指定)

    FMIO_INFO* fmio[2];       // ロックしているIOの情報..[2]なのはCEまとめ用。
} FM_RESV_INFO;

//-- ハードウェア資源確保のためのインターフェイスを提供
//   本クラス以外で直接FM_INFOのリソース制御関数を触らないこと
class FM_Access
{
public:

    FM_Access();
    ~FM_Access();
    
    bool Init( FM_INFO* fm_info ); // 初期化

    bool IsAvailable( FM_RESV_INFO* resv_info );                         // 指定されたリソースが利用可能か問い合わせ
    bool SetResourceBusy( FM_RESV_INFO* resv_info, uint64_t cur_time );  // 指定したリソースを指定時間ロック
    bool GetNextIdleResource( FM_RESV_INFO* resv_info );                 // 次時間にアイドルになるリソースを取得

private:

    FM_INFO*   fm_info;                // 実体はよそからもらう
    std::deque<FM_RESV_INFO> rsv_list; // 時間についてソートされているものとする。取り出しは先頭から
    
};


#endif
