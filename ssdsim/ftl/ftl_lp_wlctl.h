#ifndef FTL_LP_WLCTL_H
#define FTL_LP_WLCTL_H

#include "ftl_lp_ctl.h"
#include "ftl_asyn_ctl.h"

enum WL_TYPE{
    WL_PROPOSE = 0,
	WL_STEPPING = 1,
	WL_RCM_FIX = 2,

	WL_ROUNDROBIN = 10

};

// 論物更新が絡む系の処理（ページライト・消去関係）IF
class FtlInterfaceWL : public FtlInterface
{
	public:
		FtlInterfaceWL();
		virtual ~FtlInterfaceWL();

		virtual bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info );

		//-- 書き込み先パリティグループ取得関数
		//   引数：ライトターゲットタイプ（host / rc / rf)
		//         取得パリティグループ番号
		//   返値：書き込み先パリティグループが存在しなければ無効IDを返す
		virtual FTL_PG_GADDR GetOpenPG ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no );

		virtual uint32_t GetOpenPG_Num();

		//-- FTLの中身ダンプ
		//   吐き出し
		virtual void Dump( std::ofstream& write_file );

	protected:

		typedef struct {// LP毎統計情報管理構造体
			uchar     attr;   // 所属W頻度クラス
			uint32_t  w_count; // 実際の上位からのライト数
			uint32_t  rcm_count; // ReWrite回数
			uint32_t  attr_sum; // 所属W頻度クラスの和
		} LP_WL_INFO;

		WL_TYPE wl_type;
		uint32_t ttl_w_count;

		// ブロック状態遷移関数 オープン→クローズ
		// 引数：PG情報
		virtual bool OpenToClose ( PG_INFO* pg_info );

		LP_WL_INFO* lp_wl_info;

};

class FtlAsynReqInterfaceWL : public FtlAsynReqInterface
{
	public:
		virtual bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info, FtlInterface* _ftl_if );

	protected:
		//-- CloseキューからRCM対象PGを探して
		//   RCM対象キューに遷移させる
		virtual bool ReloadRcmTarget( int tgt_num );


};

#endif
