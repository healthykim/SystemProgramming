/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*-- custom macro --*/
#define DSIZE 8
#define WSIZE 4
#define CHUNKSIZE (1<<12)
#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_FPTR(bp) ((char *)(bp)+WSIZE)
#define PREV_FPTR(bp) ((char *)(bp))
#define NEXT_FB(bp) (*(unsigned int **)NEXT_FPTR(bp))
#define PREV_FB(bp) (*(unsigned int **)PREV_FPTR(bp))
#define SET_FB(p, q) (*(unsigned int *)(p) = (unsigned int)(q))

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define MAX(a, b) (a>b? a:b)
#define MIN(a, b) (a<b? a:b)

#define GETLIST(a) (freeList[a])

/*------Global Variables---------*/
float mallocAvg = 0;
int mallocNum = 0;
void * heap_lo = NULL;
void * freeList_0; void * freeList_1; void * freeList_2; void * freeList_3; void * freeList_4; void * freeList_5;
void * freeList_6; void * freeList_7; void * freeList_8; void * freeList_9; void * freeList_10; void * freeList_11;
void * freeList_12; void * freeList_13; void * freeList_14; void * freeList_15; void * freeList_16; void * freeList_17;
void * freeList_18; void * freeList_19; void * freeList_20; void * freeList_21; void * freeList_22; void * freeList_23;
void * freeList_24; void * freeList_25; void * freeList_26; void * freeList_27; void * freeList_28; void * freeList_29;

/*assuming that the maximum heap size is 1GB. 
 *[2^0-2^1-1] [2^1-2^2-1] ... [2^29-2^30-1], exponent linearly.
 *I'm considering to convert this into non-linear list... indexing may be tricky...
 */

/*------Custum Functions----------*/
static void *extend_heap(size_t words);
static void *coalesce(void* bp);
static void *place(void* bp, size_t size);
static void *find_fit(size_t size);

static int get_listIdx(size_t size);
static void* get_list(int index);
static int add_list(void* bp);
static int delete_list(void* bp);

/*
 * extend_heap : extends the heap, maintaining alignment, with a new free block.
 */
static void *extend_heap(size_t words)
{
    void *bp;
    size_t size;
    
    /*allocate heap maintaing alignment*/
    size = ALIGN(words);
    bp = mem_sbrk(size);
    if(bp == (void *)-1)
        return NULL;


    /*Initialize free block header, footer and the epilogue header*/
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // epliogue header

    /*coalesce*/
    bp = coalesce(bp);
    add_list(bp);
    return bp;

}

/*
 * place : place the block of size with block pointer bp
 */

static void *place(void *ptr, size_t asize)
{
    size_t rel_size = GET_SIZE(HDRP(ptr));
    delete_list(ptr);

    /* if the margin is less than 2*DSIZE, there is no space for header and footer and pointers. do not split*/
    if(rel_size-asize <= 2*DSIZE){
        PUT(HDRP(ptr), PACK(rel_size, 1));
        PUT(FTRP(ptr), PACK(rel_size, 1));
    }
    else{
        size_t threshold;
        if(mallocNum < 2)
            threshold = __INT_MAX__;
        else
            threshold = (int)mallocAvg;

        if(asize < threshold){ // small -> front
            size_t margin = rel_size - asize;
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(margin, 0));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(margin, 0));
            add_list(NEXT_BLKP(ptr));
        }
        else{            
            size_t margin = rel_size - asize;
            PUT(HDRP(ptr), PACK(margin, 0));
            PUT(FTRP(ptr), PACK(margin, 0));
            add_list(ptr);
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(asize, 1));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(asize, 1));
            return NEXT_BLKP(ptr);
        }
    }
    return ptr;

}

/*
 * find_fit : search the fit freed block of size asize, at the freed block list.
 * FIRST FIT MANNER
 */

static void* find_fit(size_t size)
{
    int index = get_listIdx(size);
    void* list = get_list(index);
    void* next = NULL;
    while(index<=29){
        next = list = get_list(index);
        if(list!=NULL){
            while(next!=NULL && GET_SIZE(HDRP(next))<size){
                next = NEXT_FB(next);
            }
            if(next != NULL){
                break;
            }
        }
        index++;
    }
    return next;
}

/*
 * coalesce : coalesce the freed block
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){
        /*case 1*/
        return bp;
    }
    else if(prev_alloc && !next_alloc){
        /*case 2*/
        delete_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if(!prev_alloc && next_alloc){
        /*case 3*/
        delete_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else{
        /*case 4*/
        delete_list(PREV_BLKP(bp));
        delete_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}


/*
 * get_list : get the list with given index.
 */

static void* get_list(int index){
    if(index==0) {return freeList_0;}   if(index==1) {return freeList_1;}   if(index==2) {return freeList_2;}   if(index==3) {return freeList_3;}   if(index==4) {return freeList_4;}   if(index==5) {return freeList_5;}
    if(index==6) {return freeList_6;}   if(index==7) {return freeList_7;}   if(index==8) {return freeList_8;}   if(index==9) {return freeList_9;}   if(index==10) {return freeList_10;} if(index==11) {return freeList_11;}
    if(index==12) {return freeList_12;} if(index==13) {return freeList_13;} if(index==14) {return freeList_14;} if(index==15) {return freeList_15;} if(index==16) {return freeList_16;} if(index==17) {return freeList_17;}
    if(index==18) {return freeList_18;} if(index==19) {return freeList_19;} if(index==20) {return freeList_20;} if(index==21) {return freeList_21;} if(index==22) {return freeList_22;} if(index==23) {return freeList_23;}
    if(index==24) {return freeList_24;} if(index==25) {return freeList_25;} if(index==26) {return freeList_26;} if(index==27) {return freeList_27;} if(index==28) {return freeList_28;} if(index==29) {return freeList_29;}

    return NULL;
}


/*
 * get_listIdx : set the list with given index to bp.
 */

static void set_list(int index, void* bp){
    if(index==0)  {freeList_0=bp;}   if(index==1)  {freeList_1=bp;}  if(index==2) {freeList_2=bp;}   if(index==3) {freeList_3=bp;}   if(index==4) {freeList_4=bp;}   if(index==5) {freeList_5=bp;}
    if(index==6)  {freeList_6=bp;}   if(index==7)  {freeList_7=bp;}  if(index==8) {freeList_8=bp;}   if(index==9) {freeList_9=bp;}   if(index==10) {freeList_10=bp;} if(index==11) {freeList_11=bp;}
    if(index==12) {freeList_12=bp;} if(index==13) {freeList_13=bp;} if(index==14) {freeList_14=bp;} if(index==15) {freeList_15=bp;} if(index==16) {freeList_16=bp;} if(index==17) {freeList_17=bp;}
    if(index==18) {freeList_18=bp;} if(index==19) {freeList_19=bp;} if(index==20) {freeList_20=bp;} if(index==21) {freeList_21=bp;} if(index==22) {freeList_22=bp;} if(index==23) {freeList_23=bp;}
    if(index==24) {freeList_24=bp;} if(index==25) {freeList_25=bp;} if(index==26) {freeList_26=bp;} if(index==27) {freeList_27=bp;} if(index==28) {freeList_28=bp;} if(index==29) {freeList_29=bp;}

}

/*
 * get_listIdx : get the list index where given size of block can fit.
 */
static int get_listIdx(size_t size){
    int index = 0;
    while(size != 1){
        size = size>>1;
        index++;
    }
    return index;
}

/*
 * add_list : add the given block with bp to the list
 * if fails, return -1
 */
static int add_list(void* bp)
{
    int index = get_listIdx(GET_SIZE(HDRP(bp)));
    void* freeList = get_list(index);

    /* case 1 : first and only block of the list */
    if(freeList==NULL){
        set_list(index, bp);
        SET_FB(NEXT_FPTR(bp), NULL);
        SET_FB(PREV_FPTR(bp), NULL);
    }
    else{
        void * nextBp = freeList;
        while(nextBp && GET_SIZE(HDRP(nextBp))<GET_SIZE(HDRP(bp))){
            if(NEXT_FB(nextBp))
                nextBp = NEXT_FB(nextBp);
            else
                break;
        }
        if(NEXT_FB(nextBp) == NULL){
            /* case 3 : end block of the list*/
            SET_FB(NEXT_FPTR(nextBp), bp);
            SET_FB(PREV_FPTR(bp), nextBp);
            SET_FB(NEXT_FPTR(bp), NULL);
        }
        else if(nextBp == freeList){
            /* case 2 : first block of the list*/
            SET_FB(NEXT_FPTR(bp), nextBp);
            SET_FB(PREV_FPTR(bp), NULL);
            SET_FB(PREV_FPTR(nextBp), bp);
            set_list(index, bp);
        }
        else{
            /*case 4 : middle of the list, front of the nextBp*/
            SET_FB(PREV_FPTR(bp), PREV_FB(nextBp));
            SET_FB(NEXT_FPTR(PREV_FB(nextBp)), bp);
            SET_FB(NEXT_FPTR(bp), nextBp);
            SET_FB(PREV_FPTR(nextBp), bp);
        }
    }

    return 0;
}

/*
 * delete_list : delete the given block with bp from the list
 * if fails, return -1
 */
static int delete_list(void* bp)
{
    int index = get_listIdx(GET_SIZE(HDRP(bp)));
    void* freeList = get_list(index);
    if(freeList==NULL){
        return -1;
    }
    else{
        void * ptr = freeList;
        while(ptr!=bp && NEXT_FPTR(ptr)!=NULL){
            ptr = NEXT_FB(ptr);
        }
        if(NEXT_FPTR(ptr) == NULL && ptr!=bp){
            return -1;
        }
        else{
            if(NEXT_FB(bp)==NULL && PREV_FB(bp) == NULL){
                /*case 1 : only element of the list*/
                set_list(index, NULL);
            }
            else if(PREV_FB(bp)==NULL){
                /*case 2 : first element of the list*/
                set_list(index, NEXT_FB(bp));
                SET_FB(PREV_FPTR(NEXT_FB(bp)), NULL);
            }
            else if(NEXT_FB(bp)==NULL){
                /*case 3 : last element of the list*/
                SET_FB(NEXT_FPTR(PREV_FB(bp)), NULL);
            }
            else{
                /*case 4 : middle of the list, front of the nextBp*/
                SET_FB(PREV_FPTR(NEXT_FB(bp)), PREV_FB(bp));
                SET_FB(NEXT_FPTR(PREV_FB(bp)), NEXT_FB(bp));
            }
        }
        return 0;
    }
}




/*------Main Memory Functions-----*/
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* heap_lo : start point of heap */
    mem_init();
    heap_lo = mem_sbrk(4*WSIZE);
    mallocAvg = 0;
    mallocNum = 0;
    if(heap_lo == (char *)-1)
        return -1;
    PUT(heap_lo, 0); /*ALIGNMENT PADDING*/
    PUT(heap_lo + WSIZE, PACK(DSIZE, 1)); /*PROLOGUE HEADER*/
    PUT(heap_lo + (2*WSIZE), PACK(DSIZE, 1)); /*PROLOGUE FOOTER*/
    PUT(heap_lo + (3*WSIZE), PACK(0, 1)); /*EPILOGUE HEADER*/
    heap_lo += (2*WSIZE);
    
    freeList_0=NULL;  freeList_1=NULL;  freeList_2=NULL;  freeList_3=NULL;  freeList_4=NULL;  freeList_5=NULL;
    freeList_6=NULL;  freeList_7=NULL;  freeList_8=NULL;  freeList_9=NULL;  freeList_10=NULL; freeList_11=NULL;
    freeList_12=NULL; freeList_13=NULL; freeList_14=NULL; freeList_15=NULL; freeList_16=NULL; freeList_17=NULL;
    freeList_18=NULL; freeList_19=NULL; freeList_20=NULL; freeList_21=NULL; freeList_22=NULL; freeList_23=NULL;
    freeList_24=NULL; freeList_25=NULL; freeList_26=NULL; freeList_27=NULL; freeList_28=NULL; freeList_29=NULL;

    /*Extend the heap*/
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL){
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    size_t asize;
    size_t extendsize;
    void *bp = NULL;
    mallocAvg = (mallocAvg*mallocNum+size)/(mallocNum+1);
    mallocNum = mallocNum +1;
    
    if(mallocNum >= 3000){
        mallocAvg = 0;
        mallocNum = 0;
    }

    if(size == 0)
        return 0;
    /*Align the size*/
    if(size<=DSIZE){ 
        asize = 2 * DSIZE; //need space for header and footer and pointers
    }
    else
        asize = ALIGN((size + DSIZE)); // header and footer and so on can fit
    
    /*Search the free list for a fit freed block*/
    if((bp = find_fit(asize))!=NULL){
        bp = place(bp, asize);
        return bp;
    }
    /*No fit*/
    extendsize = MAX(asize, CHUNKSIZE);
    bp = extend_heap(extendsize);
    if(bp == NULL){
        return NULL;
    }
    bp = place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    bp = coalesce(bp);
    add_list(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    size_t oldSize;
    size_t asize;


    if((int)size<0){
        return NULL;
    }
    else if((int)size == 0){
        mm_free(ptr);
        return NULL;
    }
    else{
        oldSize = GET_SIZE(HDRP(ptr));
        if(size<=DSIZE){ 
            asize = 2 * DSIZE; //need space for header and footer and pointers
        }
        else
            asize = ALIGN(size+DSIZE); // header and footer and so on can fit      
        
        asize += DSIZE; // Not to allocate more space to reallocate small space

        if(oldSize >= asize){
            return ptr;
        }  
        else{
            if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr)))){
                size_t totalSize = oldSize + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
                if(totalSize >= asize){
                    /* Case 1 : NEXT BLOCK can accomodate excessive size
                        * TODO : combine NEXT BLOCK and extend the realloc block.
                        */
                    size_t margin = totalSize - asize;
                    if(margin<= 2*asize){
                        delete_list(NEXT_BLKP(ptr));
                        PUT(HDRP(ptr), PACK(totalSize, 1));
                        PUT(FTRP(ptr), PACK(totalSize, 1));
                    }
                    else{
                        delete_list(NEXT_BLKP(ptr));
                        PUT(HDRP(ptr), PACK(asize, 1));
                        PUT(FTRP(ptr), PACK(asize, 1));
                        PUT(HDRP(NEXT_BLKP(ptr)), PACK(margin, 0));
                        PUT(FTRP(NEXT_BLKP(ptr)), PACK(margin, 0));
                        add_list(NEXT_BLKP(ptr));
                    }
                    return ptr;
                }
                else if(!GET_ALLOC(PREV_BLKP(ptr)) && GET_SIZE(HDRP(PREV_BLKP(ptr))) + totalSize >= asize){
                    /* Case 2 : NEXT BLOCK + PREV BLOCK can accomodate excessive size
                     * TODO : combine NEXT BLOCK + PREV BLOCK and extend the realloc block.
                     */
                    totalSize += GET_SIZE(HDRP(PREV_BLKP(ptr)));
                    size_t margin = totalSize - asize;
                    delete_list(PREV_BLKP(ptr));
                    delete_list(NEXT_BLKP(ptr));
                    ptr = PREV_BLKP(ptr);
                    if(margin<= 2*asize){
                        PUT(HDRP(ptr), PACK(totalSize, 1));
                        PUT(FTRP(ptr), PACK(totalSize, 1));
                    }
                    else{
                        PUT(HDRP(ptr), PACK(asize, 1));
                        PUT(FTRP(ptr), PACK(asize, 1));
                        PUT(HDRP(NEXT_BLKP(ptr)), PACK(margin, 0));
                        PUT(FTRP(NEXT_BLKP(ptr)), PACK(margin, 0));
                        add_list(NEXT_BLKP(ptr));
                    }
                    memcpy(ptr, oldptr, oldSize);
                    return ptr;
                }
            }

            //else{
                newptr = mm_malloc(asize);
                copySize = GET_SIZE(HDRP(oldptr));
                if (asize < copySize)
                copySize = asize;
                memcpy(newptr, oldptr, copySize);
                mm_free(oldptr);
            //}

            return newptr;
        }
    }

}
