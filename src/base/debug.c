/* https://gist.github.com/jvranish/4441299 */


/* compile with:
on linux:   gcc -g stack_traces.c
on OS X:    gcc -g -fno-pie stack_traces.c
on windows: gcc -g stack_traces.c -limagehlp
*/

#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
  #include <imagehlp.h>
#else
#include <err.h>
#include <execinfo.h>
#include <game/version.h>

#endif

#include "debug.h"
#include "system.h"

#if defined(__cplusplus)
extern "C" {
#endif

static char const * icky_global_program_name;
static char const * icky_global_error_string;
static int icky_global_error_sig;


int crashreport_prepare(const char *pFile)
{
	char aBuf[768];

	IOHANDLE f = io_open(pFile, IOFLAG_WRITE);
	if(!f)
	{
		printf("<<<< FAILED TO OPEN '%s' FOR WRITING >>>>\n", pFile);
		printf("<<<<  no crash report will be saved!  >>>>\n\n\n");
	}
	else
	{
		#define WRITE_LINE() \
				io_write(f, aBuf, str_length(aBuf)); \
				io_write_newline(f)

		str_format(aBuf, sizeof(aBuf), "### %s", pFile);
		WRITE_LINE();
		io_write_newline(f);

		str_format(aBuf, sizeof(aBuf), "EXECUTABLE: '%s'", icky_global_program_name);
		WRITE_LINE();

		str_format(aBuf, sizeof(aBuf), "OS: " CONF_FAMILY_STRING "/" CONF_PLATFORM_STRING);
		WRITE_LINE();

		str_format(aBuf, sizeof(aBuf), "VERSION: " ALLTHEHAXX_VERSION ".%i.%i (" GAME_VERSION "/" ATH_VERSION ")", GAME_ATH_VERSION_NUMERIC, CLIENT_VERSIONNR);
		WRITE_LINE();

		str_format(aBuf, sizeof(aBuf), "ERRSIG: %i (%s)", icky_global_error_sig, icky_global_error_string);
		WRITE_LINE();

		io_write_newline(f);
		io_write_newline(f);

		str_format(aBuf, sizeof(aBuf), "******* BEGIN CALLSTACK *******");
		WRITE_LINE();
		#undef WRITE_LINE

		io_flush(f);
		io_close(f);
		return 1;
	}
	return 0;
}

void crashreport_finish(int ReportExists, const char *pFile)
{
	char aBuf[768];
	char aMsg[512];

	str_format(aMsg, sizeof(aMsg), "Whoups, the client just crashed. I am sorry... ");

	if(ReportExists)
	{
		str_format(aBuf, sizeof(aBuf), "A crash report has been saved to \"%s\" If you send it to us, we can fix that."
				"   [If your mouse is still grabbed, you can press either ENTER or ALt+F4 to close this]", pFile);

	}
	else
		str_format(aBuf, sizeof(aBuf), "Unfortunately, we couldn\"t write to the file \"%s\" to save a report :/", pFile);

	str_append(aMsg, aBuf, sizeof(aMsg));

	const char *paFunnyMessages[] = { // these should - please - fit to a crash. Thanks in advance.
			"I know what you think...",
			"This was not supposed to happen!",
			"Who is responsible for that mess?",
			"That\\'s not my fault!",
			"My cat ate the alien intelligence",
			"What are we going to do now?",
			"YOU DIDN\\'T SAW THAT! IT\\'S CONFIDENTIAL!",
			"Please act as if nothing had happened",
			"Panda??",
			"Paper > All ",
			"Grab a pencil and a pigeon",
			"asm()",
	};

	size_t n = rand()%(sizeof(paFunnyMessages)/sizeof(paFunnyMessages[0]));
	str_format(aBuf, sizeof(aBuf), "AllTheHaxx Crash  --  %s", paFunnyMessages[n]);
	gui_messagebox(aBuf, aMsg);

}




void set_signal_handler();

void init_debugger(const char *argv0)
{
	icky_global_program_name = argv0;
	set_signal_handler();
}

/* Resolve symbol name and source location given the path to the executable
   and an address */
int addr2line(char const * const program_name, void const * const addr, const char *file)
{
	char addr2line_cmd[512] = {0};

	/* have addr2line map the address to the relent line in the code */
	#ifdef __APPLE__
	/* apple does things differently... */
    sprintf(addr2line_cmd,"atos -o '%.256s' %p >> '%s'", program_name, addr, file);
	#else
	sprintf(addr2line_cmd,"addr2line -f -p -e '%.256s' %p >> '%s'", program_name, addr, file);
	#endif

	return system(addr2line_cmd);
}


#ifdef _WIN32
void windows_print_stacktrace(CONTEXT* context)
  {
    SymInitialize(GetCurrentProcess(), 0, true);

    STACKFRAME frame = { 0 };

    /* setup initial stack frame */
    frame.AddrPC.Offset         = context->Eip;
    frame.AddrPC.Mode           = AddrModeFlat;
    frame.AddrStack.Offset      = context->Esp;
    frame.AddrStack.Mode        = AddrModeFlat;
    frame.AddrFrame.Offset      = context->Ebp;
    frame.AddrFrame.Mode        = AddrModeFlat;

    while (StackWalk(IMAGE_FILE_MACHINE_I386 ,
                     GetCurrentProcess(),
                     GetCurrentThread(),
                     &frame,
                     context,
                     0,
                     SymFunctionTableAccess,
                     SymGetModuleBase,
                     0 ) )
    {
      addr2line(icky_global_program_name, (void*)frame.AddrPC.Offset);
    }

    SymCleanup( GetCurrentProcess() );
  }

  LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo)
  {
    switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
      case EXCEPTION_ACCESS_VIOLATION:
        fputs("Error: EXCEPTION_ACCESS_VIOLATION\n", stderr);
        break;
      case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        fputs("Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n", stderr);
        break;
      case EXCEPTION_BREAKPOINT:
        fputs("Error: EXCEPTION_BREAKPOINT\n", stderr);
        break;
      case EXCEPTION_DATATYPE_MISALIGNMENT:
        fputs("Error: EXCEPTION_DATATYPE_MISALIGNMENT\n", stderr);
        break;
      case EXCEPTION_FLT_DENORMAL_OPERAND:
        fputs("Error: EXCEPTION_FLT_DENORMAL_OPERAND\n", stderr);
        break;
      case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        fputs("Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n", stderr);
        break;
      case EXCEPTION_FLT_INEXACT_RESULT:
        fputs("Error: EXCEPTION_FLT_INEXACT_RESULT\n", stderr);
        break;
      case EXCEPTION_FLT_INVALID_OPERATION:
        fputs("Error: EXCEPTION_FLT_INVALID_OPERATION\n", stderr);
        break;
      case EXCEPTION_FLT_OVERFLOW:
        fputs("Error: EXCEPTION_FLT_OVERFLOW\n", stderr);
        break;
      case EXCEPTION_FLT_STACK_CHECK:
        fputs("Error: EXCEPTION_FLT_STACK_CHECK\n", stderr);
        break;
      case EXCEPTION_FLT_UNDERFLOW:
        fputs("Error: EXCEPTION_FLT_UNDERFLOW\n", stderr);
        break;
      case EXCEPTION_ILLEGAL_INSTRUCTION:
        fputs("Error: EXCEPTION_ILLEGAL_INSTRUCTION\n", stderr);
        break;
      case EXCEPTION_IN_PAGE_ERROR:
        fputs("Error: EXCEPTION_IN_PAGE_ERROR\n", stderr);
        break;
      case EXCEPTION_INT_DIVIDE_BY_ZERO:
        fputs("Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n", stderr);
        break;
      case EXCEPTION_INT_OVERFLOW:
        fputs("Error: EXCEPTION_INT_OVERFLOW\n", stderr);
        break;
      case EXCEPTION_INVALID_DISPOSITION:
        fputs("Error: EXCEPTION_INVALID_DISPOSITION\n", stderr);
        break;
      case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        fputs("Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n", stderr);
        break;
      case EXCEPTION_PRIV_INSTRUCTION:
        fputs("Error: EXCEPTION_PRIV_INSTRUCTION\n", stderr);
        break;
      case EXCEPTION_SINGLE_STEP:
        fputs("Error: EXCEPTION_SINGLE_STEP\n", stderr);
        break;
      case EXCEPTION_STACK_OVERFLOW:
        fputs("Error: EXCEPTION_STACK_OVERFLOW\n", stderr);
        break;
      default:
        fputs("Error: Unrecognized Exception\n", stderr);
        break;
    }
    fflush(stderr);
    /* If this is a stack overflow then we can't walk the stack, so just show
      where the error happened */
    if (EXCEPTION_STACK_OVERFLOW != ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
        windows_print_stacktrace(ExceptionInfo->ContextRecord);
    }
    else
    {
        addr2line(icky_global_program_name, (void*)ExceptionInfo->ContextRecord->Eip);
    }

    return EXCEPTION_EXECUTE_HANDLER;
  }

  void set_signal_handler()
  {
    SetUnhandledExceptionFilter(windows_exception_handler);
  }
#else

#define MAX_STACK_FRAMES 64
static void *stack_traces[MAX_STACK_FRAMES];
void posix_print_stack_trace()
{
	fs_makedir("./crashlogs");
	char aFile[512];
	time_t rawtime;
	time(&rawtime);
	str_timestamp_ex(rawtime, aFile, sizeof(aFile), "crashlogs/report_%Y-%m-%d_%H-%M-%S.log");

	int ReportWritten = crashreport_prepare(aFile);

	int i, trace_size = 0;
	char **messages = (char **)NULL;

	trace_size = backtrace(stack_traces, MAX_STACK_FRAMES);
	messages = backtrace_symbols(stack_traces, trace_size);

	/* skip the first couple stack frames (as they are this function and
	 our handler) and also skip the last frame as it's (always?) junk. */
	// for (i = 3; i < (trace_size - 1); ++i)
	for (i = 0; i < trace_size; ++i) // we'll use this for now so you can see what's going on
	{
		if (addr2line(icky_global_program_name, stack_traces[i], aFile) != 0)
		{
			printf("  error determining line # for: %s\n", messages[i]);
		}

	}
	if (messages) { free(messages); }

	crashreport_finish(ReportWritten, aFile);
}

void posix_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
	(void)context;
	icky_global_error_string = "unknown error";
	switch(sig)
	{
		case SIGSEGV:
			icky_global_error_string = "Caught SIGSEGV: Segmentation Fault"; break;
		case SIGINT:
			icky_global_error_string = "Caught SIGINT: Interactive attention signal, (usually ctrl+c)"; break;
		case SIGFPE:
			switch(siginfo->si_code)
			{
				case FPE_INTDIV:
					icky_global_error_string = "Caught SIGFPE: (integer divide by zero)"; break;
				case FPE_INTOVF:
					icky_global_error_string = "Caught SIGFPE: (integer overflow)"; break;
				case FPE_FLTDIV:
					icky_global_error_string = "Caught SIGFPE: (floating-point divide by zero)"; break;
				case FPE_FLTOVF:
					icky_global_error_string = "Caught SIGFPE: (floating-point overflow)"; break;
				case FPE_FLTUND:
					icky_global_error_string = "Caught SIGFPE: (floating-point underflow)"; break;
				case FPE_FLTRES:
					icky_global_error_string = "Caught SIGFPE: (floating-point inexact result)"; break;
				case FPE_FLTINV:
					icky_global_error_string = "Caught SIGFPE: (floating-point invalid operation)"; break;
				case FPE_FLTSUB:
					icky_global_error_string = "Caught SIGFPE: (subscript out of range)"; break;
				default:
					icky_global_error_string = "Caught SIGFPE: Arithmetic Exception"; break;
			} break;
		case SIGILL:
			switch(siginfo->si_code)
			{
				case ILL_ILLOPC:
					icky_global_error_string = "Caught SIGILL: (illegal opcode)"; break;
				case ILL_ILLOPN:
					icky_global_error_string = "Caught SIGILL: (illegal operand)"; break;
				case ILL_ILLADR:
					icky_global_error_string = "Caught SIGILL: (illegal addressing mode)"; break;
				case ILL_ILLTRP:
					icky_global_error_string = "Caught SIGILL: (illegal trap)"; break;
				case ILL_PRVOPC:
					icky_global_error_string = "Caught SIGILL: (privileged opcode)"; break;
				case ILL_PRVREG:
					icky_global_error_string = "Caught SIGILL: (privileged register)"; break;
				case ILL_COPROC:
					icky_global_error_string = "Caught SIGILL: (coprocessor error)"; break;
				case ILL_BADSTK:
					icky_global_error_string = "Caught SIGILL: (internal stack error)"; break;
				default:
					icky_global_error_string = "Caught SIGILL: Illegal Instruction"; break;
			} break;
		case SIGTERM:
			icky_global_error_string = "Caught SIGTERM: a termination request was sent to the program"; break;
		case SIGABRT:
			icky_global_error_string = "Caught SIGABRT: usually caused by an abort() or assert()"; break;
	}

	icky_global_error_sig = sig;
	fputs(icky_global_error_string, stderr);
	fputs("\n", stderr);

	posix_print_stack_trace();
	_Exit(1);
}

static uint8_t alternate_stack[SIGSTKSZ];
void set_signal_handler()
{
	/* setup alternate stack */
	{
		stack_t ss = {};
		/* malloc is usually used here, I'm not 100% sure my static allocation
		   is valid but it seems to work just fine. */
		ss.ss_sp = (void*)alternate_stack;
		ss.ss_size = SIGSTKSZ;
		ss.ss_flags = 0;

		if (sigaltstack(&ss, NULL) != 0) { err(1, "sigaltstack"); }
	}

	/* register our signal handlers */
	{
		struct sigaction sig_action = {};
		sig_action.sa_sigaction = posix_signal_handler;
		sigemptyset(&sig_action.sa_mask);

		#ifdef __APPLE__
		/* for some reason we backtrace() doesn't work on osx
             when we use an alternate stack */
          sig_action.sa_flags = SA_SIGINFO;
		#else
		sig_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
		#endif

		if (sigaction(SIGSEGV, &sig_action, NULL) != 0) { err(1, "sigaction"); }
		if (sigaction(SIGFPE,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
		if (sigaction(SIGINT,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
		if (sigaction(SIGILL,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
		if (sigaction(SIGTERM, &sig_action, NULL) != 0) { err(1, "sigaction"); }
		if (sigaction(SIGABRT, &sig_action, NULL) != 0) { err(1, "sigaction"); }
	}
}
#endif


#if defined(__cplusplus)
}
#endif
