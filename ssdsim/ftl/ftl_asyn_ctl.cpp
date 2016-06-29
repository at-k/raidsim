
#include "ftl_lp_info.h"
#include "ftl_lp_ctl.h"
#include "ftl_lp_func.h"
#include "ftl_asyn_ctl.h"


//#define MAX_PB_TGT_NUM_LC 10
//#define MAX_REQ_PAGE_NUM   4

bool FtlAsynReqInterface::Init( LP_INFO* _lp_info, FM_INFO* _fm_info, FtlInterface* _ftl_if )
{
    lp_info = _lp_info;
    fm_info = _fm_info;

    if( lp_info->pb_num == 0 )
        return false;

    // RCM開始閾値の設定
    rcm_start_th    = (FTL_PB_GADDR)(((lp_info->pb_num*lp_info->op_ratio)/100) * lp_info->rcm_th / 100 );
    if( rcm_start_th <= _ftl_if->GetOpenPG_Num() * FTL_PG_BLOCK_NUM )
    {// 最低でも2回分は確保
        rcm_start_th = _ftl_if->GetOpenPG_Num() * lp_info->pg_pb_num * 2;
    }

    // pre_free_pb_num = 0;

    return true;
}

// RCM開始閾値と，現在の空き＋消去待ち＋RCM予備群の差を，新規RCM対象ブロックとする
//
inline int FtlAsynReqInterface::GetNeededBlockNum()
{
    return  rcm_start_th - (
        (lp_info->apg.rcm_cmp_pg_que.size() + lp_info->apg.rcm_pg_que.size()) * lp_info->pg_pb_num
        + lp_info->pool.total_invalid_pb_count + lp_info->pool.total_free_pb_count );
}

bool FtlAsynReqInterface::ReloadRcmTarget(int tgt_num)
{
    PG_INFO* pg;

    if( tgt_num > 0 )
    {// 空きブロックとRCM予備ブロックの差が閾値以下
        QUEUE* que = end_item( &lp_info->apg.close_pg_que );
        QUEUE* que_tmp;

        while( tgt_num >= 0 && !is_que_end(que) )
        {// 後方から必要数分rcmターゲットに追加する

            pg = (PG_INFO*)get_data(que);
            pg->status = FTL_PGS_RCM;
            lp_info->apg.rcm_pg_que.push_back(pg);

            // 取り出して次のアイテムへ
            que_tmp = que_forward( que );
            remove_item( &lp_info->apg.close_pg_que, que );
            que = que_tmp;

            if( pg->vs_num == lp_info->pg_cw_num )
            {// 残有効数がフル，RCMする意味なし。おかしい。
                PrintMessage( LOG_TYPE_ERROR, "Error FtlAsynReqInterface : RCMリクエスト異常\n" );
                return false;
            }

            tgt_num -= lp_info->pg_pb_num;
        }
    }
    return true;
}

//-- ReWrite対象を取得
//   ついでにRCM実施判定も行う
int FtlAsynReqInterface::GetReWriteReq( REVERSE_INFO req_list[FTL_MAX_REWR_REQ] )
{
    uint32_t req_num = 0;
    int rcm_tgt_new_block = 0;

    PG_INFO* pg;

    // RCM閾値まで不足している分を取得
    rcm_tgt_new_block = GetNeededBlockNum();

    if( !ReloadRcmTarget( rcm_tgt_new_block ) )
        return -1;

    REVERSE_INFO rev;

    while( req_num != FTL_MAX_REWR_REQ && !lp_info->apg.rcm_pg_que.empty() )
    {
        pg = lp_info->apg.rcm_pg_que.front();
        rev = GetNextReWriteLPN( pg );

        if( rev.lpn != FTL_REWR_END )
        {
            req_list[req_num] = rev;
            req_num ++;
        }else
        {// 最後までやりきった
            lp_info->apg.rcm_pg_que.pop_front();
            lp_info->apg.rcm_cmp_pg_que.push_back( pg );
        }
    }

    return req_num;
}

// 消去対象取得(BUS,CEについてラウンドロビン)
// 取得されたブロックは無効キューからとりだされ，消去予約キューに入れられる
int FtlAsynReqInterface::GetEraseReq ( FTL_ERASE_REQ erase_req[FTL_MAX_ERASE_REQ] )
{
    static uint16_t nxt_bus  = 0; // 検査開始するバス番号：ラウンドロビンでチェック
    static uint16_t nxt_ce   = 0;

    uint32_t   tgt_block_num = 0; // 返値
    uint32_t   req_id = 0;
    PB_INFO*   pb;

    if( lp_info->pool.erase_resv_que.empty() )
    {// 消去要求は既に十分出ている
        return -1;
    }

    for( uint16_t k = 0; k < fm_info->GetCeNumPerBus() + 1; k++ )
    {// 全てのCEを順にチェック (+ もう1周)
        for( ; nxt_bus < fm_info->GetBusNum(); nxt_bus++ )
        {// 同一CEラインでバスをチェック
            if( !lp_info->pool.invalid_block_que[nxt_bus][nxt_ce].empty() &&
                !lp_info->pool.erase_resv_que.empty() )
            {
                // キューから取り出し
                pb = lp_info->pool.invalid_block_que[nxt_bus][nxt_ce].front();
                lp_info->pool.invalid_block_que[nxt_bus][nxt_ce].pop_front();
                lp_info->pool.total_invalid_pb_count --;

                // 消去予約
                req_id = lp_info->pool.erase_resv_que.front();
                lp_info->pool.erase_resv_que.pop_front();
                if( lp_info->pool.erase_resv_list[req_id] != NULL )
                {
                    PrintMessage( LOG_TYPE_ERROR, "Error GetEraseReq: 消去資源不整合\n" );
                    return -1;
                }

                lp_info->pool.erase_resv_list[req_id] = pb;
                erase_req[tgt_block_num].pb_id = pb->id;
                erase_req[tgt_block_num].reg_id = req_id;

                tgt_block_num ++;
                if( tgt_block_num == FTL_MAX_ERASE_REQ )
                {
                    nxt_bus = ROUNDROBIN( nxt_bus, fm_info->GetBusNum() );
                    if( nxt_bus == 0 )
                        nxt_ce  = ROUNDROBIN( nxt_ce, fm_info->GetCeNumPerBus() );
                    goto _ERASE_SEARCH_END;
                }
            }
        }

        nxt_bus = 0;
        nxt_ce  = ROUNDROBIN( nxt_ce, fm_info->GetCeNumPerBus() );
    }

_ERASE_SEARCH_END:

    return tgt_block_num;

}


// -- 逆引き系
//
// 次のReWrite対象論理ページ#を取得
inline REVERSE_INFO FtlAsynReqInterface::GetNextReWriteLPN( PG_INFO* pg_info )
{
    FTL_LP_GADDR lpn;
    REVERSE_INFO rev,rev_rtn;
    uint32_t     ofs;
    VPA_INFO     vpa;

    lpn = FTL_REWR_END;
    rev_rtn.lpn = FTL_REWR_END;

    while( pg_info->p2l_tbl.size() > pg_info->next_rewr_index )
    {// 論理=物理な有効ページを探す
        rev = pg_info->p2l_tbl[pg_info->next_rewr_index]; // 逆引き取得
        ofs = pg_info->next_rewr_pgsect;    // チェック対象の物理アドレス取得
        vpa = LpnToVpa( rev.lpn, lp_info ); // 正引き取得

        // インデックスを進める
        pg_info->next_rewr_index ++;
        //pg_info->next_rewr_pgsect += ExtractVpaLength(lpn);
        pg_info->next_rewr_pgsect += rev.len;

        if( vpa.pgn == pg_info->id && vpa.ofs == ofs ) // 逆引きと正引きの突き合わせ
        {
            rev_rtn = rev;
            break;
        }
    }

    return rev_rtn;
}

#if 0

bool GetRcmRequest( LP_INFO* l2p_info, uint32_t tgtlp_list[MAX_RCM_IO_CREATE_NUM], uint16_t* io_count )
{
    static uint16_t   next_pool = 0;  // ポーリングを開始するプール番号
    static uint32_t   tgtlp_list_tmp[MAX_REQ_PAGE_NUM];

    POOL_INFO* pool_info;
    uint16_t   pool_no;

    PB_INFO*   pb_info;
    QUEUE*     pb_que;

    *io_count = 0;  // 要求総数初期化

    for ( pool_no = 0; pool_no < l2p_info->pool_num; pool_no++ )
    {
        // ポーリング対象のプールを選択
        pool_info = &l2p_info->pool_list[next_pool];

        UpdatePoolRcmRequest( l2p_info, pool_info );

        // 全ての当該プールのRCM対象についてサーチ

        pb_que = begin_item( &pool_info->rcm_block_que );
        while ( !is_que_end( pb_que ) )
        {
            uint32_t needed_count;

            pb_info = (PB_INFO*)pb_que->item;

            needed_count = MAX_RCM_IO_CREATE_NUM - *io_count; // 必要数取得

            needed_count = SearchRcmTargetPage( l2p_info, pb_info, tgtlp_list_tmp, needed_count ); // 対象ページ数取得

            for ( uint32_t i = 0; i < needed_count ; i ++ )
            {
                tgtlp_list[*io_count] = tgtlp_list_tmp[i];  // 結果格納
                (*io_count) = (*io_count) + 1;              // インクリメント
            }

            if ( *io_count == MAX_RCM_IO_CREATE_NUM )
                goto __RCM_REQ_END;          // 終わり

            pb_que = next_back( pb_que ); // 次のキューへ
        }

        //-- ラウンドロビン
        next_pool ++;
        if ( next_pool == l2p_info->pool_num )  next_pool = 0;
    }
__RCM_REQ_END:

    return true;
}

//-- ブロックが減るたびにコール
void UpdatePoolRcmRequest( LP_INFO* l2p_info, POOL_INFO* pool_info )
{
    int tgt_pb_num = 0;
    uint32_t rank = 0;

    if ( pool_info->total_free_pb_count < l2p_info->pool_rc_th )
    {// 不足分あり
        //-- 必要数分計算
        // 閾値に届いていない分のうち，RCM登録済みのもの，既に無効化されたもの（の半分）を除く
        tgt_pb_num = (l2p_info->pool_rc_th - pool_info->total_free_pb_count) -
            (pool_info->rcm_block_que.count + (pool_info->total_invalid_pb_count / 2));
    }

    if ( tgt_pb_num <= 0 )
    {// 既に十分のブロックが登録済み
        return;
    }

    if ( tgt_pb_num <= 0 )
    {// 既に十分のブロックが登録済み
        return;
    }

    // 不足分をランキング上のチャンクから補う
    SearchRcmTargetPB ( l2p_info, pool_info, tgt_pb_num );

    return;
}

uint32_t SearchRcmTargetPB ( LP_INFO* l2p_info, POOL_INFO* pool_info, uint32_t needed_count )
{
    uint32_t   pb_num;
    uint16_t   rank;
    QUEUE*     pb_que;

    pb_num = 0;

    // 対象ブロックサーチ
    for( rank = 0; rank < CLOSE_BLOCK_RANK; rank++ )
    {
        while( pool_info->close_block_que[rank].count != 0 )
        {
            // 対象アイテム取得
            pb_que = begin_item( &pool_info->close_block_que[rank] );

            // RCM対象ブロックとして登録
            SetPBasRcmTarget( l2p_info, ((PB_INFO*)(pb_que->item))->pb_id );

            pb_num ++; // ターゲット目標数更新

            if( pb_num == needed_count )
            {// 十分たまったので終了

                // 当該LCについては予定分たまったので抜ける
                goto __SEARCH_END;
            }
        }
    }
__SEARCH_END:

    return pb_num;
}

uint32_t SearchRcmTargetPage ( LP_INFO* l2p_info, PB_INFO* pb_info, uint32_t lptgt_list[MAX_REQ_PAGE_NUM], uint32_t needed_count )
{
    uint16_t ppo      = 0;
    uchar    bmap     = 0;
    uint32_t io_count = 0;

    uint16_t start_byte = (pb_info->rcm_search_pos) / ((sizeof(uchar)*8)); // 検査開始位置バイト数
    uint16_t start_offs = (pb_info->rcm_search_pos) % ((sizeof(uchar)*8)); // 検査開始位置オフセット

    uint16_t offs_count = start_offs;

    if ( needed_count > MAX_REQ_PAGE_NUM )
        needed_count = MAX_REQ_PAGE_NUM;

    for( uint16_t i = start_byte; i < VP_BITMAP_BYTES; i ++ )
    {
        bmap = pb_info->vp_bitmap[i]; // 検査対象ビットマップ取得
        ppo  = i * 8;                 // 開始ページオフセット番号

        while( bmap != 0 && io_count < needed_count )
        {

            // 最初のビットマップの位置合わせ。
            if ( offs_count )
            {
                offs_count --;
                bmap = bmap >> 1;
                ppo++;
                continue;
            }

            uint32_t lpn = 0;

            if( bmap & 0x01 )
            {

                if( !GetLPNfromPPO_PBN( ppo, pb_info->pb_id, &lpn, l2p_info ) )  // チャンク内オフセット計算
                    return false;

                lptgt_list[io_count] = lpn;       // 結果格納

                pb_info->rcm_search_pos = ppo + 1;// 次回検査開始位置

                io_count++;

            }
            bmap = bmap >> 1;
            ppo++;

        }

        offs_count = 0;

        // 所望の数だけ取得できればブレイク。
        if( io_count == needed_count )
        {
            break;
        }
    }

    return io_count;
}

#endif
