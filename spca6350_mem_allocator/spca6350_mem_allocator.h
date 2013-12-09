#ifndef _SPCA6350_MEM_ALLOCATOR_H_
#define _SPCA6350_MEM_ALLOCATOR_H_
#include <linux/ioctl.h>

#define SPCA6350_MEM_IOC_MAGIC	"G"

/*
 * S means "Set" through a ptr,                                        
 * T means "Tell" directly with the argument value                     
 * G means "Get": reply by setting through a pointer                   
 * Q means "Query": response is on the return value                    
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */
#define MEM_GET_INFO	_IOWR(SPCA6350_MEM_IOC_MAGIC,  1, unsigned long)                                                               
#define MEM_GET_BUF     _IOWR(SPCA6350_MEM_IOC_MAGIC,  2, unsigned long)
                                       
#define MEMALLOC_IOCHARDRESET       _IO(SPCA6350_MEM_IOC_MAGIC, 15) /* debugging tool */                                                              
#define SPCA6350_MEM_MAXNR 15                                          

typedef struct
{
	unsigned busAddress;                                               
	unsigned size;
}memInfo;                                     



#endif
