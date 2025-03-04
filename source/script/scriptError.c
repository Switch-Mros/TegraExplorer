#include "scriptError.h"
#include "compat.h"
#include <stdarg.h>

s64 scriptCurrentLine;
u8 scriptLastError = 0;

void printScriptError(u8 errLevel, char* message, ...) {
	va_list args;
	scriptLastError = errLevel;
	va_start(args, message);
	gfx_printf("\n\n[%s] ", (errLevel == SCRIPT_FATAL) ? "FATAL" : (errLevel == SCRIPT_PARSER_FATAL) ? "PARSE_FATAL" : "WARNUNG");
	gfx_vprintf(message, args);
	if (errLevel < SCRIPT_WARN)
		gfx_printf("\nFehler aufgetreten in oder nahe der Zeile %d\n", (u32)scriptCurrentLine);
	va_end(args);
	#ifndef WIN32
		hidWait();
	#endif
}