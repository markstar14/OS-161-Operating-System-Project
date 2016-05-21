#ifndef _FILEHANDLE_H_
#define _FILEHANDLE_H_

#include <vnode.h>
#include <types.h>

//#define FD_LIMIT 128
#define FD_LIMIT 12
struct filehandle 
{
	struct vnode *vn;
	off_t offset;
	int openflags;
	int refcnt;
	struct lock *lk;		
};

struct filehandle * filehandle_create(struct vnode *vn, off_t offset, int flags);

/*Helper function to search through the file descriptor table for the next
 * available fd
 * Implemented in thread.c
 */

int get_next_aval_fd(struct filehandle **farr, int *fd); 

#endif /*_FILEHANDLE_H_*/
