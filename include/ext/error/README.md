# Extensions for check values and generating dumps

#Exceptions
If traces enabled generate output with error on exception creation
```C++
	try
	{ 
		EXT_EXPECT(is_ok()) << "Something wrong!";
	}
	catch (...)
	{	
		try
		{
			std::throw_with_nested(ext::exception(EXT_SRC_LOCATION, "Job failed")); 
		}
		catch (...)
		{
			::MessageBox(NULL, ext::ManageExceptionText("Big bang"));
		}
	}
```
#Dumps

Allow to catch unhandled exceptions and generate dump file

Declare unhandled exceptions handler(called automatic on calling ext::dump::create_dump())
```C++
	void main()
	{
		EXT_DUMP_DECLARE_HANDLER();
		...
	}
```
If you need to catch error inside you code you add check:
```C++
    EXT_DUMP_IF(is_something_wrong());
``` 
In this case if debugger presents - it will be stopped here, otherwise generate dump file and **continue** execution, @see DEBUG_BREAK_OR_CREATE_DUMP.
Dump generation and debug break in case with EXT_DUMP_IF generates only once to avoid spam.