#ifndef LPTRANS__H
#define LPTRANS__H

#include <math.h>
#include "phy/fm_macro_def.h"
#include "ftl_lp_info.h"
#include "pbque.h"


// 論物計算・参照：
//   Calc関数は計算，Get関数はテーブル参照あり
//   論物の変更はここでは提供しない

// -- セクタ <-> 論理ページ
inline uint32_t  HlbaToLpn( uint64_t hlba )  { return (uint32_t)(floor( (double)hlba / FTL_SECTS_PER_LP )); }
inline uint64_t  LpnToHlba( uint32_t lpn)    { return lpn * FTL_SECTS_PER_LP; }
inline bool      IsPPAligned( uint64_t hlba) { return (hlba % FTL_SECTS_PER_LP) == 0 ? true : false; }

#define ROUNDROBIN(x,y) ( ( (x) + 1 ) == (y) ? 0 : (x) + 1 )

// -- 物理ページ#と物理ブロック#の関係づけ
// 物理ページ#から物理ブロック内ページオフセットを算出
inline uint16_t  PpnToPpo( uint32_t ppn )
{
    return ppn % PP_PER_PB;
}

// 物理ブロック#と物理ブロック内ページオフセットから物理ページ#を算出
inline uint32_t  PbnAndPpoToPpn( uint32_t pbn, uint16_t ppo )
{
    return pbn * PP_PER_PB + ppo;
}

// 物理ページ#から物理ブロック#取得
inline uint32_t  PpnToPbn( uint32_t ppn )
{
    return ppn / PP_PER_PB;
}

// -- 正引き系
//
// 論物メインテーブル
inline VPA_INFO LpnToVpa( uint32_t lpn, LP_INFO* lp_tbl )
{
    return lp_tbl->l2p_tbl[lpn];
}
#if 0
// VPAから物理CW位置情報取得(ストライプ跨がり＆2ページ以上跨がり禁止)
inline bool VpaToCw( VPA_INFO* vpa, CW_INFO* cw_start, CW_INFO* cw_end, LP_INFO* lp_tbl )
{

    // 開始位置
    uint32_t pg_vbn = (vpa->ofs / CW_PER_PP) % FTL_PG_BLOCK_NUM;     // PG内仮想ブロック番取得
    cw_start->pbn = lp_tbl->pg_list[ vpa->pgn ].pb_list[pg_vbn]->id; // 実ブロック番号取得
    cw_start->ppo = (vpa->ofs / CW_PER_PP) / FTL_PG_BLOCK_NUM;       // ブロック内ページオフセット番号取得
    cw_start->cwo = vpa->ofs % CW_PER_PP;                            // ページCW内オフセット番号取得

    if( cw_start->cwo + vpa->len > CW_PER_PP )
    {// 跨がり有り
        cw_start->len = CW_PER_PP - cw_start->cwo;

        pg_vbn ++;

        if( pg_vbn >= FTL_PG_BLOCK_NUM - 1 ) // ストライプ跨がり禁止
            return false;

        cw_end->pbn = lp_tbl->pg_list[ vpa->pgn ].pb_list[pg_vbn]->id;
        cw_end->ppo = cw_start->ppo;
        cw_end->cwo = 0;
        cw_end->len = cw_start->cwo + vpa->len - CW_PER_PP;

        if( cw_end->len > CW_PER_PP ) // 2ページ以上跨がり禁止
            return false;
    }
    else
    {// 跨がり無し
        cw_start->len = vpa->len;
        cw_end->len = 0;
    }
    
    return true;
}
#endif

// ホストコマンドから論理ページコマンドへの変換
// 
inline void HostIoToFMIo( 
    uint64_t hlba, uint32_t length, uint32_t* start_lp,
    uint32_t* end_lp, bool* start_rmw_flag, bool* end_rmw_flag )
{
    *start_lp = HlbaToLpn( hlba );          // 開始ページ
    *end_lp   = HlbaToLpn( hlba + length ); // 最終ページ

    // RMWフラグ設定
    *start_rmw_flag = ( !IsPPAligned( hlba ) ); // 開始ページ
    
    if( !IsPPAligned( hlba + length ) )
    {
        (*end_lp) ++;         // はみ出た分だけ余分に追加
        *end_rmw_flag = true; // 読み出し必要
    }
    else
    {
        *end_rmw_flag = false; // RMW不要
    }
}

#endif
