#include "debug.h"
#include "hook.h"


asmlinkage int (*original_call)(const char *, int, int);

asmlinkage int our_sys_open(const char *filename, int flags, int mode)
{
	TRACE_INFO("Redirecting to original");
	return original_call(filename, flags, mode);
}



static int __init hook_init(void)
{
	TRACE_INFO("Loading module");
	hook_open(our_sys_open);
	return 0;
}

static void __exit hook_exit(void){
	TRACE_INFO("Exit");
	unhook_open(original_call);
	TRACE_INFO("Exit Done");
}

module_init(hook_init);
module_exit(hook_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sasha ");
MODULE_DESCRIPTION("Targil module");
