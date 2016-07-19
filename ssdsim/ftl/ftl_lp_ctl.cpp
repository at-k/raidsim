
#include "common_def.h"

#include "ftl_lp_ctl.h"
#include "ftl_lp_info.h"
#include "ftl_lp_func.h"
//#include "ftl_asyn_ctl.h"
#include "phy/fm_arch_info.h"

#include <algorithm>

FtlInterface::FtlInterface()
{
}
FtlInterface::~FtlInterface()
{
}

//--オープンPG取得関数
//  留意点：取得されるOpenPGは空きが不十分かもしれない（Lengthに関する検証をしていない）
//          Lengthが不定な(例えば圧縮など)ケースを導入する場合，ここ修正必要。
FTL_PG_GADDR FtlInterface::GetOpenPG ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no )
{
    static uchar hostpg_index = 0;
    static uchar rcmpg_index  = 0;

    FTL_PG_GADDR pg_no;

    if( type == FMT_HOST_WR )
    {// ホストライト
        if( lp_info->apg.host_openpg[ hostpg_index ] == NULL )
        {
            lp_info->apg.host_openpg[ hostpg_index ] = BuildPG( type, hostpg_index );
            if( lp_info->apg.host_openpg[ hostpg_index ] == NULL ) // 取得失敗
                return FTL_PG_INVALID_ID;
        }
        pg_no = lp_info->apg.host_openpg[ hostpg_index ]->id; // 割り当て
        hostpg_index = ROUNDROBIN( hostpg_index, FTL_OPENPG_NUM );
    }
    else if( type == FMT_RCM_RWR )
    {// RCMによるReWrite
        if( lp_info->apg.rcm_openpg[ rcmpg_index ] == NULL )
        {
            lp_info->apg.rcm_openpg[ rcmpg_index ] = BuildPG( type, rcmpg_index );
            if( lp_info->apg.rcm_openpg[ rcmpg_index ] == NULL ) // 取得失敗
                return FTL_PG_INVALID_ID;
        }
        pg_no = lp_info->apg.rcm_openpg[ rcmpg_index ]->id; // 割り当て
        rcmpg_index = ROUNDROBIN( rcmpg_index, FTL_OPENPG_NUM );
    }
    else
    {// RFは未サポート
        return FTL_PG_INVALID_ID;
    }

    return pg_no;
}


//-- 論物更新
bool FtlInterface::L2P_Update ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no, uchar len, FTL_PG_GADDR pg_no )
{
    PG_INFO* pg_dst_info;   // 書込先パリティグループ
    PG_INFO* pg_src_info;   // 前のパリティグループ
    //VPA_INFO vpa_dst;       // 割り付け先仮想アドレス
    VPA_INFO vpa_src;       // 前の仮想アドレス
    REVERSE_INFO rev_info;  // 逆引き登録情報

    vpa_src = LpnToVpa( lp_no, lp_info );   // 旧アドレス取得
    pg_dst_info = &lp_info->pg_list[pg_no]; // 書き先のPG情報取得

    if( vpa_src.pgn != FTL_PG_INVALID_ID )
    {// 既に割り当て済み
        pg_src_info = &lp_info->pg_list[vpa_src.pgn]; // 旧PG情報取得
    }else
    {
        pg_src_info = NULL;
    }

    // ---- エラー処理
    if( pg_dst_info->status != FTL_PGS_OPEN )
    {// OpenしていないPGを指定
        PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: pg state violation\n" );
        return false;
    } else if( pg_dst_info->next_ofs + len > lp_info->pg_cw_num )
    {// 書き込む余地無いブロックを指定
        PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: no free space\n" );
        return false;
    } else if( pg_src_info != NULL && pg_src_info->vs_num < vpa_src.len )
    {// 旧PGの情報がマッチしない
        PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: unmatch write source information\n" );
        return false;
    }

    // ---- 書き先更新
    lp_info->l2p_tbl[ lp_no ].pgn = pg_no;
    lp_info->l2p_tbl[ lp_no ].len = len;
    lp_info->l2p_tbl[ lp_no ].ofs = pg_dst_info->next_ofs;

    // 逆引き作成
    rev_info.lpn = lp_no;
    rev_info.len = len;

    // 書き先PGの情報更新
    pg_dst_info->p2l_tbl.push_back( rev_info ); // 逆引き登録
    pg_dst_info->next_ofs += len;
    pg_dst_info->vs_num += len; // 有効セクタ数
    pg_dst_info->lp_num ++;     // 参照論理ページ数

    if( pg_dst_info->next_ofs + FTL_SECTS_PER_LP >= lp_info->pg_cw_num*SECTS_PER_CW )
    {// close pg remaining one Lp space
        if( OpenToClose( pg_dst_info ) == false )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: ブロックライト完遷移失敗\n" );
            return false;
        }
    }

    // ---- 書き元更新
    if( pg_src_info != NULL )
    {
        pg_src_info->vs_num -= vpa_src.len;
        pg_src_info->lp_num --;

        if( pg_src_info->vs_num != 0 )
        {
            if( pg_src_info->status == FTL_PGS_CLOSE )
            {// close PG
                if( UpdateCloseRanking( pg_src_info ) == false )
                {
                    PrintMessage( LOG_TYPE_ERROR, "Error L2p_Update: closeキュー更新失敗\n");
                    return false;
                }
            }
        }else
        {
            if( pg_src_info->status != FTL_PGS_OPEN )
            {// 全無効化
                if( CloseToInvalid( pg_src_info ) == false )
                {
                    PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: close->invalid失敗\n");
                    return false;
                }
            }
        }
    }

    /* ビットマップ管理は今はしない */

    return true;
}

bool FtlInterface::ErasePB ( FTL_ERASE_REQ erase_req )
{
    PB_INFO*   pb_info;   // 対象ブロック情報

    pb_info = &lp_info->pb_list[erase_req.pb_id];

    if( pb_info != lp_info->pool.erase_resv_list[erase_req.reg_id] )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error ErasePB : 消去資源不整合\n" );
        return false;
    }

    // 資源解放
    lp_info->pool.erase_resv_list[erase_req.reg_id] = NULL;
    lp_info->pool.erase_resv_que.push_back( erase_req.reg_id );

    if( pb_info->pg_no != FTL_PG_INVALID_ID )
    {// PGに所属しているPBが選ばれている
        PrintMessage( LOG_TYPE_ERROR, "Error ErasePB: 不適なブロック\n" );
        return false;
    }

    // 消去情報を更新
    pb_info->erase_count++;
    // pb_info->last_erase_time =

    if( InvalidToFree( pb_info ) == false )
    {// なんか失敗
        return false;
    }

    return true;
}

bool FtlInterface::InvalidToFree ( PB_INFO* pb_info )
{
    if( pb_info->status != FTL_PBS_INVALID )
    {// 状態がおかしい
        PrintMessage( LOG_TYPE_ERROR, "Error InvalidToFree: ブロック状態異常\n" );
        return false;
    }

    pb_info->status = FTL_PBS_FREE;

    // フリーキューへエンキュー, とりあえずFIFO...寿命なんか知らない。
    lp_info->pool.free_block_que[pb_info->bus_no][pb_info->ce_no].push_back(pb_info);

    pb_info->status = FTL_PBS_FREE;       // 状態変更
    lp_info->pool.total_free_pb_count ++; // 全空きブロック数を更新

    return true;
}

// 空きブロックから必要数分取得してパリティグループを構成
// typeは今のところ未使用
//
PG_INFO* FtlInterface::BuildPG ( FTL_FMIO_TYPE type, uint32_t id )
{
    PG_INFO* pg;
    PB_INFO* pb;

    if( lp_info->free_pg_que.empty() )
    {// 空きPGが無い
        PrintMessage( LOG_TYPE_ERROR, "Error BuildPG: no free space\n" );
        return NULL;
    }
    // 空きを取り出し & 削除
    pg = lp_info->free_pg_que.front();
    lp_info->free_pg_que.pop_front(); // キューから削除
    pg->status = FTL_PGS_OPEN;        // 状態変更
    pg->opg_info.id = id;             // オープン情報追加
    pg->opg_info.type = type;

    // ブロック取りだし
    for( uint32_t i = 0; i < lp_info->pg_pb_num; i++ )
    {
        static int aho = 0;
        if( type == FMT_RCM_RWR )
            aho ++;
        if( aho == 8 )
            aho = 0;

        pb = GetFreePB( type );
        if( pb == NULL )
        {
            if( lp_info->pool.total_free_pb_count == 0 )
            {// 空きブロック無し，ブロック枯渇
                PrintMessage( LOG_TYPE_ERROR, "Error BuildPG: no free space\n" );
            }else
            {
                PrintMessage( LOG_TYPE_ERROR, "Error BuildPG: fail to get block\n" );
            }
            return NULL;
        }
        if( pb->pg_no != FTL_PG_INVALID_ID || pb->status != FTL_PBS_FREE )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error BuildPG: block state violation\n");
            return NULL;
        }

        pg->pb_list[i] = pb;
        pb->pg_no = pg->id;
        pb->status = FTL_PBS_BIND;

    }

    return pg;
}

// 空きブロックを1つ取得 バス方向にラウンドロビン
// typeは今のところ未使用
PB_INFO* FtlInterface::GetFreePB( FTL_FMIO_TYPE type )
{
    static uint16_t nxt_bus = 0;
    static uint16_t nxt_ce  = 0;

    PB_INFO* pb_info = NULL;

    // 検索開始
    for( uint16_t k = 0; k < fm_info->GetCeNumPerBus() + 1; k++ )
    {// 全てのCEを順にチェック( + 最後にもう1周)
        for( ; nxt_bus < fm_info->GetBusNum(); nxt_bus++ )
        {// CEラインでバスをチェック
            if( !lp_info->pool.free_block_que[nxt_bus][nxt_ce].empty() )
            {// ブロックが見つかった

                // ブロックの取り出し 先頭ブロックから取得
                pb_info = lp_info->pool.free_block_que[nxt_bus][nxt_ce].front();
                lp_info->pool.free_block_que[nxt_bus][nxt_ce].pop_front();

                lp_info->pool.total_free_pb_count --;

                nxt_bus = ROUNDROBIN( nxt_bus, fm_info->GetBusNum() );
                if( nxt_bus == 0 )
                    nxt_ce = ROUNDROBIN( nxt_ce, fm_info->GetCeNumPerBus() );
                goto __SEL_PB_END;
            }
        }

        nxt_bus = 0;
        nxt_ce = ROUNDROBIN( nxt_ce, fm_info->GetCeNumPerBus() );
    }

__SEL_PB_END:

    return pb_info;
}


bool FtlInterface::OpenToClose ( PG_INFO* pg_info )
{
    if( pg_info->status != FTL_PGS_OPEN )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: PG状態不整合\n" );
        return false;
    }

    if( pg_info->opg_info.type == FMT_HOST_WR )
    {// ホストライト用
        if( lp_info->apg.host_openpg[pg_info->opg_info.id] != pg_info )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: 対象Host PG情報不整合\n" );
            return false;
        }

        // オープンPG登録解除
        lp_info->apg.host_openpg[pg_info->opg_info.id] = NULL;
    }
    else if( pg_info->opg_info.type == FMT_RCM_RWR )
    {// リライト用
        if( lp_info->apg.rcm_openpg[pg_info->opg_info.id] != pg_info )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: 対象RCM PG情報不整合\n" );
            return false;
        }

        // オープンPG登録解除
        lp_info->apg.rcm_openpg[pg_info->opg_info.id] = NULL;
    }
    else
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: 不明なライトタイプ\n" );
        return false;
    }

    // クローズ登録
    pg_info->status = FTL_PGS_CLOSE;
    if( UpdateCloseRanking( pg_info ) == false )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: closeキュー更新失敗\n" );
        return false;
    }

    return true;
}

// rcm->invalid以外のパスは削除。
bool FtlInterface::CloseToInvalid( PG_INFO* pg_info )
{
    if( (pg_info->status != FTL_PGS_RCM && pg_info->status != FTL_PGS_CLOSE) || pg_info->vs_num != 0 )
    {// !rcm or !close  or そもそも有効データ抱えてる場合
        PrintMessage( LOG_TYPE_ERROR, "Error CloseToInvalid: 状態不整合\n" );
        return false;
    }

    // ブロック解体&無効へ追加
    PB_INFO* pb;
    for( uint32_t i = 0; i < lp_info->pg_pb_num; i++ )
    {
        pb = pg_info->pb_list[i];
        if( pb == NULL || pb->status != FTL_PBS_BIND )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error CloseToInvalid: ブロック状態不整合\n");
            return false;
        }

        pg_info->pb_list[i] = NULL;
        pb->status = FTL_PBS_INVALID;
        pb->pg_no  = FTL_PG_INVALID_ID;

        lp_info->pool.invalid_block_que[pb->bus_no][pb->ce_no].push_back(pb);
        lp_info->pool.total_invalid_pb_count++;

    }

    //-- キューから追い出し
    if( pg_info->status == FTL_PGS_RCM )
    {// rcmキュー
        PG_STL_ITR it = find( lp_info->apg.rcm_cmp_pg_que.begin(), lp_info->apg.rcm_cmp_pg_que.end(), pg_info );
        if( it != lp_info->apg.rcm_cmp_pg_que.end() )
        {
	        lp_info->apg.rcm_cmp_pg_que.erase( it );
        }else
        {
            it = find( lp_info->apg.rcm_pg_que.begin(), lp_info->apg.rcm_pg_que.end(), pg_info );
            if( it != lp_info->apg.rcm_pg_que.end() )
            {
                lp_info->apg.rcm_pg_que.erase( it );
            }else
            {
                PrintMessage( LOG_TYPE_ERROR, "Error CloseToInvalid: 対象(rcm) not found\n");
                return false;
            }
        }
    } else
    {// closeキュー
        if( pg_info->que.head != &lp_info->apg.close_pg_que )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error CloseToInvalid: 対象(close) not found\n");
            return false;
        }
        remove_item( &lp_info->apg.close_pg_que, &pg_info->que );
    }

    // PG初期化
    lp_info->InitPG( pg_info );
    lp_info->free_pg_que.push_back( pg_info );  // 空きPG資源を各プールに追加

    return true;
}

//-- 有効論理ページすべてに1回IOを実施する
bool FtlInterface::Format(bool enable_comp, double comp_ratio)
{
    FTL_PG_GADDR addr;
	uint32_t len = FTL_SECTS_PER_LP;
	if( enable_comp )
		len *= comp_ratio;

    for( uint32_t lp_no = 0; lp_no < lp_info->lp_num; lp_no++ )
    {
        addr = GetOpenPG( FMT_HOST_WR, lp_no );
        if( addr == FTL_PG_INVALID_ID )
            return false;

        if ( L2P_Update( FMT_HOST_WR, lp_no, len, addr) == false )
            return false;
    }

	return true;
}

//-- プールにブロック追加
bool FtlInterface::InitialAdd ( POOL_INFO* pool_info, PB_INFO* pb_info )
{
    // PB総数をカウント
    pool_info->pb_num++;

    //-- フリーキューへエンキュー
    pool_info->free_block_que[pb_info->bus_no][pb_info->ce_no].push_back(pb_info);
    pool_info->total_free_pb_count ++; // 全空きブロック数を更新

    return true;
}

bool FtlInterface::UpdateCloseRanking( PG_INFO* pg_info )
{
    QUEUE*   pos;
    PG_INFO* pg_tmp;

    // 挿入ソート
    if( pg_info->que.head == NULL )
    {// 状態遷移直後
        pos = begin_item( &lp_info->apg.close_pg_que );
    }else
    {// 接続先の更新
        pos = que_back( &pg_info->que );
        remove_item( &lp_info->apg.close_pg_que, &pg_info->que );
    }

    while( !is_que_end( pos ) )
    {// 無効数が降順のキューをサーチ(後ろの方が無効率が高い)
        pg_tmp = (PG_INFO*)get_data(pos);
        if( pg_info->vs_num >= pg_tmp->vs_num )
            break;

        pos = que_back( pos );
    }

    insert_forward( &lp_info->apg.close_pg_que, pos, &pg_info->que );

    return true;
}

void FtlInterface::Dump(std::ofstream& os )
{
	os <<  "LP, map" << std::endl;

    for( uint32_t i = 0; i < lp_info->lp_num;  i++ )
    {
		os << i << "," <<
			lp_info->l2p_tbl[i].pgn << "," <<
			lp_info->l2p_tbl[i].ofs << "," <<
			+lp_info->l2p_tbl[i].len << "," <<
			+lp_info->l2p_tbl[i].rcm_count << std::endl;
	}

	os << std::endl;
}

