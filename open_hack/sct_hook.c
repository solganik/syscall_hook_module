#include <asm/unistd.h>
#include <asm/unistd.h>
#include <linux/preempt.h>
#include <linux/stop_machine.h>
#include <linux/kallsyms.h>
#include "utils.h"
#include "udis_utils.h"
#include "debug.h"
#include "hook.h"

void **sys_call_table = NULL;
static void **sct_map = NULL;

void *get_sys_call_table(void){
	int low, high;
	void *system_call;

	asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (0xc0000082));
	system_call = (void *)(((u64)high << 32) | low);

	return ud_find_syscall_table_addr(system_call);
}

static void *find_syscalltable_address(void)
{
	return (void *)kallsyms_lookup_name("sys_call_table");
}

static int get_sct(void)
{
	sys_call_table = find_syscalltable_address();
	if(!sys_call_table){
		TRACE_INFO("syscall_table is NULL, quitting...");
		return 0;
	}else{
		TRACE_INFO("Found syscall_table addr at 0x%p\n", sys_call_table);
	}
	return 1;
}

struct _hook_struct{
	open_syscall_type new_cb;
	open_syscall_type orig_cb;
};

int do_hook_open(void *args)
{
	struct _hook_struct* param = (struct _hook_struct *)args;
	param->orig_cb = sct_map[__NR_open];
	sct_map[__NR_open] = param->new_cb;
	return 0;
}

open_syscall_type hook_open(open_syscall_type cb)
{
	struct _hook_struct param = {.orig_cb = NULL, .new_cb = cb};
	if(!get_sct())
		return NULL;

	sct_map = map_writable(sys_call_table, (__NR_open + 1) * sizeof(void *));
	if(!sct_map){
		TRACE_ERROR("Can't get writable SCT mapping\n");
		goto out;
	}

	stop_machine(do_hook_open, &param, 0);
out:
	vunmap((void *)((unsigned long)sct_map & PAGE_MASK)), sct_map = NULL;
	return param.orig_cb;
}



void unhook_open(open_syscall_type original)
{
	if(!get_sct())
		return;

	sct_map = map_writable(sys_call_table, (__NR_open + 1) * sizeof(void *));
	if(!sct_map){
		TRACE_ERROR("Can't get writable SCT mapping");
		goto out;
	}
	TRACE_INFO("Unhooking open");
	sct_map[__NR_open] = original;
out:
	vunmap((void *)((unsigned long)sct_map & PAGE_MASK)), sct_map = NULL;
}
