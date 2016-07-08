
#include "ftl_lp_wlctl.h"
#include "ftl_lp_func.h"

#include <string.h>

// 各W頻度クラスのPGの最後にRCMされた時点の無効率情報
// 2つのクラスに共有されているのでとりあえず外に出した
typedef struct
{
    float    wa;
    uint32_t pgnum;
    uint32_t ecount;

} WFREQ_INFO;

WFREQ_INFO wf_info[FTL_OPENPG_WL_NUM];

//uint32_t WA_TABLE [ FTL_OPENPG_WL_NUM ];
//uint32_t WA_TABLE_COUNT [ FTL_OPENPG_WL_NUM ];

FtlInterfaceWL::FtlInterfaceWL()
{
    lp_wl_info = NULL;
}

FtlInterfaceWL::~FtlInterfaceWL()
{
    if( lp_wl_info != NULL )
        delete lp_wl_info;
}

//-- 初期化
//
bool FtlInterfaceWL::Init( LP_INFO* _lp_info, FM_INFO* _fm_info )
{
    // WAtableの初期化
    memset(wf_info, 0, sizeof(WFREQ_INFO)*FTL_OPENPG_WL_NUM);

    lp_wl_info = new LP_WL_INFO[ _lp_info->lp_num ];
    memset( lp_wl_info, 0, sizeof(LP_WL_INFO)*_lp_info->lp_num );

    wl_type = WL_ROUNDROBIN;
    ttl_w_count = 0;

    if( FTL_OPENPG_WL_NUM < 3 )
        return false;

    return FtlInterface::Init( _lp_info, _fm_info );
}

uint32_t FtlInterfaceWL::GetOpenPG_Num()
{
    uint32_t num = 0;

    if( wl_type == WL_PROPOSE || wl_type == WL_STEPPING || wl_type == WL_ROUNDROBIN )
        num = FTL_OPENPG_WL_NUM;
    else
        num = 3; // SYNC,RCM,RF

    return num;
}

//-- LP table dump
//
void FtlInterfaceWL::Dump(std::ofstream& os )
{
	os << "WL_TYPE: " << wl_type << std::endl;
	os << "ATTR, WA, Num.of PGs, Erace Count, Average Erace Count"  << std::endl;

	for( uint32_t i = 0; i < FTL_OPENPG_WL_NUM; i++ )
    {
        float avg_ec = (wf_info[i].pgnum != 0)? (float)wf_info[i].ecount / wf_info[i].pgnum : 0;
		os << i << "," << wf_info[i].wa << "," << wf_info[i].pgnum << ","
			<< wf_info[i].ecount << "," << avg_ec << std::endl;
        wf_info[i].ecount = 0;
    }

	os <<  "LP, ATTR, WCOUNT, RCMCOUNT, AVG_ATTR, WRATIO" << std::endl;

    float w_count_avg;
    w_count_avg = (float)ttl_w_count / lp_info->lp_num;

    float matching = 0;

    for( uint32_t i = 0; i < lp_info->lp_num;  i++ )
    {
        float attr_avg, w_ratio;
        if( lp_wl_info[i].w_count + lp_wl_info[i].rcm_count != 0)
            attr_avg = (float)lp_wl_info[i].attr_sum / (lp_wl_info[i].w_count + lp_wl_info[i].rcm_count);
        else
            attr_avg = lp_wl_info[i].attr;

        w_ratio = (float)lp_wl_info[i].w_count / w_count_avg;

		os << i << "," << +lp_wl_info[i].attr << "," << lp_wl_info[i].w_count << ","
			<< lp_wl_info[i].rcm_count << "," << attr_avg << "," << w_ratio << std::endl;

        //PrintMessage( key, "%d,%d,%d,%d, %.2f, %.2f\n",
        //    i, lp_wl_info[i].attr, lp_wl_info[i].w_count, lp_wl_info[i].rcm_count, attr_avg, w_ratio );

        // update matching
        matching += (attr_avg * w_ratio);

        // clear at every output
        lp_wl_info[i].w_count = 0;
        lp_wl_info[i].rcm_count = 0;
        lp_wl_info[i].attr_sum = 0;
    }

    ttl_w_count = 0;

	os << "matching = " << matching << std::endl;
    //PrintMessage( key, "# matching = %.2f\n", matching );
	os << std::endl;
}

//--オープンPG取得関数
//  留意点：取得されるOpenPGは空きが不十分かもしれない（Lengthに関する検証をしていない）
//          Lengthが不定な(例えば圧縮など)ケースを導入する場合，ここ修正必要。
FTL_PG_GADDR FtlInterfaceWL::GetOpenPG ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no )
{
    FTL_PG_GADDR pg_no, cur_pg_no;

    uchar cur_attr = FTL_OPENPG_WL_NUM;
    uchar nxt_attr = FTL_OPENPG_WL_NUM; // 最下位LV+1

    static uchar pg_index = 0;// type == roundrobin用

    if( lp_info->l2p_tbl[lp_no].ofs != FTL_LP_NOT_ASSIGNED )
    {
        cur_pg_no = lp_info->l2p_tbl[lp_no].pgn;
        cur_attr = lp_info->pg_list[cur_pg_no].attr;
    }

    if( type == FMT_HOST_WR )
    {// ホストライト
        lp_info->l2p_tbl[lp_no].rcm_count = 0;

        if( wl_type == WL_PROPOSE)
            nxt_attr = (cur_attr != 0)? cur_attr - 1 : cur_attr;
        else if( wl_type == WL_STEPPING )
        {
            if( lp_info->l2p_tbl[lp_no].ofs == FTL_LP_NOT_ASSIGNED )
                nxt_attr = FTL_OPENPG_WL_NUM - 1;
            else
                nxt_attr = 0;
        }
        else if( wl_type == WL_ROUNDROBIN)
        {
            nxt_attr = pg_index;
            pg_index = ROUNDROBIN(pg_index, FTL_OPENPG_WL_NUM);
        }
        else
        {
            if( lp_info->l2p_tbl[lp_no].ofs == FTL_LP_NOT_ASSIGNED )
                nxt_attr = FTL_OPENPG_WL_NUM - 1;
            else
                nxt_attr = 0;
        }

        // LPのWL情報更新…本来はLP_Updateで実施すべきだが。
        lp_wl_info[lp_no].w_count ++;
        ttl_w_count ++;
    }
    else if(type == FMT_RCM_RWR)
    {
        lp_info->l2p_tbl[lp_no].rcm_count ++;

        if( wl_type == WL_PROPOSE )
        {
            if( wf_info[cur_attr].wa < lp_info->l2p_tbl[lp_no].rcm_count )
                nxt_attr = (cur_attr < FTL_OPENPG_WL_NUM - 2)? cur_attr + 1 : cur_attr;
            else
                nxt_attr = cur_attr;
        }else if( wl_type == WL_STEPPING )
        {
            nxt_attr = (cur_attr < FTL_OPENPG_WL_NUM - 2)? cur_attr + 1 : cur_attr;
        }else if( wl_type == WL_ROUNDROBIN )
        {
            nxt_attr = pg_index;
            pg_index = ROUNDROBIN(pg_index, FTL_OPENPG_WL_NUM);
        }
        else
            nxt_attr = 1;

        lp_wl_info[lp_no].rcm_count ++;
    }
    else
    {// RFは未サポート
        return FTL_PG_INVALID_ID;
    }

    if( lp_info->apg.wl_openpg[nxt_attr] == NULL )
    {
        lp_info->apg.wl_openpg[ nxt_attr ] = BuildPG( type, nxt_attr );
        if( lp_info->apg.wl_openpg[ nxt_attr ] == NULL ) // 取得失敗
            return FTL_PG_INVALID_ID;
        lp_info->apg.wl_openpg[ nxt_attr ]->attr = nxt_attr;

        wf_info[nxt_attr].pgnum++;
        wf_info[nxt_attr].ecount++;
    }

    lp_wl_info[lp_no].attr = nxt_attr;
    lp_wl_info[lp_no].attr_sum += nxt_attr;

    pg_no = lp_info->apg.wl_openpg[ nxt_attr ]->id;

    return pg_no;
}


bool FtlInterfaceWL::OpenToClose ( PG_INFO* pg_info )
{
    if( pg_info->status != FTL_PGS_OPEN )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: PG状態不整合\n" );
        return false;
    }

    if( pg_info->attr >= FTL_OPENPG_WL_NUM )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: 属性不整合\n");
        return false;
    }

    if( lp_info->apg.wl_openpg[pg_info->attr] != pg_info )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: 対象Host PG情報不整合\n" );
        return false;
    }

    // オープンPG登録解除
    lp_info->apg.wl_openpg[pg_info->attr] = NULL;

    // クローズ登録
    pg_info->status = FTL_PGS_CLOSE;
    if( UpdateCloseRanking( pg_info ) == false )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: closeキュー更新失敗\n" );
        return false;
    }

    return true;
}

bool FtlAsynReqInterfaceWL::Init( LP_INFO* _lp_info, FM_INFO* _fm_info, FtlInterface* _ftl_if )
{

    if( ! FtlAsynReqInterface::Init( _lp_info, _fm_info, _ftl_if ) )
        return false;

    /*
    // RCM開始閾値だけ書き換え
    rcm_start_th    = (FTL_PB_GADDR)(((lp_info->pb_num*lp_info->op_ratio)/100) * lp_info->rcm_th / 100 );
    if( rcm_start_th <= FTL_OPENPG_WL_NUM * FTL_PG_BLOCK_NUM )
    {// 最低でも2回分は確保
        rcm_start_th = FTL_OPENPG_WL_NUM*lp_info->pg_pb_num*2;
    }
    */

    return true;
}

bool FtlAsynReqInterfaceWL::ReloadRcmTarget(int tgt_num)
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

            // WA記録
            wf_info[pg->attr].wa = (float)lp_info->pg_cw_num  / (lp_info->pg_cw_num - pg->vs_num);
            wf_info[pg->attr].pgnum --;

        }
    }

    return true;
}
