#ifndef FMINFO___H
#define FMINFO___H

#include <nf_common.h>
#include "fm_macro_def.h"

// -- FMの位置情報を参照するための構造体
typedef struct {
    uint16_t   bus; // バス
    uint16_t  chip; // チップ        ( bus内通番 )
    uint16_t   die; // ダイ          ( chip内通番 )
    uint32_t    pb; // 物理ブロック# (ブロック内通番 )
    uint16_t   ppo; // 物理ページオフセット
} FMADDR;

// -- CEの位置情報を参照するための構造体
typedef struct {
    uint16_t dma_id; // DMA ID       ( PKG内ユニーク )
    uint16_t ce_id;  // ChipEnableID ( 同一DMA内通番 )
} CEADDR;

// -- FM物理アクセス用のオペコード
enum FM_OPCODE
{
    PAGE_WRITE,  // ページ単位Write
    PAGE_READ,   // ページ単位Read
    BLOCK_ERASE  // ブロック単位Erase
};

enum PB_STATUS {
    PB_IDLE = 1,
    PB_WRITE= 3,
    PB_READ = 4,
    PB_ERASE= 6
};

enum DIE_STATUS {
    DIE_IDLE     = 1,
    DIE_WREADY   = 2,
    DIE_WRITE    = 3,
    DIE_READ     = 4,
    DIE_READ_REG = 5,
    DIE_ERASE    = 6,
    DIE_READY    = 7
};

enum BUS_STATUS {
    BUS_IDLE = 1,
    BUS_BUSY = 2
};

enum CE_STATUS {
    CE_BUSY  = 1,
    CE_IDLE  = 2
};

enum READY_BUSY_STATUS {
    RB_READY = 1,
    RB_BUSY  = 2
};

/*typedef struct {
    uint32_t  id;
    PB_STATUS status;
} PB_INFO;*/

typedef struct {
    uint16_t  id;
    CE_STATUS status;
} CE_INFO;

typedef struct _DIE_INFO_ {
    uint16_t    id;         // ID
    DIE_STATUS  status;     // ステータス

    CE_INFO*    ce_info;    // 繋がっているChip Enable

    _DIE_INFO_* next_die;   // 同一CEに接続している次のダイ

    // for debug
    uint16_t  bus;
    uint16_t  chip;
} DIE_INFO;

typedef struct {
    uint16_t  id;
    DIE_INFO* dlist;
} CHIP_INFO;

typedef struct {
    uint16_t    id;
    BUS_STATUS  status;
    CHIP_INFO   *clist;
} BUS_INFO;

//-- FMの構成情報，および，ステータス（busy/idle)管理
//   ステータス管理はFM_Accessクラスからのみ使用すること。
class FM_INFO
{
public:
    FM_INFO();
    ~FM_INFO();

    // FMモジュールの初期化関数
    bool InitFlashModule(
        const uint16_t  bus_num_i       = DEF_BUS_NUM,
        const uint16_t  chip_per_bus_i  = DEF_CHIP_PER_BUS,
        const uint16_t  die_per_chip_i  = DEF_DIE_PER_CHIP,
        const uint64_t  byte_per_chip_i = DEF_SISE_GB_PER_CHIP*1024*1024*1024,
        const uint16_t  dma_num_i       = DEF_DMA_NUM,    // CEの横方向の数
        const uint16_t  ce_per_bus_i    = DEF_CE_PER_BUS  // バスあたりに接続するCE数(CEの縦方向の数)
        );

    // get関数
    inline uint16_t  GetBusNum()        const{ return bus_num;}
    inline uint16_t  GetChipNumPerBus() const{ return chip_per_bus;}
    inline uint16_t  GetDieNumPerChip() const{ return die_per_chip;}

    inline uint16_t  GetBusNumPerCe()   const{ return bus_num / dma_num;}
    inline uint16_t  GetCeNumPerBus()   const{ return ce_per_bus;}
    inline uint16_t  GetDMANum()        const{ return dma_num;}

    inline uint16_t  GetTotalDieNum()   const{ return bus_num * chip_per_bus * die_per_chip;}
    inline uint32_t  GetTotalCENum()    const{ return dma_num * ce_per_bus; }

    inline uint32_t  GetFMSizeGB()      const{ return (uint32_t)(fm_byte / 1024 / 1024 / 1024);}
    inline uint64_t  GetTotalSector()   const{ return fm_sects;}

    inline uint32_t  GetPBNum()         const{ return pb_num;}
    inline uint32_t  GetPPNum()         const{ return pp_num;}

    // バスの状態を設定
    bool   SetBusStatus( uint16_t bus_id, BUS_STATUS status );

    // バスの状態参照
    inline bool  IsBusBusy( uint16_t bus_id )
    {
        if( bus_info[bus_id].status == BUS_BUSY )
            return true;
        return false;
    }

    // CEの状態を設定
    bool   SetCEStatus( CEADDR* ce_addr, CE_STATUS status );

    // 現在CEはビジーかどうか
    inline bool   IsCEBusy( CEADDR* ce_addr )
    {
        if( ce_info[ce_addr->dma_id][ce_addr->ce_id].status == CE_BUSY )
            return true;
        return false;
    }

    // - 物理ブロックのGlobal ID(通し番号)とFM位置との対応付け定義
    inline void GetPBGIDFromFMAddress( FMADDR* addr, uint32_t* pb_gid  )
    {
        *pb_gid =
            (addr->bus  * pb_per_bus)  +
            (addr->chip * pb_per_chip) +
            (addr->die  * pb_per_die ) +
            (addr->pb);
    }

    // -- 上の逆引き
    inline void GetPBFMAddressFromGID( uint32_t pb_gid, FMADDR* out )
    {
        out->bus  =  pb_gid / pb_per_bus;
        out->chip = (pb_gid % pb_per_bus)  / pb_per_chip;
        out->die  = (pb_gid % pb_per_chip) / pb_per_die;
        out->pb   = (pb_gid % pb_per_die);
    }

    // -- ダイのアドレスとCEアドレスの対応付け定義
    inline void GetCEAddressFromDieAddress( FMADDR* die_addr, CEADDR* ce_addr )
    {
        ce_addr->dma_id = die_addr->bus / (bus_num / dma_num);  // バス数 / DMA数
        ce_addr->ce_id  = (die_addr->chip * die_per_chip + die_addr->die) / die_per_ce_per_bus; // バス内でのID
    }

    // -- ダイのアドレスから，同じCEと交差する何番目のバスかを計算
    inline void GetBusOffsetFromDieAddress( FMADDR* die_addr, uint16_t* bus_id )
    {// 同一CEに接続されるバス数で割ったあまり
        *bus_id = die_addr->bus % ( bus_num / dma_num );
    }

private:
    BUS_INFO*  bus_info;     // 全バス，全チップ，ブロックの接続を保持
    CE_INFO**  ce_info;      // CEの状態保持

    //-- FM構成情報
    uint16_t   bus_num;      // 全バス数
    uint16_t   chip_per_bus; // バスあたりのチップ数
    uint16_t   die_per_chip; // チップあたりのダイ数
    uint16_t   ce_per_bus;   // バスあたりのCE数（縦方向の数）
    uint16_t   dma_num;      // 全DMA数         （CEの横方向の数）
    uint16_t   die_per_ce_per_bus;   // 同一バス，同一CEのダイ数

    //-- ブロック数情報
    uint32_t  pb_per_bus;
    uint32_t  pb_per_chip;
    uint32_t  pb_per_die;

    //-- 参考情報
    uint32_t  pb_num;   // 物理ブロック総数
    uint32_t  pp_num;   // 物理ページ総数
    uint64_t  fm_byte;  // 全体容量(Byte)
    uint64_t  fm_sects; // 全体セクタ数
};

#endif
