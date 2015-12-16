#ifndef __COMMON_DEBUG_H__
#define __COMMON_DEBUG_H__


#	ifdef __KERNEL__

#		define _TRACE_DEBUG( format, args... ) do {\
			pr_debug( "STRATO: DEBUG : " format " (" __stringify(__FILE__) ":%s:" __stringify(__LINE__) ")\n", ##args,__func__ ); \
		}while(0);

#		define _TRACE( level, levelName, format, args... ) do { \
			printk( level "STRATO: " levelName ": " format " (" __stringify(__FILE__) ":%s:" __stringify(__LINE__) ")\n", ##args, __func__ ); \
		} while( 0 )



#			define _MASS_TRACE( levelName, format, args... ) do {} while( 0 )



#		ifdef DEBUG
#			include <linux/stringify.h>
#			include <linux/delay.h>
#			include <linux/bug.h>

#			define 	_ASSERT_BUG_ON(condition) BUG_ON(!(condition))
#			define _ASSERT(condition, format, args...) do { \
				if (!(condition)) { \
					panic( "Assertion '%s' failed (" __stringify(__FILE__) ":" __stringify(__LINE__) ")" format "\nPanicing on purpose\n", #condition, ##args ); \
				} \
			} while( 0 )

#		else

#			define _ASSERT(condition, format, args... ) do {} while( 0 )
#			define 	_ASSERT_BUG_ON(condition) do {} while( 0 )

#		endif
#	endif // ! __KERNEL__


#define TRACE_KERNEL_EMERGENCY( format, args... ) _TRACE( KERN_EMERG, "EMERGENCY", format, ##args )
#define TRACE_DEBUG( format, args... ) _TRACE_DEBUG( format, ##args )
#define TRACE_INFO( format, args... ) _TRACE( KERN_NOTICE, "INFO", format, ##args )
#define TRACE_WARNING( format, args... ) _TRACE( KERN_WARNING, "WARNING", format, ##args )
#define TRACE_ERROR( format, args... ) _TRACE( KERN_ERR, "ERROR", format, ##args )
#define MASS_TRACE_INFO( format, args... ) _MASS_TRACE( "INFO", format, ##args )
#define MASS_TRACE_WARNING( format, args... ) _MASS_TRACE( "WARNING", format, ##args )
#define MASS_TRACE_ERROR( format, args... ) _MASS_TRACE( "ERROR", format, ##args )
#define ASSERT( x ) _ASSERT( x, "" )
#define ASSERT_VERBOSE( x, format, args... ) _ASSERT( x, ": " format, ##args )
#define ASSERT_BUG_ON(x) _ASSERT_BUG_ON(x)

#define _TRACE_ONCE( level, levelName, format, ...)			\
	({								\
        static bool __print_once;					\
									\
        if ( !__print_once ) {						\
                __print_once = true;					\
                _TRACE( level, levelName, format, ##__VA_ARGS__ );	\
        }								\
})

#define TRACE_EMERGENCY_ONCE( format, args... ) _TRACE_ONCE( KERN_EMERG, "EMERGENCY", format, ##args )
#define TRACE_DEBUG_ONCE( format, args... ) _TRACE_ONCE( KERN_DEBUG, "DEBUG", format, ##args )
#define TRACE_INFO_ONCE( format, args... ) _TRACE_ONCE( KERN_NOTICE, "INFO", format, ##args )
#define TRACE_WARNING_ONCE( format, args... ) _TRACE_ONCE( KERN_WARNING, "WARNING", format, ##args )
#define TRACE_ERROR_ONCE( format, args... ) _TRACE_ONCE( KERN_ERR, "ERROR", format, ##args )

#endif // __COMMON_DEBUG_H__

