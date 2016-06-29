#include "fm_interface.h"


// �o�X�ECE�̃r�W�[���ԊǗ�
// ���̂͂����̃\�[�g�L���[�̂݁B���V���v��
FM_Access::FM_Access()
{
    fm_info = NULL;
}

FM_Access::~FM_Access()
{
    rsv_list.clear();
}

bool FM_Access::Init( FM_INFO* fm_info_i )
{
    fm_info = fm_info_i;

    return true;
}

bool FM_Access::IsAvailable( FM_RESV_INFO* resv_info )
{

    if( resv_info->type == FM_DMA_DOWN_RSC || resv_info->type == FM_DMA_UP_RSC )
    {// DMA�w��--�ʂɑS��d�ł͂Ȃ��̂ŁC��艺�蓯���B
        if ( fm_info->IsBusBusy( resv_info->dma_no ) )
        {
            return false;
        }
    }
    else if( resv_info->type == FM_CE_RSC )
    {// CE�w��
        if ( fm_info->IsCEBusy( &resv_info->ce_addr ) )
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return true;
}

bool FM_Access::SetResourceBusy( FM_RESV_INFO* resv_info, uint64_t cur_time )
{
    std::deque<FM_RESV_INFO>::iterator itr;

    if( IsAvailable( resv_info ) == false )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error at FM_Access::SetResourceBusy -- ���p�s�\�ȃ��\�[�X�ւ̃A�N�Z�X\n" );
        return false;
    }

    if( resv_info->type == FM_DMA_DOWN_RSC || resv_info->type == FM_DMA_UP_RSC )
    {// DMA���r�W�[��
        // ���v���
        {
            g_statis.bus_util_time += resv_info->release_time_us;

            if( resv_info->fmio[1] != NULL )// CE�܂Ƃߌ���
                g_statis.bus_util_time += resv_info->release_time_us;
        }

        if ( fm_info->SetBusStatus( resv_info->dma_no, BUS_BUSY) == false )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error at FM_Access::SetResourceBusy -- ���\�[�X�i�o�X�j���m�ۏo���܂���ł���\n" );
            return false;
        }
    }
    else if( resv_info->type = FM_CE_RSC )
    {// CE���r�W�[��
        // ���v���
        {
            g_statis.ce_util_time += resv_info->release_time_us;
            if( resv_info->fmio[1] != NULL )// CE�܂Ƃߌ���
                g_statis.ce_util_time += resv_info->release_time_us;
        }

        if ( fm_info->SetCEStatus( &resv_info->ce_addr, CE_BUSY) == false )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error at FM_Access::SetResourceBusy -- ���\�[�X�iCE�j���m�ۏo���܂���ł���\n" );
            return false;
        }
    }
    else
    {
        PrintMessage( LOG_TYPE_ERROR, "Error at FM_Access::SetResourceBusy -- ����`���\�[�X�ł�\n" );
        return false;
    }

    resv_info->release_time_us += cur_time;

    // -- �}��
    for( itr = rsv_list.begin(); itr != rsv_list.end(); itr++ )
    {
        if( (*itr).release_time_us > resv_info->release_time_us )
        {// ���b�N�I�����ԏ��Ƀ\�[�g
            break; // �t�_����ӏ�������Δ�����
        }
    }
    rsv_list.insert( itr, (*resv_info) );
    //--

    if( resv_info->type == FM_DMA_DOWN_RSC )
    {
        SetTraceHeader( "%lld,%lld,busy_dma_down,", g_ttl_statis.cur_sim_time, g_ttl_statis.host_io_count );
        PrintTrace( LOG_TYPE_TRACE, 2+(resv_info->dma_no *fm_info->GetCeNumPerBus()) ,
                    "%d\n",
                    resv_info->dma_no );
    }
    else if (  resv_info->type == FM_DMA_UP_RSC )
    {
        SetTraceHeader( "%lld,%lld,busy_dma_up,", g_ttl_statis.cur_sim_time, g_ttl_statis.host_io_count );
        PrintTrace( LOG_TYPE_TRACE, 2+(resv_info->dma_no *fm_info->GetCeNumPerBus()) ,
                    "%d\n",
                    resv_info->dma_no );
    }
    else
    {
        SetTraceHeader( "%lld,%lld,busy_ce,", g_ttl_statis.cur_sim_time, g_ttl_statis.host_io_count );
        PrintTrace( LOG_TYPE_TRACE, 2+(resv_info->ce_addr.dma_id *fm_info->GetCeNumPerBus()) + resv_info->ce_addr.ce_id,
            "(%d-%d)\n", resv_info->ce_addr.dma_id, resv_info->ce_addr.ce_id );

    }

    return true;
}

bool FM_Access::GetNextIdleResource( FM_RESV_INFO* resv_info )
{
    if( rsv_list.empty() )
    {
        resv_info->type = FM_NONE_RSC;
        return true;
    }

    *resv_info = rsv_list.front();
    rsv_list.pop_front();

    if( resv_info->type == FM_DMA_DOWN_RSC || resv_info->type == FM_DMA_UP_RSC )
    {// DMA��IDLE��
        if ( fm_info->SetBusStatus( resv_info->dma_no, BUS_IDLE ) == false )
            return false;
    }
    else
    {// CE��IDLE��
        if ( fm_info->SetCEStatus( &resv_info->ce_addr, CE_IDLE ) == false )
            return false;
    }

    if( resv_info->type == FM_DMA_DOWN_RSC )
    {
        SetTraceHeader( "%lld,%lld,idle_dma_down,", resv_info->release_time_us, g_ttl_statis.host_io_count );
        PrintTrace( LOG_TYPE_TRACE, 2+(resv_info->dma_no *fm_info->GetCeNumPerBus()) ,
            "%d\n", resv_info->dma_no );
    }
    else if ( resv_info->type == FM_DMA_UP_RSC )
    {
        SetTraceHeader( "%lld,%lld,idle_dma_up,", resv_info->release_time_us, g_ttl_statis.host_io_count );
        PrintTrace( LOG_TYPE_TRACE, 2+(resv_info->dma_no *fm_info->GetCeNumPerBus()) ,
            "%d\n", resv_info->dma_no );
    }
    else
    {
        SetTraceHeader( "%lld,%lld,idle_ce,", resv_info->release_time_us, g_ttl_statis.host_io_count );
        PrintTrace( LOG_TYPE_TRACE, 2+(resv_info->ce_addr.dma_id *fm_info->GetCeNumPerBus()) + resv_info->ce_addr.ce_id,
            "(%d-%d)\n", resv_info->ce_addr.dma_id, resv_info->ce_addr.ce_id );

    }

    return true;
}
