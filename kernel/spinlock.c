/*
 * Copyright (2004) Linus Torvalds
 *
 * Author: Zwane Mwaikambo <zwane@fsmlabs.com>
 *
 * Copyright (2004) Ingo Molnar
 */

#include <linux/config.h>
#include <linux/linkage.h>
#include <linux/preempt.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/module.h>

/*
 * Generic declaration of the raw read_trylock() function,
 * architectures are supposed to optimize this:
 */
int __lockfunc generic_raw_read_trylock(rwlock_t *lock)
{
	_raw_read_lock(lock);
	return 1;
}
EXPORT_SYMBOL(generic_raw_read_trylock);

int __lockfunc _spin_trylock(spinlock_t *lock)
{
	preempt_disable();
	if (_raw_spin_trylock(lock))
		return 1;
	
	preempt_enable();
	return 0;
}
EXPORT_SYMBOL(_spin_trylock);

int __lockfunc _read_trylock(rwlock_t *lock)
{
	preempt_disable();
	if (_raw_read_trylock(lock))
		return 1;

	preempt_enable();
	return 0;
}
EXPORT_SYMBOL(_read_trylock);

int __lockfunc _write_trylock(rwlock_t *lock)
{
	preempt_disable();
	if (_raw_write_trylock(lock))
		return 1;

	preempt_enable();
	return 0;
}
EXPORT_SYMBOL(_write_trylock);

#ifndef CONFIG_PREEMPT

/**
 * ��û�������ں���ռʱ��read_lock��ʵ�֡�
 */
void __lockfunc _read_lock(rwlock_t *lock)
{
	preempt_disable();
	_raw_read_lock(lock);
}
EXPORT_SYMBOL(_read_lock);

unsigned long __lockfunc _spin_lock_irqsave(spinlock_t *lock)
{
	unsigned long flags;

	local_irq_save(flags); //���жϣ��������жϱ�־�� pushfq,pop flags
	preempt_disable();		//����ռ��current_thread_info()->preempt_count+1
	_raw_spin_lock_flags(lock, flags); //æ��
	return flags;
}
EXPORT_SYMBOL(_spin_lock_irqsave);

void __lockfunc _spin_lock_irq(spinlock_t *lock)
{
	local_irq_disable();
	preempt_disable();
	_raw_spin_lock(lock);
}
EXPORT_SYMBOL(_spin_lock_irq);

void __lockfunc _spin_lock_bh(spinlock_t *lock)
{
	local_bh_disable();
	preempt_disable();
	_raw_spin_lock(lock);
}
EXPORT_SYMBOL(_spin_lock_bh);

unsigned long __lockfunc _read_lock_irqsave(rwlock_t *lock)
{
	unsigned long flags;

	local_irq_save(flags);
	preempt_disable();
	_raw_read_lock(lock);
	return flags;
}
EXPORT_SYMBOL(_read_lock_irqsave);

void __lockfunc _read_lock_irq(rwlock_t *lock)
{
	local_irq_disable();
	preempt_disable();
	_raw_read_lock(lock);
}
EXPORT_SYMBOL(_read_lock_irq);

void __lockfunc _read_lock_bh(rwlock_t *lock)
{
	local_bh_disable();
	preempt_disable();
	_raw_read_lock(lock);
}
EXPORT_SYMBOL(_read_lock_bh);

unsigned long __lockfunc _write_lock_irqsave(rwlock_t *lock)
{
	unsigned long flags;

	local_irq_save(flags);
	preempt_disable();
	_raw_write_lock(lock);
	return flags;
}
EXPORT_SYMBOL(_write_lock_irqsave);

void __lockfunc _write_lock_irq(rwlock_t *lock)
{
	local_irq_disable();
	preempt_disable();
	_raw_write_lock(lock);
}
EXPORT_SYMBOL(_write_lock_irq);

void __lockfunc _write_lock_bh(rwlock_t *lock)
{
	local_bh_disable();
	preempt_disable();
	_raw_write_lock(lock);
}
EXPORT_SYMBOL(_write_lock_bh);

void __lockfunc _spin_lock(spinlock_t *lock)
{
	preempt_disable();
	_raw_spin_lock(lock);
}

EXPORT_SYMBOL(_spin_lock);

void __lockfunc _write_lock(rwlock_t *lock)
{
	preempt_disable();
	_raw_write_lock(lock);
}

EXPORT_SYMBOL(_write_lock);

#else /* CONFIG_PREEMPT: */

/*
 * This could be a long-held lock. We both prepare to spin for a long
 * time (making _this_ CPU preemptable if possible), and we also signal
 * towards that other CPU that it should break the lock ASAP.
 *
 * (We do this in a function because inlining it would be excessive.)
 */
/**
 * ͨ��BUILD_LOCK_OPS(spin, spinlock);������_spin_lock������ʵ����spin_lock
 * �����ھ����ں���ռʱ��spin_lock��ʵ�֡�
 */
#define BUILD_LOCK_OPS(op, locktype)					\
void __lockfunc _##op##_lock(locktype##_t *lock)			\
{									\
	/**
	 * preempt_disable�����ں���ռ��
	 * �����ڲ���spinlock��ֵǰ���Ƚ�ֹ��ռ��ԭ��ܼ򵥣��ڲ���ֵʱ���������ռ����ʲô�����
	 */
	preempt_disable();						\
	for (;;) {							\
		/**
	     * ����_raw_spin_trylock,������������slock�ֶν���ԭ���ԵĲ��Ժ����á�
		 * ��������ִ�����´��룺
		 *     movb $0,%al
		 *     xchgb %al, slp->slock
		 * xchgbԭ���ԵĽ���al��slp->slock�ڴ浥Ԫ�����ݡ����ԭֵ>0���ͷ���1�����򷵻�0
		 * ���仰˵�����ԭ�������ǿ��ŵģ��͹ص����������سɹ���־�����ԭ���������ŵģ��ٴ���������־��������0��
		 */
		if (likely(_raw_##op##_trylock(lock)))			\
			/**
		     * �����ֵ�����ģ���ʾ���Ǵ򿪵ģ���������Ѿ�����������ˡ�
			 * ע�⣺���غ󣬱�������һ�������þ��ǽ�����ռ�ˡ����ʹ��unlockʱ�ٴ���ռ��
			 * ����һ�½�����ռ�ı�Ҫ�ԡ�
			 */
			break;						\
		/**
		 * �����޷��������������ѭ��һֱ������CPU�ͷ���������
		 * ��ѭ��ǰ����ʱ��preempt_enable��Ҳ����˵���ڵȴ����������м䣬�����ǿ��ܱ���ռ�ġ�
		 */
		preempt_enable();					\
		/**
		 * break_lock��ʾ�����������ڵȴ�����
		 * ӵ�����Ľ��̿����ж������־����ǰ�ͷ�����
		 * ���ǣ��ĸ����̻��ж������־�أ���
		 * ����һ�������ǣ����ж���ʲô�أ������ֱ������break_lockΪ1��Ч�ʻ���΢��һ�㡣
		 */
		if (!(lock)->break_lock)				\
			(lock)->break_lock = 1;				\
		/**
		 * ִ�еȴ�ѭ����cpu_relax�򻯳�һ��pauseָ�
		 * ΪʲôҪ����cpu_relax������ԭ��ģ������Ͽ���������һ����ѭ���Ļ�����������ѭ��
		 * ����ʵ�����ǲ��������ĵģ���������ס���ߣ�unlock������ֵ�������ˡ�
		 * cpu_relax����Ҫ��CPU��Ϣһ�£���������ʱ�ó�����
		 */
		while (!op##_can_lock(lock) && (lock)->break_lock)	\
			cpu_relax();					\
		/**
		 * �������ѭ��lock��ֵ�Ѿ��仯�ˡ���ô����ռ���ٴε���_raw_spin_trylock
		 * �����Ļ����������_raw_spin_trylock�С�
		 */
		preempt_disable();					\
	}								\
}									\
									\
EXPORT_SYMBOL(_##op##_lock);						\
									\
unsigned long __lockfunc _##op##_lock_irqsave(locktype##_t *lock)	\
{									\
	unsigned long flags;						\
									\
	preempt_disable();						\
	for (;;) {							\
		local_irq_save(flags);					\
		if (likely(_raw_##op##_trylock(lock)))			\
			break;						\
		local_irq_restore(flags);				\
									\
		preempt_enable();					\
		if (!(lock)->break_lock)				\
			(lock)->break_lock = 1;				\
		while (!op##_can_lock(lock) && (lock)->break_lock)	\
			cpu_relax();					\
		preempt_disable();					\
	}								\
	return flags;							\
}									\
									\
EXPORT_SYMBOL(_##op##_lock_irqsave);					\
									\
void __lockfunc _##op##_lock_irq(locktype##_t *lock)			\
{									\
	_##op##_lock_irqsave(lock);					\
}									\
									\
EXPORT_SYMBOL(_##op##_lock_irq);					\
									\
void __lockfunc _##op##_lock_bh(locktype##_t *lock)			\
{									\
	unsigned long flags;						\
									\
	/*							*/	\
	/* Careful: we must exclude softirqs too, hence the	*/	\
	/* irq-disabling. We use the generic preemption-aware	*/	\
	/* function:						*/	\
	/**/								\
	flags = _##op##_lock_irqsave(lock);				\
	local_bh_disable();						\
	local_irq_restore(flags);					\
}									\
									\
EXPORT_SYMBOL(_##op##_lock_bh)

/*
 * Build preemption-friendly versions of the following
 * lock-spinning functions:
 *
 *         _[spin|read|write]_lock()
 *         _[spin|read|write]_lock_irq()
 *         _[spin|read|write]_lock_irqsave()
 *         _[spin|read|write]_lock_bh()
 */
BUILD_LOCK_OPS(spin, spinlock);
BUILD_LOCK_OPS(read, rwlock);
BUILD_LOCK_OPS(write, rwlock);

#endif /* CONFIG_PREEMPT */

void __lockfunc _spin_unlock(spinlock_t *lock)
{
	_raw_spin_unlock(lock);
	preempt_enable();
}
EXPORT_SYMBOL(_spin_unlock);
/**
 * �ͷ�д����
 */
void __lockfunc _write_unlock(rwlock_t *lock)
{
	/**
	 * ���û��lock ; addl $0x01000000, rwlp���ֶ��е�δ����־��λ��
	 */
	_raw_write_unlock(lock);
	/**
	 * ��Ȼ�ˣ��ڻ����ʱ�ǽ�����ռ�ģ���ʱҪ����ռ�򿪡�
	 * ���⣬ע������˳������lockʱ�෴��
	 */
	preempt_enable();
}
EXPORT_SYMBOL(_write_unlock);

void __lockfunc _read_unlock(rwlock_t *lock)
{
	_raw_read_unlock(lock);
	preempt_enable();
}
EXPORT_SYMBOL(_read_unlock);

void __lockfunc _spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
	_raw_spin_unlock(lock);
	local_irq_restore(flags);
	preempt_enable();
}
EXPORT_SYMBOL(_spin_unlock_irqrestore);

void __lockfunc _spin_unlock_irq(spinlock_t *lock)
{
	_raw_spin_unlock(lock);
	local_irq_enable();
	preempt_enable();
}
EXPORT_SYMBOL(_spin_unlock_irq);

void __lockfunc _spin_unlock_bh(spinlock_t *lock)
{
	_raw_spin_unlock(lock);
	preempt_enable();
	local_bh_enable();
}
EXPORT_SYMBOL(_spin_unlock_bh);

void __lockfunc _read_unlock_irqrestore(rwlock_t *lock, unsigned long flags)
{
	_raw_read_unlock(lock);
	local_irq_restore(flags);
	preempt_enable();
}
EXPORT_SYMBOL(_read_unlock_irqrestore);

void __lockfunc _read_unlock_irq(rwlock_t *lock)
{
	_raw_read_unlock(lock);
	local_irq_enable();
	preempt_enable();
}
EXPORT_SYMBOL(_read_unlock_irq);

void __lockfunc _read_unlock_bh(rwlock_t *lock)
{
	_raw_read_unlock(lock);
	preempt_enable();
	local_bh_enable();
}
EXPORT_SYMBOL(_read_unlock_bh);

void __lockfunc _write_unlock_irqrestore(rwlock_t *lock, unsigned long flags)
{
	_raw_write_unlock(lock);
	local_irq_restore(flags);
	preempt_enable();
}
EXPORT_SYMBOL(_write_unlock_irqrestore);

void __lockfunc _write_unlock_irq(rwlock_t *lock)
{
	_raw_write_unlock(lock);
	local_irq_enable();
	preempt_enable();
}
EXPORT_SYMBOL(_write_unlock_irq);

void __lockfunc _write_unlock_bh(rwlock_t *lock)
{
	_raw_write_unlock(lock);
	preempt_enable();
	local_bh_enable();
}
EXPORT_SYMBOL(_write_unlock_bh);

int __lockfunc _spin_trylock_bh(spinlock_t *lock)
{
	local_bh_disable();
	preempt_disable();
	if (_raw_spin_trylock(lock))
		return 1;

	preempt_enable();
	local_bh_enable();
	return 0;
}
EXPORT_SYMBOL(_spin_trylock_bh);

int in_lock_functions(unsigned long addr)
{
	/* Linker adds these: start and end of __lockfunc functions */
	extern char __lock_text_start[], __lock_text_end[];

	return addr >= (unsigned long)__lock_text_start
	&& addr < (unsigned long)__lock_text_end;
}
EXPORT_SYMBOL(in_lock_functions);