
/*

	Using printf on the Arduino.
	by Michael McElligott
	
	Usage:
	Set a buffer size with _PRINTF_BUFFER_LENGTH_, default is 64 bytes, or about a single line
	Set output stream with _Stream_Obj_. eg; SerialUSB
	
	printf(format string, argument(s) list).
	printfn(): As above but appends a new line on each print; aka serial.println()
	
	eg; printf("%.2f, 0x%X", 1234.5678f, 32768);
	
	For a detailed list on printf specifiers:
	http://www.cplusplus.com/reference/cstdio/printf/
	and
	http://man7.org/linux/man-pages/man3/printf.3.html


		
	Tested with the Arduino Due 1.6.6
	Jan 2016	

*/

#ifndef _printf_h_
#define _printf_h_

#define _PRINTF_BUFFER_LENGTH_		128

#define _Stream_Obj_				Serial
	

#if 1
static char _pf_buffer_[_PRINTF_BUFFER_LENGTH_];
#else
extern char _pf_buffer_[_PRINTF_BUFFER_LENGTH_];
#endif


#define printf(a,...)														\
	do{																		\
	snprintf(_pf_buffer_, sizeof(_pf_buffer_), a, ##__VA_ARGS__);			\
	_Stream_Obj_.print(_pf_buffer_);										\
	}while(0)

#define printfn(a,...)														\
	do{																		\
	snprintf(_pf_buffer_, sizeof(_pf_buffer_), a"\r\n", ##__VA_ARGS__);		\
	_Stream_Obj_.print(_pf_buffer_);										\
	}while(0)



#endif


