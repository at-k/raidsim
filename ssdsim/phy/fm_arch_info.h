#ifndef FMINFO___H
#define FMINFO___H

#include <nf_common.h>
#include "fm_macro_def.h"

// -- FM�̈ʒu�����Q�Ƃ��邽�߂̍\����
typedef struct {
    uint16_t   bus; // �o�X
    uint16_t  chip; // �`�b�v        ( bus���ʔ� )
    uint16_t   die; // �_�C          ( chip���ʔ� )
    uint32_t    pb; // �����u���b�N# (�u���b�N���ʔ� )
    uint16_t   ppo; // �����y�[�W�I�t�Z�b�g
} FMADDR;

// -- CE�̈ʒu�����Q�Ƃ��邽�߂̍\����
typedef struct {
    uint16_t dma_id; // DMA ID       ( PKG�����j�[�N )
    uint16_t ce_id;  // ChipEnableID ( ����DMA���ʔ� )
} CEADDR;

// -- FM�����A�N�Z�X�p�̃I�y�R�[�h
enum FM_OPCODE
{
    PAGE_WRITE,  // �y�[�W�P��Write
    PAGE_READ,   // �y�[�W�P��Read
    BLOCK_ERASE  // �u���b�N�P��Erase
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
    DIE_STATUS  status;     // �X�e�[�^�X

    CE_INFO*    ce_info;    // �q�����Ă���Chip Enable

    _DIE_INFO_* next_die;   // ����CE�ɐڑ����Ă��鎟�̃_�C

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

//-- FM�̍\�����C����сC�X�e�[�^�X�ibusy/idle)�Ǘ�
//   �X�e�[�^�X�Ǘ���FM_Access�N���X����̂ݎg�p���邱�ƁB
class FM_INFO
{
public:
    FM_INFO();
    ~FM_INFO();

    // FM���W���[���̏������֐�
    bool InitFlashModule(
        const uint16_t  bus_num_i       = DEF_BUS_NUM,
        const uint16_t  chip_per_bus_i  = DEF_CHIP_PER_BUS,
        const uint16_t  die_per_chip_i  = DEF_DIE_PER_CHIP,
        const uint64_t  byte_per_chip_i = DEF_SISE_GB_PER_CHIP*1024*1024*1024,
        const uint16_t  dma_num_i       = DEF_DMA_NUM,    // CE�̉������̐�
        const uint16_t  ce_per_bus_i    = DEF_CE_PER_BUS  // �o�X������ɐڑ�����CE��(CE�̏c�����̐�)
        );

    // get�֐�
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

    // �o�X�̏�Ԃ�ݒ�
    bool   SetBusStatus( uint16_t bus_id, BUS_STATUS status );

    // �o�X�̏�ԎQ��
    inline bool  IsBusBusy( uint16_t bus_id )
    {
        if( bus_info[bus_id].status == BUS_BUSY )
            return true;
        return false;
    }

    // CE�̏�Ԃ�ݒ�
    bool   SetCEStatus( CEADDR* ce_addr, CE_STATUS status );

    // ����CE�̓r�W�[���ǂ���
    inline bool   IsCEBusy( CEADDR* ce_addr )
    {
        if( ce_info[ce_addr->dma_id][ce_addr->ce_id].status == CE_BUSY )
            return true;
        return false;
    }

    // - �����u���b�N��Global ID(�ʂ��ԍ�)��FM�ʒu�Ƃ̑Ή��t����`
    inline void GetPBGIDFromFMAddress( FMADDR* addr, uint32_t* pb_gid  )
    {
        *pb_gid =
            (addr->bus  * pb_per_bus)  +
            (addr->chip * pb_per_chip) +
            (addr->die  * pb_per_die ) +
            (addr->pb);
    }

    // -- ��̋t����
    inline void GetPBFMAddressFromGID( uint32_t pb_gid, FMADDR* out )
    {
        out->bus  =  pb_gid / pb_per_bus;
        out->chip = (pb_gid % pb_per_bus)  / pb_per_chip;
        out->die  = (pb_gid % pb_per_chip) / pb_per_die;
        out->pb   = (pb_gid % pb_per_die);
    }

    // -- �_�C�̃A�h���X��CE�A�h���X�̑Ή��t����`
    inline void GetCEAddressFromDieAddress( FMADDR* die_addr, CEADDR* ce_addr )
    {
        ce_addr->dma_id = die_addr->bus / (bus_num / dma_num);  // �o�X�� / DMA��
        ce_addr->ce_id  = (die_addr->chip * die_per_chip + die_addr->die) / die_per_ce_per_bus; // �o�X���ł�ID
    }

    // -- �_�C�̃A�h���X����C����CE�ƌ������鉽�Ԗڂ̃o�X�����v�Z
    inline void GetBusOffsetFromDieAddress( FMADDR* die_addr, uint16_t* bus_id )
    {// ����CE�ɐڑ������o�X���Ŋ��������܂�
        *bus_id = die_addr->bus % ( bus_num / dma_num );
    }

private:
    BUS_INFO*  bus_info;     // �S�o�X�C�S�`�b�v�C�u���b�N�̐ڑ���ێ�
    CE_INFO**  ce_info;      // CE�̏�ԕێ�

    //-- FM�\�����
    uint16_t   bus_num;      // �S�o�X��
    uint16_t   chip_per_bus; // �o�X������̃`�b�v��
    uint16_t   die_per_chip; // �`�b�v������̃_�C��
    uint16_t   ce_per_bus;   // �o�X�������CE���i�c�����̐��j
    uint16_t   dma_num;      // �SDMA��         �iCE�̉������̐��j
    uint16_t   die_per_ce_per_bus;   // ����o�X�C����CE�̃_�C��

    //-- �u���b�N�����
    uint32_t  pb_per_bus;
    uint32_t  pb_per_chip;
    uint32_t  pb_per_die;

    //-- �Q�l���
    uint32_t  pb_num;   // �����u���b�N����
    uint32_t  pp_num;   // �����y�[�W����
    uint64_t  fm_byte;  // �S�̗e��(Byte)
    uint64_t  fm_sects; // �S�̃Z�N�^��
};

#endif
