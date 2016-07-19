#ifndef L2PTBL_MNG_H
#define L2PTBL_MNG_H

#include <fstream>

#include "common_def.h"
#include "ftl_lp_info.h"
#include "ftl_asyn_ctl.h"

// 論物更新が絡む系の処理（ページライト・消去関係）IF
class FtlInterface
{
	public:
		FtlInterface();
		virtual ~FtlInterface();

		//bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info, FtlAsynReqInterface* _asyn_ctl ) {
		virtual bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info ) {
			lp_info = _lp_info;
			fm_info = _fm_info;
			//asyn_ctl = _asyn_ctl;
			return true;
		}

		//-- 書き込み先パリティグループ取得関数
		//   引数：ライトターゲットタイプ（host / rc / rf)
		//         取得パリティグループ番号
		//   返値：書き込み先パリティグループが存在しなければ無効IDを返す
		virtual FTL_PG_GADDR GetOpenPG ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no );

		virtual uint32_t GetOpenPG_Num(){ return FTL_OPENPG_NUM; }

		//-- 論物更新関数
		//   引数：IO種別
		//         変更対象の論理ページ
		//         書き込み先の物理PG番号
		//   返値：失敗すれば，未割り当てを返す
		virtual bool L2P_Update ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no, uchar len, FTL_PG_GADDR pg_no );

		//-- ブロック消去関数
		//   引数：消去対象の物理ブロック番号
		//   返値：対象ブロックの状態が異常であればfalseを返す
		bool ErasePB ( FTL_ERASE_REQ pb_no );

		//-- ブロック初期追加
		//    入力：追加先プール
		//          追加対象物理ブロック
		bool InitialAdd ( POOL_INFO* pool_info, PB_INFO* pb_info );

		//-- FM領域初期化
		//   シーケンシャルに全領域1回ライト
		bool Format(bool enable_comp = false, double comp_ratio = 1.0);

		//-- FTLの中身ダンプ
		virtual void Dump( std::ofstream& write_file );

	protected:
		LP_INFO* lp_info;
		FM_INFO* fm_info;

		// 引数：空きブロック補充対象のプール，補充対象ブロック種別
		virtual bool InvalidToFree ( PB_INFO* pb_info );

		// ブロック補充関数
		// 引数：補充対象ブロック種別, 登録番号
		virtual PG_INFO* BuildPG ( FTL_FMIO_TYPE type, uint32_t id );

		// 空きブロック選択関数，パラメータの更新も行う
		// 引数：種別
		virtual PB_INFO* GetFreePB ( FTL_FMIO_TYPE type );

		// ブロック状態遷移関数 クローズ→無効
		// 引数：PG情報
		virtual bool CloseToInvalid ( PG_INFO* pg_info );

		// ブロック状態遷移関数 オープン→クローズ
		// 引数：PG情報
		virtual bool OpenToClose ( PG_INFO* pg_info );

		// クローズキュー内のブロックランキング更新
		virtual bool UpdateCloseRanking ( PG_INFO* pg_info );
};

#endif
