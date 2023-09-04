# Trace extensions

Show traces with defferent levels and time stamps in cout/cerr/output/trace file

Don`t forget enable traces via 
```C++
#include <ext/traces/tracer.h>
ext::get_tracer()->EnableTraces(true);
```
Simple macroses:
Default information trace

	EXT_TRACE() << "My trace"; 

Debug information only for Debug build

	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION "called";
	
Error trace to cerr, mostly used in EXT_CHECK/EXT_EXPECT

	EXT_TRACE_ERR() << EXT_TRACE_FUNCTION "called";
	
Can be called for scope call function check. Trace start and end scope with the given text

	EXT_TRACE_SCOPE() << EXT_TRACE_FUNCTION << "Main function called with " << args;

