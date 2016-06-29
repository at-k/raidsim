#ifndef GENERIC_QUE_H__
#define GENERIC_QUE_H__

#include <stdlib.h>


struct __QUEUE_HEAD;

//template <typename t_type>
typedef struct __QUE {
    void*         data;  // キューに格納するデータ
    __QUEUE_HEAD* head;  // キューの頭
    __QUE*        fwd;   // 前のキュー
    __QUE*        bck;   // 次のキュー
} QUEUE;

typedef struct __QUEUE_HEAD {
    uint32_t   count;    // キューに繋がっているエントリ数
    QUEUE      begin;    // 先頭キュー
    QUEUE      end;      // 最後尾
} QUEUE_HEAD;

inline void init_que(QUEUE* que)
{
    que->head = NULL;
    que->data = NULL;
    que->fwd  = NULL;
    que->bck  = NULL;
}

inline void init_que_head(QUEUE_HEAD* head)
{
    head->count = 0;
    init_que(&head->begin);
    init_que(&head->end);
    head->begin.bck = &head->end;
    head->end.fwd   = &head->begin;
}

inline void* get_data(QUEUE* que) {
    return que->data;
}

inline QUEUE* begin_item(QUEUE_HEAD* head) {
    return head->begin.bck;
}
inline QUEUE* end_item(QUEUE_HEAD* head) {
    return head->end.fwd;
}

inline int is_que_end(QUEUE* que) {
    return (que->data == NULL);
}

inline QUEUE* que_back(QUEUE* que) {
    return (que->bck);
}

inline QUEUE* que_forward(QUEUE* que) {
    return (que->fwd);
}

inline void remove_item(QUEUE_HEAD* head, QUEUE* target) {

    QUEUE* tmp_fwd, *tmp_bck;
    target->head = NULL;

    tmp_fwd = target->fwd;
    tmp_bck = target->bck;
    tmp_fwd->bck = tmp_bck;
    tmp_bck->fwd = tmp_fwd;

    target->fwd = NULL;
    target->bck = NULL;

    head->count--;
    if( head->count == 0 ) {
        head->begin.bck = &head->end;
        head->end.fwd  = &head->begin;
    }
}

inline void insert_forward(QUEUE_HEAD* head, QUEUE* pos, QUEUE* target) {
    QUEUE* tmp_fwd;
#ifdef _DEBUG
    if(target->fwd != NULL || target->bck != NULL || target->head != NULL)
        printf("error\n");
#endif

    tmp_fwd     = pos->fwd;
    pos->fwd    = target;
    target->bck = pos;
    target->fwd = tmp_fwd;
    tmp_fwd->bck= target;

    target->head = head;
    head->count ++;
}

inline void insert_back(QUEUE_HEAD* head, QUEUE* pos, QUEUE* target) {
    QUEUE* tmp_back;
#ifdef _DEBUG
    if(target->fwd != NULL || target->bck != NULL || target->head != NULL)
        printf("error");
#endif

    tmp_back     = pos->bck;
    pos->bck     = target;
    target->fwd  = pos;
    target->bck  = tmp_back;
    tmp_back->fwd= target;

    target->head = head;
    head->count ++;
}

inline void insert_front(QUEUE_HEAD* head, QUEUE* target) {
    insert_back(head, &head->begin, target);
}

inline void insert_end(QUEUE_HEAD* head, QUEUE* target) {
    insert_forward(head, &head->end, target);
}

inline QUEUE* pop_front(QUEUE_HEAD* head)
{
    if( head->count == 0 )
        return NULL;

    QUEUE* target = head->begin.bck;

    QUEUE* tmp_fwd, *tmp_bck;
#ifdef _DEBUG
    if( target->head != head)
        printf("error\n");
#endif
    target->head = NULL;

    tmp_fwd = target->fwd;
    tmp_bck = target->bck;
    tmp_fwd->bck = tmp_bck;
    tmp_bck->fwd = tmp_fwd;

    target->fwd = NULL;
    target->bck = NULL;

    head->count--;
    if( head->count == 0 ) {
        head->begin.bck = &head->end;
        head->end.fwd  = &head->begin;
    }

    return target;
}

/*inline void sort_insert(PB_Que_HEAD* head, PB_Que* target) {
PB_Que* pos;
pos = head->Sort(target);
insert_back(head, pos, target);
}*/

/*inline void print_queue(PB_Que_HEAD* head)
{
    PB_Que* que = head->begin.bck;
    while( !is_que_end(que) )
    {
        PB_Info* pb = GETPBINFO(que->item);
        PrintLCLogMessage("%d,%d,%d,%d\n", pb->pb_id, pb->ec, pb->next_ppo, pb->vp_count);
        que = que->bck;
    }
    PrintLCLogMessage("\n");
}*/

// targetのキューのボックス残量が変更された時にコール
// ボックス量について昇順でソートする
/*inline void update_que(LC_Que_HEAD* head, LC_Que* target) {
    LC_Que*  cur;
    LC_Info* tgt_info = GETLCINFO(target->item);
    if( target->fwd->item != NULL && tgt_info->free_box.count < GETLCINFO(target->fwd->item)->free_box.count )
    {
        cur = target->fwd;
        remove_item(head, target);
        while( cur->item != NULL ) {
            if( tgt_info->free_box.count > GETLCINFO(cur->item)->free_box.count ) {
                insert_back(head, cur, target);
                return;
            }
            cur = cur->fwd;
        }
        insert_front(head, target);

    }
    else if( target->bck->item != NULL && tgt_info->free_box.count > GETLCINFO(target->bck->item)->free_box.count )
    {
        cur = target->bck;
        remove_item(head, target);
        while( cur->item != NULL ) {
            if( tgt_info->free_box.count < GETLCINFO(cur->item)->free_box.count ) {
                insert_forward(head, cur, target);
                return;
            }
            cur = cur->bck;
        }
        insert_end(head, target);
    }
    // [==] の時は別に動かさない。
}*/

#endif
