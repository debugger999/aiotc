#ifndef _NCX_LOCK_H_
#define _NCX_LOCK_H_

#define ncx_shmtx_lock  sem_wait
#define ncx_shmtx_unlock sem_post

typedef struct {

	ncx_uint_t spin;
	
} ncx_shmtx_t;

#endif
