//------------------------------------------------------------------------------
//
// memtrace
//
// trace calls to the dynamic memory manager
//
#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memlog.h>
#include <memlist.h>

//
// function pointers to stdlib's memory management functions
//
static void *(*mallocp)(size_t size) = NULL;
static void (*freep)(void *ptr) = NULL;
static void *(*callocp)(size_t nmemb, size_t size);
static void *(*reallocp)(void *ptr, size_t size);

//
// statistics & other global variables
//
static unsigned long n_malloc  = 0;
static unsigned long n_calloc  = 0;
static unsigned long n_realloc = 0;
static unsigned long n_allocb  = 0;
static unsigned long n_freeb   = 0;
static item *list = NULL;

//
// init - this function is called once when the shared library is loaded
//
__attribute__((constructor))
void init(void)
{
  char *error;

  LOG_START();

  // initialize a new list to keep track of all memory (de-)allocations
  // (not needed for part 1)
  list = new_list();

  // ...
}

void *malloc(size_t size){

  void *ptr;
  char *dl_err;

  if(!mallocp){
    mallocp = dlsym(RTLD_NEXT, "malloc");
    if((dl_err = dlerror())!=NULL) {
      fputs(dl_err, stderr);
      exit(1);
    }
  }



  n_malloc = n_malloc + 1;
  n_allocb = n_allocb + size;

  ptr = mallocp(size);

  alloc(list, ptr, size);

  LOG_MALLOC(size, ptr);

  return ptr;
}

void free(void* ptr){
  
  char *dl_err;

  if(!freep){
    freep = dlsym(RTLD_NEXT, "free");
    if((dl_err = dlerror())!=NULL){
      fputs(dl_err, stderr);
      exit(1);
    }
  }

  item* deallocb = dealloc(list, ptr);

  n_freeb = n_freeb + deallocb->size;

  freep(ptr);

  LOG_FREE(ptr);

}

void *realloc(void *ptr, size_t size){
  char *dl_err;
  char *res;

  if(!reallocp){
    reallocp = dlsym(RTLD_NEXT, "realloc");
    if((dl_err = dlerror())!=NULL){
      fputs(dl_err, stderr);
      exit(1);
    }
  }

  res = reallocp(ptr, size);

  n_freeb = n_freeb + dealloc(list, ptr)->size;
  alloc(list, res, size);

  n_realloc = n_realloc + 1;
  n_allocb = n_allocb + size;

  LOG_REALLOC(ptr, size, res);

  return res;
}

void *calloc(size_t nmemb, size_t size){
  char *dl_err;
  char *ptr;

  if(!callocp){
    callocp = dlsym(RTLD_NEXT, "calloc");
    if((dl_err = dlerror())!=NULL){
      fputs(dl_err, stderr);
      exit(1);
    }
  }

  ptr = callocp(nmemb, size*nmemb);

  n_calloc = n_calloc+1;
  n_allocb = n_allocb + size*nmemb;

  alloc(list, ptr, size);

  LOG_CALLOC(nmemb, size, ptr);

  return ptr;
}





//
// fini - this function is called once when the shared library is unloaded
//
__attribute__((destructor))
void fini(void)
{
  // ...

  LOG_STATISTICS(n_allocb, n_allocb/(n_malloc+n_calloc+n_realloc), n_freeb);

  
  item *next_item = list->next;
  while(next_item!=NULL && next_item->cnt==0){
    next_item = next_item->next;
  }

  if(next_item!=NULL){
    LOG_NONFREED_START();
    LOG_BLOCK(next_item->ptr, next_item->size, next_item->cnt);
    while((next_item = next_item->next) != NULL){
      if(next_item->cnt!=0)
        LOG_BLOCK(next_item->ptr, next_item->size, next_item->cnt);
    }
  }

  LOG_STOP();

  // free list (not needed for part 1)
  free_list(list);
}

// ...
