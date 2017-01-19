#ifndef LPTRANS__H
#define LPTRANS__H

#include <math.h>
#include "../phy/fm_macro_def.h"
#include "ftl_lp_info.h"
#include "pbque.h"


// �_���v�Z�E�Q�ƁF
//   Calc�֐��͌v�Z�CGet�֐��̓e�[�u���Q�Ƃ���
//   �_���̕ύX�͂����ł͒񋟂��Ȃ�

// -- �Z�N�^ <-> �_���y�[�W
inline uint32_t  HlbaToLpn( uint64_t hlba )  { return (uint32_t)(floor( (double)hlba / FTL_SECTS_PER_LP )); }
inline uint64_t  LpnToHlba( uint32_t lpn)    { return lpn * FTL_SECTS_PER_LP; }
inline bool      IsPPAligned( uint64_t hlba) { return (hlba % FTL_SECTS_PER_LP) == 0 ? true : false; }

#define ROUNDROBIN(x,y) ( ( (x) + 1 ) == (y) ? 0 : (x) + 1 )

// -- �����y�[�W#�ƕ����u���b�N#�̊֌W�Â�
// �����y�[�W#���畨���u���b�N���y�[�W�I�t�Z�b�g���Z�o
inline uint16_t  PpnToPpo( uint32_t ppn )
{
    return ppn % PP_PER_PB;
}

// �����u���b�N#�ƕ����u���b�N���y�[�W�I�t�Z�b�g���畨���y�[�W#���Z�o
inline uint32_t  PbnAndPpoToPpn( uint32_t pbn, uint16_t ppo )
{
    return pbn * PP_PER_PB + ppo;
}

// �����y�[�W#���畨���u���b�N#�擾
inline uint32_t  PpnToPbn( uint32_t ppn )
{
    return ppn / PP_PER_PB;
}

// -- �������n
//
// �_�����C���e�[�u��
inline VPA_INFO LpnToVpa( uint32_t lpn, LP_INFO* lp_tbl )
{
    return lp_tbl->l2p_tbl[lpn];
}

// �z�X�g�R�}���h����_���y�[�W�R�}���h�ւ̕ϊ�
//
inline void HostIoToFMIo(
    uint64_t hlba, uint32_t length, uint32_t* start_lp,
    uint32_t* end_lp, bool* start_rmw_flag, bool* end_rmw_flag )
{
    *start_lp = HlbaToLpn( hlba );          // �J�n�y�[�W
    *end_lp   = HlbaToLpn( hlba + length ); // �ŏI�y�[�W

    // RMW�t���O�ݒ�
    *start_rmw_flag = ( !IsPPAligned( hlba ) ); // �J�n�y�[�W

    if( !IsPPAligned( hlba + length ) )
    {
        *end_rmw_flag = true; // �ǂݏo���K�v
    }
    else
    {
        *end_rmw_flag = false; // RMW�s�v
    }
}

#endif
