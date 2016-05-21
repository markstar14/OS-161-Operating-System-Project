#include <types.h>
#include <lib.h>
#include <thread.h>
#include <filehandle.h>
#include <kern/errno.h>
#include <synch.h>

struct filehandle * filehandle_create(struct vnode *vn, off_t offset, int flags){
	struct filehandle *fh;

	fh = kmalloc(sizeof(*fh));
	if(fh == NULL)
		return NULL;
	
	fh->vn = vn;
	fh->offset = offset;
	fh->openflags = flags;
	fh->refcnt = 1;
//	fh->lk=kmalloc(sizeof(struct lock *));
//	fh->lk = lock_create("fd_lk");	

	return fh;
}

int get_next_aval_fd(struct filehandle **farr, int *fd)
{
        int count;

        for(count = 0; count < FD_LIMIT; count++)
        {
                if(farr[count] == NULL || farr[count] == (struct filehandle *) 0xdeadbeef)
                {
                        *fd = count;
                        return 0;
                }
        }
        //If we get this far then there are no fds available
        //return an error
        return EMFILE;
}
   
