/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Redefine the classical malloc/free/realloc functions so that they fit well in the mmalloc framework */

#include "mmprivate.h"
#include "gras_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_mm_legacy, xbt,
                                "Logging specific to mm_legacy in mmalloc");

static void *__mmalloc_current_heap = NULL;     /* The heap we are currently using. */

#include "xbt_modinter.h"

void *mmalloc_get_current_heap(void)
{
  return __mmalloc_current_heap;
}

void mmalloc_set_current_heap(void *new_heap)
{
  __mmalloc_current_heap = new_heap;
}

#ifdef MMALLOC_WANT_OVERIDE_LEGACY
void *malloc(size_t n)
{
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_MMAP
  if (!mdp)
    mmalloc_preinit();
#endif
  LOCK(mdp);
  void *ret = mmalloc(mdp, n);
  UNLOCK(mdp);

  return ret;
}

void *calloc(size_t nmemb, size_t size)
{
  size_t total_size = nmemb * size;
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_MMAP
  if (!mdp)
    mmalloc_preinit();
#endif
  LOCK(mdp);
  void *ret = mmalloc(mdp, total_size);
  UNLOCK(mdp);

  /* Fill the allocated memory with zeroes to mimic calloc behaviour */
  memset(ret, '\0', total_size);

  return ret;
}

void *realloc(void *p, size_t s)
{
  void *ret = NULL;
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_MMAP
  if (!mdp)
    mmalloc_preinit();
#endif
  LOCK(mdp);
  if (s) {
    if (p)
      ret = mrealloc(mdp, p, s);
    else
      ret = mmalloc(mdp, s);
  } else {
    if (p)
      mfree(mdp, p);
  }
  UNLOCK(mdp);

  return ret;
}

void free(void *p)
{
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_GTNETS
  if(!mdp) return;
#endif
  LOCK(mdp);
  mfree(mdp, p);
  UNLOCK(mdp);
}
#endif

/* Make sure it works with md==NULL */

/* Safety gap from the heap's break address.
 * Try to increase this first if you experience strange errors under
 * valgrind. */
#define HEAP_OFFSET   (128UL<<20)

void *mmalloc_get_default_md(void)
{
  xbt_assert(__mmalloc_default_mdp);
  return __mmalloc_default_mdp;
}

static void mmalloc_fork_prepare(void)
{
  struct mdesc* mdp = NULL;
  if ((mdp =__mmalloc_default_mdp)){
    while(mdp){
      LOCK(mdp);
      if(mdp->fd >= 0){
        mdp->refcount++;
      }
      mdp = mdp->next_mdesc;
    }
  }
}

static void mmalloc_fork_parent(void)
{
  struct mdesc* mdp = NULL;
  if ((mdp =__mmalloc_default_mdp)){
    while(mdp){
      if(mdp->fd < 0)
        UNLOCK(mdp);
      mdp = mdp->next_mdesc;
    }
  }
}

static void mmalloc_fork_child(void)
{
  struct mdesc* mdp = NULL;
  if ((mdp =__mmalloc_default_mdp)){
    while(mdp){
      UNLOCK(mdp);
      mdp = mdp->next_mdesc;
    }
  }
}

/* Initialize the default malloc descriptor. */
void mmalloc_preinit(void)
{
  int res;
  if (!__mmalloc_default_mdp) {
    unsigned long mask = ~((unsigned long)getpagesize() - 1);
    void *addr = (void*)(((unsigned long)sbrk(0) + HEAP_OFFSET) & mask);
    __mmalloc_default_mdp = mmalloc_attach(-1, addr);
    /* Fixme? only the default mdp in protected against forks */
    res = xbt_os_thread_atfork(mmalloc_fork_prepare,
			       mmalloc_fork_parent, mmalloc_fork_child);
    if (res != 0)
      THROWF(system_error,0,"xbt_os_thread_atfork() failed: return value %d",res);
  }
  xbt_assert(__mmalloc_default_mdp != NULL);
}

void mmalloc_postexit(void)
{
  /* Do not detach the default mdp or ldl won't be able to free the memory it allocated since we're in memory */
  //  mmalloc_detach(__mmalloc_default_mdp);
  mmalloc_pre_detach(__mmalloc_default_mdp);
}

int mmalloc_compare_heap(void *h1, void *h2){

  if(h1 == NULL && h2 == NULL){
    XBT_DEBUG("Malloc descriptors null");
    return 0;
  }
 

  /* Heapstats */

  struct mstats ms1 = mmstats(h1);
  struct mstats ms2 = mmstats(h2);

  if(ms1.bytes_total !=  ms2.bytes_total){
    XBT_DEBUG("Different total size of the heap");
    return 1;
  }

  if(ms1.chunks_used !=  ms2.chunks_used){
    XBT_DEBUG("Different chunks allocated by the user");
    return 1;
  }

  if(ms1.bytes_used !=  ms2.bytes_used){
    XBT_DEBUG("Different byte total of user-allocated chunks");
    return 1;
  }

  if(ms1.bytes_free !=  ms2.bytes_free){
    XBT_DEBUG("Different byte total of chunks in the free list");
    return 1;
  }

  if(ms1.chunks_free !=  ms2.chunks_free){
    XBT_DEBUG("Different chunks in the free list");
    return 1;
  }

  struct mdesc *mdp1, *mdp2;
  mdp1 = MD_TO_MDP(h1);
  mdp2 = MD_TO_MDP(h2);
  
  if(mmalloc_compare_mdesc(mdp1, mdp2))
    return 1;
  

  return 0;
}

int mmalloc_compare_mdesc(struct mdesc *mdp1, struct mdesc *mdp2){

   if(mdp1->headersize != mdp2->headersize){
    XBT_DEBUG("Different size of the file header for the mapped files");
    return 1;
  }

  if(mdp1->refcount != mdp2->refcount){
    XBT_DEBUG("Different number of processes that attached the heap");
    return 1;
  }
 
  if(strcmp(mdp1->magic, mdp2->magic) != 0){
    XBT_DEBUG("Different magic number");
    return 1;
  }

  if(mdp1->flags != mdp2->flags){
    XBT_DEBUG("Different flags");
    return 1;
  }

  if(mdp1->heapsize != mdp2->heapsize){
    XBT_DEBUG("Different number of info entries");
    return 1;
  }

  //XBT_DEBUG("Heap size : %d", mdp1->heapsize);

  if(mdp1->heapbase != mdp2->heapbase){
    XBT_DEBUG("Different first block of the heap");
    return 1;
  }

  if(mdp1->heapindex != mdp2->heapindex){
    XBT_DEBUG("Different index for the heap table");
    return 1;
  }

  //XBT_DEBUG("Heap index : %d", mdp1->heapindex);

  if(mdp1->base != mdp2->base){
    XBT_DEBUG("Different base address of the memory region");
    return 1;
  }

  if(mdp1->breakval != mdp2->breakval){
    XBT_DEBUG("Different current location in the memory region");
    return 1;
  }

  if(mdp1->top != mdp2->top){
    XBT_DEBUG("Different end of the current location in the memory region");
    return 1;
  }
  
  if(mdp1->heaplimit != mdp2->heaplimit){
    XBT_DEBUG("Different limit of valid info table indices");
    return 1;
  }

  //XBT_DEBUG("Heap limit : %d", mdp1->heaplimit);


  if(mdp1->fd != mdp2->fd){
    XBT_DEBUG("Different file descriptor for the file to which this malloc heap is mapped");
    return 1;
  }

  if(mdp1->saved_errno != mdp2->saved_errno){
    XBT_DEBUG("Different errno");
    return 1;
  }

  if(mdp1->version != mdp2->version){
    XBT_DEBUG("Different version of the mmalloc package");
    return 1;
  }

 
  size_t block_free1, start1, block_free2 , start2, block_busy1, block_busy2 ;
  unsigned int i;
  void *addr_block1, *addr_block2;
  struct mdesc* mdp;

  start1 = block_free1 = mdp1->heapindex; 
  start2 = block_free2 = mdp2->heapindex;
  block_busy1 = start1 + mdp1->heapinfo[start1].free.size;
  block_busy2 = start2 + mdp2->heapinfo[start2].free.size;

  XBT_DEBUG("Block busy : %d - %d", block_busy1, block_busy2);


  if(mdp1->heapinfo[start1].free.size != mdp2->heapinfo[start2].free.size){ // <=> check block_busy
    
    XBT_DEBUG("Different size (in blocks) of a free cluster");
    return 1;

  }else{

    if(mdp1->heapinfo[start1].free.next != mdp2->heapinfo[start1].free.next){

      XBT_DEBUG("Different index of next free cluster");
      return 1;

    }else{
   
      i=block_busy1 ;

      XBT_DEBUG("Next free : %d", mdp1->heapinfo[start1].free.next);


      while(i<mdp1->heapinfo[start1].free.next){

	XBT_DEBUG("i (block busy) : %d", i);

	if(mdp1->heapinfo[i].busy.type != mdp2->heapinfo[i].busy.type){
	  XBT_DEBUG("Different type of busy block");
	  return 1;
	}else{
	  mdp = mdp1;
	  addr_block1 = ADDRESS(i);
	  mdp = mdp2;
	  addr_block2 = ADDRESS(i); 
	  switch(mdp1->heapinfo[i].busy.type){
	  case 0 :
	    if(mdp1->heapinfo[i].busy.info.size != mdp2->heapinfo[i].busy.info.size){
	      XBT_DEBUG("Different size of a large cluster");
	      return 1;
	    }else{
	      XBT_DEBUG("Blocks %d : %p - %p / Data size : %d (%d blocks)", i, addr_block1, addr_block2, (mdp->heapinfo[i].busy.info.size * BLOCKSIZE),mdp->heapinfo[i].busy.info.size );	
	      if(memcmp(addr_block1, addr_block2, mdp1->heapinfo[i].busy.info.size * BLOCKSIZE) != 0){
		XBT_DEBUG("Different data in block %d", i);
		return 1;
	      } 
	    }
	    i = i + mdp1->heapinfo[i].busy.info.size;
	    break;
	  default :	  
	    if(mdp1->heapinfo[i].busy.info.frag.nfree != mdp2->heapinfo[i].busy.info.frag.nfree){
	      XBT_DEBUG("Different free fragments in a fragmented block");
	      return 1;
	    }else{
	      if(mdp1->heapinfo[i].busy.info.frag.first != mdp2->heapinfo[i].busy.info.frag.first){
		XBT_DEBUG("Different first free fragments of the block");
		return 1; 
	      }else{
		if(memcmp(addr_block1, addr_block2, BLOCKSIZE) != 0){
		  XBT_DEBUG("Different data in block %d", i);
		  return 1;
		} 
	      }
	    }
	    i++;
	    break;
	  }
	} 
      }


      block_free1 = mdp1->heapinfo[start1].free.next;
      block_free2 = mdp2->heapinfo[start2].free.next;

      //XBT_DEBUG("Index of next free cluster : %d", block_free1);

      while((block_free1 != start1) && (block_free2 != start2)){ 

	block_busy1 = block_free1 + mdp1->heapinfo[block_free1].free.size;
	block_busy2 = block_free2 + mdp2->heapinfo[block_free2].free.size;

	if(block_busy1 != block_busy2){
	  XBT_DEBUG("Different index of busy block");
	  return 1;
	}else{

	  XBT_DEBUG("Index of next busy block : %d - %d", block_busy1, block_busy2);
	  XBT_DEBUG("Index of next free cluster : %d", mdp1->heapinfo[block_free1].free.next);
	
	  i = block_busy1;

	  while(i<mdp1->heapinfo[block_free1].free.next){

	    XBT_DEBUG("i (block busy) : %d", i);

	    if(mdp1->heapinfo[i].busy.type != mdp2->heapinfo[i].busy.type){
	      XBT_DEBUG("Different type of busy block");
	      return 1;
	    }else{
	      mdp = mdp1;
	      addr_block1 = ADDRESS(i);
	      mdp = mdp2;
	      addr_block2 = ADDRESS(i); 
	      switch(mdp1->heapinfo[i].busy.type){
	      case 0 :
		if(mdp1->heapinfo[i].busy.info.size != mdp2->heapinfo[i].busy.info.size){
		  XBT_DEBUG("Different size of a large cluster");
		  return 1;
		}else{
		  XBT_DEBUG("Blocks %d : %p - %p / Data size : %d", i, addr_block1, addr_block2, (mdp->heapinfo[i].busy.info.size * BLOCKSIZE));		
		  //XBT_DEBUG("Size of large cluster %d", mdp->heapinfo[i].busy.info.size);
		  if(memcmp(addr_block1, addr_block2, (mdp1->heapinfo[i].busy.info.size * BLOCKSIZE)) != 0){
		    XBT_DEBUG("Different data in block %d", i);
		    return 1;
		  } 
		}
		i = i+mdp1->heapinfo[i].busy.info.size;
		break;
	      default :	  
		if(mdp1->heapinfo[i].busy.info.frag.nfree != mdp2->heapinfo[i].busy.info.frag.nfree){
		  XBT_DEBUG("Different free fragments in a fragmented block");
		  return 1;
		}else{
		  if(mdp1->heapinfo[i].busy.info.frag.first != mdp2->heapinfo[i].busy.info.frag.first){
		    XBT_DEBUG("Different first free fragments of the block");
		    return 1; 
		  }else{
		    if(memcmp(addr_block1, addr_block2, BLOCKSIZE) != 0){
		      XBT_DEBUG("Different data in fragmented block %d", i);
		      return 1;
		    } 
		  }
		}
		i++;
		break;
	      }
	    } 
	  }
	}

	block_free1 = mdp1->heapinfo[block_free1].free.next;
	block_free2 = mdp2->heapinfo[block_free2].free.next;
      
      } 
    
    }
  
      }
  return 0;   
  
  
  
}

 
  
  

