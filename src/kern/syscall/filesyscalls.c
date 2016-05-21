#include <types.h>
#include <lib.h>
#include <array.h>
#include <current.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
//#include <syscall.h>
#include <filehandle.h>
#include <stat.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <mips/trapframe.h>
#include <uio.h>
#include <kern/iovec.h>
#include <kern/seek.h>
#include <limits.h>
#include <synch.h>
#include <filehandle.h>
//#include <syscall.h>
#include <kern/syscall.h>
//#include <mips/trapframe.h>
#include <syscall.h>

int sys_open(const char *filename, int flags, int32_t *retval)
{
	int result, fd;
	off_t offset;
	char path[PATH_MAX];
	struct filehandle *fh;
	struct vnode *vn;
	struct stat *st;
	size_t length;

	result = get_next_aval_fd(curthread->fd_tbl, &fd);
	if(result)
		return result;

	if((flags & O_ACCMODE) == 3)
		return EINVAL;
	
//	path = (char *) kmalloc(sizeof(char)*PATH_MAX);

//	kprintf("copystr again???");
	result = copyinstr((const_userptr_t) filename, path, PATH_MAX, &length);
	if(result)
		return result;
	
	result = vfs_open(path, flags, 0, &vn);
	if(result)
		return result;

	if(flags & O_APPEND)
	{
		st = (struct stat *)kmalloc(sizeof(struct stat));
		VOP_STAT(vn, st);
		offset = st->st_size;
		kfree(st);
	}
	else
		offset = 0;

	flags = flags & O_ACCMODE;

	fh = filehandle_create(vn, offset, flags);

	curthread->fd_tbl[fd] = fh;
	
	

	*retval = fd;
	
	return 0;
}

int sys_close(int fd)
{
	if(fd < 0 || fd >= FD_LIMIT || curthread->fd_tbl[fd] == NULL || curthread->fd_tbl[fd] ==(struct filehandle *)  0xdeadbeef)
	{
		return EBADF;
	}

//	curthread->fd_tbl[fd]->refcnt -= 1;
//	vfs_close(curthread->fd_tbl[fd]->vn);
	if(curthread->fd_tbl[fd]->refcnt == 1)
	{
		vfs_close(curthread->fd_tbl[fd]->vn);
		kfree(curthread->fd_tbl[fd]);
		curthread->fd_tbl[fd] = NULL;
	}
	else{
//		VOP_DECREF(curthread->fd_tbl[fd]->vn);
		curthread->fd_tbl[fd]->refcnt = curthread->fd_tbl[fd]->refcnt - 1;
	}	
	return 0;
}

int sys_write(int fd, const void *buf, size_t nbytes, int32_t *retval)
{
	int result;
	struct uio *uio_w;
	struct iovec *iov_w;

//	kprintf("begin write");
	
	void *buf_t;
//	buf_t = kmalloc(sizeof(*buf)*nbytes);
//	if(buf_t == NULL){
//		kfree(buf_t);
//		return EINVAL;
//	}	

	if(fd < 0 || fd >= FD_LIMIT || (curthread->fd_tbl[fd] == NULL) || curthread->fd_tbl[fd] == (struct filehandle *) 0xdeadbeef)
	{
//		kprintf("%d, %p\n", fd, curthread->fd_tbl[fd]);
//		kfree(buf_t);
		return EBADF;
	}

	if(curthread->fd_tbl[fd]->openflags == O_RDONLY) 
	{
//		kfree(buf_t);
		return EBADF;
	}		


//	lock_acquire(curthread->fd_tbl[fd]->lk);		

	buf_t = kmalloc(sizeof(*buf)*nbytes);
        if(buf_t == NULL){
                kfree(buf_t);
                return EINVAL;
        }

	
	iov_w = (struct iovec *)kmalloc(sizeof(struct iovec));
	result = copyin((const_userptr_t) buf, buf_t, nbytes);//iov_w->iov_ubase, nbytes);
	if(result)
	{
		kfree(iov_w);
		kfree(buf_t);
//		lock_release(curthread->fd_tbl[fd]->lk);
		return result;
	}
	iov_w->iov_len = nbytes;
	iov_w->iov_ubase = (userptr_t)buf;
//	kprintf("%c", *((char *) iov_w->iov_ubase));
	uio_w = (struct uio *)kmalloc(sizeof(struct uio));

	uio_w->uio_iov = iov_w;
	uio_w->uio_iovcnt = 1;
	uio_w->uio_segflg = UIO_USERSPACE;	
	uio_w->uio_rw = UIO_WRITE;
	uio_w->uio_offset = curthread->fd_tbl[fd]->offset;
	uio_w->uio_resid = nbytes;
	uio_w->uio_space = curthread->t_addrspace;
	

	result = VOP_WRITE(curthread->fd_tbl[fd]->vn, uio_w); 
	if(result)
	{
		kfree(iov_w);
		kfree(uio_w);
		kfree(buf_t);
//		lock_release(curthread->fd_tbl[fd]->lk);
		return result;
	}	
	*retval = nbytes - uio_w->uio_resid;

	curthread->fd_tbl[fd]->offset += *retval;
//	curthread->fd_tbl[fd]->offset = uio_w->uio_offset;
	
	kfree(iov_w);
        kfree(uio_w);
	kfree(buf_t);
//	lock_release(curthread->fd_tbl[fd]->lk);

//	kprintf("finished write\n");

	return 0;
}



int sys_read(int fd, void *buf, size_t buflen, int32_t *retval)
{
        int result;
        struct uio *uio_r;
        struct iovec *iov_r;



        if(fd < 0 || fd >= FD_LIMIT || curthread->fd_tbl[fd] == NULL || curthread->fd_tbl[fd] == (struct filehandle *)0xdeadbeef)
        {
                return EBADF;
        }

        if(curthread->fd_tbl[fd]->openflags == O_WRONLY)
        {
                return EBADF;
        }

//	lock_acquire(curthread->fd_tbl[fd]->lk);

	iov_r = (struct iovec *)kmalloc(sizeof(struct iovec));

//      result = copyin((const_userptr_t) buf, iov_r->iov_ubase, buflen);
//      if(result)
//              return result;
	
	iov_r->iov_ubase = (userptr_t)buf;
        iov_r->iov_len = buflen;

	uio_r = (struct uio *)kmalloc(sizeof(struct uio));

        uio_r->uio_iov = iov_r;
        uio_r->uio_iovcnt = 1;
        uio_r->uio_segflg = UIO_USERSPACE;
        uio_r->uio_rw = UIO_READ;
        uio_r->uio_offset = curthread->fd_tbl[fd]->offset;
        uio_r->uio_resid = buflen;
        uio_r->uio_space = curthread->t_addrspace;


        result = VOP_READ(curthread->fd_tbl[fd]->vn, uio_r);
        if(result){
  //       	lock_release(curthread->fd_tbl[fd]->lk);
		kfree(uio_r);
		kfree(iov_r);
       		return result;
	}
        *retval = buflen - uio_r->uio_resid;

	kfree(uio_r);
	kfree(iov_r);

//	curthread->fd_tbl[fd]->offset = uio_r->uio_offset;
	curthread->fd_tbl[fd]->offset += *retval;

//	lock_release(curthread->fd_tbl[fd]->lk);

        return 0;
}


int sys_chdir(const char *pathname)
{
	char *name;
//	char name[PATH_MAX];
	int result = 0;
	size_t length;
	name = (char *) kmalloc(sizeof(char)*PATH_MAX);

	result = copyinstr((const_userptr_t)  pathname, name, PATH_MAX, &length); // NULL);
	if(result)
	{
		kfree(name);
                return result;
        }

//	kprintf("reached vfs_chdir");

	result = vfs_chdir(name);
	if(result)
	{	
		kfree(name);
		return result;
	}

	kfree(name);

	return 0;

} 

int sys___getcwd(char *buf, size_t buflen, int32_t *retval){
	
	int result;
	size_t length;
	struct uio *cd_uio;
	struct iovec *cd_iov;

	cd_iov = (struct iovec *)kmalloc(sizeof(struct iovec));
	cd_uio = (struct uio *)kmalloc(sizeof(struct uio));

	cd_iov->iov_len = buflen;

	cd_uio->uio_iov = cd_iov;	
	cd_uio->uio_iovcnt = 1;
	cd_uio->uio_segflg = UIO_USERSPACE;
	cd_uio->uio_space = curthread->t_addrspace;
	cd_uio->uio_rw = UIO_READ;
	cd_uio->uio_resid = buflen;
	cd_uio->uio_offset = (off_t)0;

	result = vfs_getcwd(cd_uio);
	if(result){
		return result;
	}

	

	result = copyoutstr((const char *) cd_uio->uio_iov->iov_kbase, (userptr_t)buf, buflen, &length);
	if(result)
		return result;

	*retval = length;	

	kfree(cd_uio);
	kfree(cd_iov);

	return 0;
}

int sys_dup2(int oldfd, int newfd, int32_t *retval)
{
	if(oldfd < 0 || newfd < 0 || oldfd >= FD_LIMIT || newfd >= FD_LIMIT || (curthread->fd_tbl[oldfd] == NULL) || curthread->fd_tbl[oldfd] == (struct filehandle *) 0xdeadbeef)
	{
		return EBADF;
	}
	if(curthread->fd_tbl[newfd] != NULL)
	{
		sys_close(newfd); 
	}
	
	curthread->fd_tbl[newfd] = curthread->fd_tbl[oldfd];
	curthread->fd_tbl[newfd]->refcnt++;

	*retval = newfd;
	return 0;
} 

off_t sys_lseek(int fd, off_t pos, int whence, int32_t *retval, int32_t *ret){

	int result;
	off_t finpos;
	struct stat *st;

//	kprintf("begin lseel");

	if(fd < 0 || fd >= FD_LIMIT || curthread->fd_tbl[fd] == NULL|| curthread->fd_tbl[fd] == (struct filehandle *) 0xdeadbeef){
		return EBADF;
	}
	
	if(whence > 2 || whence < 0){ 	
		return EINVAL;
	}

//	lock_acquire(curthread->fd_tbl[fd]->lk);

	
	
	if(whence == SEEK_SET){
		finpos = pos;	
	}

	else if(whence == SEEK_CUR){
		finpos = pos + curthread->fd_tbl[fd]->offset;
	}

	else if(whence == SEEK_END){
		st = (struct stat *)kmalloc(sizeof(struct stat));
		result = VOP_STAT(curthread->fd_tbl[fd]->vn, st);
		if(result){
			kfree(st);
			return result;
		}
		finpos = pos + st->st_size;
		kfree(st);
	}
	else{
//		lock_release(curthread->fd_tbl[fd]->lk);
		return EINVAL;
	}
	
	

	if(finpos < 0){
//		lock_release(curthread->fd_tbl[fd]->lk);
		return EINVAL;
	}

//	kprintf("passed the error checking in lseek with the whence %d\n", whence);
	
	result = VOP_TRYSEEK(curthread->fd_tbl[fd]->vn, finpos);
	if(result){
//		lock_release(curthread->fd_tbl[fd]->lk);
		return result;
	}

	curthread->fd_tbl[fd]->offset = finpos;
	
	*retval = (uint32_t)((finpos & 0xFFFFFFFF00000000) >> 32);

	*ret = (uint32_t)(finpos & 0x00000000FFFFFFFF);	

//	kfree(st);

//	kprintf("got past VOP_SEEK with result %d and finpos %lld and ret %lld\n", result, finpos, *ret);
	
//	lock_release(curthread->fd_tbl[fd]->lk);
//	kprintf("finished lseek\n");
	return 0;
}
