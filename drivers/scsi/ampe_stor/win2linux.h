#if !defined(__WIN_TO_LINUX_H_)
/*************************************************************************************************/
#define __WIN_TO_LINUX_H_


#define IN
#define OUT


#define KSPIN_LOCK				spinlock_t
#define KeInitializeSpinLock	spin_lock_init

#define KDPC			struct tasklet_struct
#define KeInitializeDpc			tasklet_init

#define KTIMER					struct timer_list
#define KeCancelTimer			del_timer


#define SysDelay				mdelay
#define sys_delay				mdelay

#define DbgPrint	printk



#define PDMA_ADAPTER	u8
#define PKINTERRUPT		u8
#define KEVENT			u8
#define PIO_WORKITEM	u8



















/*************************************************************************************************/
#endif
