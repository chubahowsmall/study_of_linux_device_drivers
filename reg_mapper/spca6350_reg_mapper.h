#ifndef _SPCA6350_REG_MAPPER_H_
#define _SPCA6350_REG_MAPPER_H_
#include <linux/ioctl.h>

#define SPCA6350_REG_MAPPER_IOC_MAGIC	'M'

struct _regMapper{
	unsigned long addr;
	unsigned long val;
}regMapper;
/*
 * S means "Set" through a ptr,                                        
 * T means "Tell" directly with the argument value                     
 * G means "Get": reply by setting through a pointer                   
 * Q means "Query": response is on the return value                    
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */
#define REG_MAPPER_READ		_IOWR(SPCA6350_REG_MAPPER_IOC_MAGIC, \
 				1, \
				struct _regMapper)                                                               
#define REG_MAPPER_WRITE	_IOW(SPCA6350_REG_MAPPER_IOC_MAGIC, \
				2,\
				struct _regMapper)
                                       
#define MEMALLOC_IOCHARDRESET       _IO(SPCA6350_MEM_IOC_MAGIC, 15) /* debugging tool */                                                              
#define SPCA6350_MEM_MAXNR 15                                          

typedef struct
{
	unsigned busAddress;                                               
	unsigned size;
}memInfo;                                     



#endif
