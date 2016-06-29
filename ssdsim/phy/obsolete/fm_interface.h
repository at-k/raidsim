#ifndef FM_ACCESS__H
#define FM_ACCESS__H

#include <deque>
#include "fm_arch_info.h"
#include "ftl/ftl_lp_info.h"

// FM�A�N�Z�X�̂��߂̏��
// �g�����U�N�V�����Ǘ������˂�
typedef struct __FMIO_INFO
{
    uint64_t       io_id;    // �ʂ��ԍ�
    uint64_t       hio_id;   // �z�X�gIO�N���̏ꍇ�͑Ή�ID���i�[�B
    FM_OPCODE      op_code;  // write or read or erase
    FTL_FMIO_TYPE  req_type; // �v�����s��
    
    uint32_t  lp_no;  // op_code = ERASE���͖���
    uint32_t  pb_no;  // op_code = ERASE���̂ݗL��
    FMADDR   fm_addr; // �ŏI�����A�h���X
    CEADDR   ce_addr; // �ŏICE�A�h���X

    uint16_t* hostio_comp_wait_count;  // �z�X�gIO�������y�[�W�ɕ������ꂽ�ꍇ�̋��ʃJ�E���g
    uint64_t  start_time; // �J�n���ԁi�G���L���[���ԁj

    //-- rcm�x��
    bool         is_read_end;
    __FMIO_INFO* pair;
    
} FMIO_INFO;

// �\�񂷂�FM�����̎��
typedef enum {
    FM_DMA_DOWN_RSC, // DMA�o��
    FM_DMA_UP_RSC,   // DMA����
    FM_CE_RSC,       // CE Busy
    FM_NONE_RSC      // ���̑� N/A
} RSC_TYPE;

// �����\��̂��߂̏��
typedef struct  
{
    RSC_TYPE type;            // ���b�N�Ώۂ̃��\�[�X���
    uint16_t dma_no;          // DMA�ԍ�
    CEADDR   ce_addr;         // CE�A�h���X
    uint64_t release_time_us; // ����A�C�h���ɂȂ鎞��(���ӁF���Ύ��Ԏw��)

    FMIO_INFO* fmio[2];       // ���b�N���Ă���IO�̏��..[2]�Ȃ̂�CE�܂Ƃߗp�B
} FM_RESV_INFO;

//-- �n�[�h�E�F�A�����m�ۂ̂��߂̃C���^�[�t�F�C�X���
//   �{�N���X�ȊO�Œ���FM_INFO�̃��\�[�X����֐���G��Ȃ�����
class FM_Access
{
public:

    FM_Access();
    ~FM_Access();
    
    bool Init( FM_INFO* fm_info ); // ������

    bool IsAvailable( FM_RESV_INFO* resv_info );                         // �w�肳�ꂽ���\�[�X�����p�\���₢���킹
    bool SetResourceBusy( FM_RESV_INFO* resv_info, uint64_t cur_time );  // �w�肵�����\�[�X���w�莞�ԃ��b�N
    bool GetNextIdleResource( FM_RESV_INFO* resv_info );                 // �����ԂɃA�C�h���ɂȂ郊�\�[�X���擾

private:

    FM_INFO*   fm_info;                // ���̂͂悻������炤
    std::deque<FM_RESV_INFO> rsv_list; // ���Ԃɂ��ă\�[�g����Ă�����̂Ƃ���B���o���͐擪����
    
};


#endif
