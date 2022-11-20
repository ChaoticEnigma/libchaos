/*******************************************************************************
**                                  LibChaos                                  **
**                                 zerror.cpp                                 **
**                          (c) 2015 Charlie Waters                           **
*******************************************************************************/
#include "zerror.h"
#include "zlog.h"
#include "zmap.h"

#include <functional>

#define FUCK_WINDOWS 1

#if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS || LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_CYGWIN
    #include <stdlib.h>
    #include <windows.h>

    #ifndef FUCK_WINDOWS
        #include <imagehlp.h>
        #include <winerror.h>

        #include <winnt.h>
        #include <Psapi.h>

        #include <algorithm>
        #include "StackWalker.h"
        #include <list>
    #endif // FUCK_WINDOWS
#elif LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_MACOSX
    #include <sys/errno.h>
    #include <execinfo.h>
    #include <signal.h>
    #include <cstring>
    #include <stdio.h>
    #include <cstdlib>
    #include <errno.h>
#else
    #include <execinfo.h>
    #include <signal.h>
    #include <cstring>
    #include <stdio.h>
    #include <cstdlib>
    #include <errno.h>
    #define HAVE_DECL_BASENAME 1
    #ifdef IBERTY_DEMANGLE
        #include "demangle.h"
    #endif
#endif // LIBCHAOS_PLATFORM

//#include <assert.h>

namespace LibChaos {
namespace ZError {

// Map of signals to signal handling functions
ZMap<int, ZError::sigset> sigmap;

void zassert(bool condition){
    zassert(condition, "ZError zassert failed");
}
void zassert(bool condition, ZString message){
    if(!condition)
        throw ZException(message);
}

#if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS || LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_CYGWIN

#ifndef FUCK_WINDOWS
struct module_data {
    std::string image_name;
    std::string module_name;
    void *base_address;
    DWORD load_size;
};
typedef std::vector<module_data> ModuleList;

bool show_stack(std::ostream &, HANDLE hThread, CONTEXT& c);
//DWORD __stdcall TargetThread( void *arg );
//void ThreadFunc1();
//void ThreadFunc2();
//DWORD Filter( EXCEPTION_POINTERS *ep );
void *load_modules_symbols( HANDLE hProcess, DWORD pid );

STACKFRAME64 init_stack_frame(CONTEXT *c){
    STACKFRAME64 s;
#ifdef _M_X64
    s.AddrPC.Offset = c->Rip;
    s.AddrPC.Mode = AddrModeFlat;
    s.AddrStack.Offset = c->Rsp;
    s.AddrStack.Mode = AddrModeFlat;
    s.AddrFrame.Offset = c->Rbp;
    s.AddrFrame.Mode = AddrModeFlat;
#else
    s.AddrPC.Offset = c->Eip;
    s.AddrPC.Mode = AddrModeFlat;
    s.AddrStack.Offset = c->Esp;
    s.AddrStack.Mode = AddrModeFlat;
    s.AddrFrame.Offset = c->Ebp;
    s.AddrFrame.Mode = AddrModeFlat;
#endif
    return s;
}

class symbol {
    typedef IMAGEHLP_SYMBOL64 sym_type;
    sym_type *sym;
    static const int max_name_len = 1024;
public:
    symbol(HANDLE process, DWORD64 address) : sym((sym_type *)::operator new(sizeof(*sym) + max_name_len)) {
        memset(sym, '\0', sizeof(*sym) + max_name_len);
        sym->SizeOfStruct = sizeof(*sym);
        sym->MaxNameLength = max_name_len;
        DWORD64 displacement;

        SymGetSymFromAddr64(process, address, &displacement, sym);
        //if (!SymGetSymFromAddr64(process, address, &displacement, sym))
        //    throw(std::logic_error("Bad symbol"));
    }

    std::string name() { return std::string(sym->Name); }
    std::string undecorated_name() {
        std::vector<char> und_name(max_name_len);
        UnDecorateSymbolName(sym->Name, &und_name[0], max_name_len, UNDNAME_COMPLETE);
        std::string str(&und_name[0], strlen(&und_name[0]));
        return str;
    }
};

class get_mod_info {
    HANDLE process;
    static const int buffer_length = 4096;
public:
    get_mod_info(HANDLE h) : process(h){}

    module_data operator()(HMODULE module){
        module_data ret;
        char temp[buffer_length];
        MODULEINFO mi;

        GetModuleInformation(process, module, &mi, sizeof(mi));
        ret.base_address = mi.lpBaseOfDll;
        ret.load_size = mi.SizeOfImage;

        GetModuleFileNameEx(process, module, temp, sizeof(temp));
        ret.image_name = temp;
        GetModuleBaseName(process, module, temp, sizeof(temp));
        ret.module_name = temp;
        std::vector<char> img(ret.image_name.begin(), ret.image_name.end());
        std::vector<char> mod(ret.module_name.begin(), ret.module_name.end());
        SymLoadModule64(process, 0, &img[0], &mod[0], (DWORD64)ret.base_address, ret.load_size);
        return ret;
    }
};

void *load_modules_symbols(HANDLE process, DWORD pid) {
    ModuleList modules;

    DWORD cbNeeded;
    std::vector<HMODULE> module_handles(1);

    EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);
    module_handles.resize(cbNeeded/sizeof(HMODULE));
    EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);

    std::transform(module_handles.begin(), module_handles.end(), std::back_inserter(modules), get_mod_info(process));
    return modules[0].base_address;
}

ZString getStack(){
    ZString str;

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    //EXCEPTION_POINTERS *ep = GetExceptionInformation();
    //CONTEXT *c = ep->ContextRecord;
    CONTEXT con;
    CONTEXT *c = &con;
    RtlCaptureContext(c);
    //GetThreadContext(thread, c);

    int frame_number = 0;
    DWORD offset_from_symbol = 0;
    IMAGEHLP_LINE64 line = {0};

    //SymHandler handler(process);
    SymInitialize(process, NULL, false);

    DWORD add = SYMOPT_LOAD_LINES | SYMOPT_UNDNAME;
    DWORD remove = 0;
    DWORD symOptions = SymGetOptions();
    symOptions |= add;
    symOptions &= ~remove;
    SymSetOptions(symOptions);

    void *base = load_modules_symbols(process, GetCurrentProcessId());

    STACKFRAME64 s = init_stack_frame(c);

    line.SizeOfStruct = sizeof line;

    IMAGE_NT_HEADERS *h = ImageNtHeader(base);
    DWORD image_type = h->FileHeader.Machine;

    do {
        if (!StackWalk64(image_type, process, thread, &s, c, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
            return ZString();

        str << "\n" << frame_number << "\t";
        if ( s.AddrPC.Offset != 0 ) {
            str << symbol(process, s.AddrPC.Offset).undecorated_name();

            if (SymGetLineFromAddr64( process, s.AddrPC.Offset, &offset_from_symbol, &line ) )
                    str << "\t" << line.FileName << "(" << line.LineNumber << ")";
        }
        else
            str << "(No Symbols: PC == 0)";
        ++frame_number;
    } while (s.AddrReturn.Offset != 0);

    SymCleanup(process);
    return str;
}

/////////////////////////////

int addr2line(char const * const program_name, void const * const addr)
{
  char addr2line_cmd[512] = {0};

  /* have addr2line map the address to the relent line in the code */
  #ifdef __APPLE__
    /* apple does things differently... */
    sprintf(addr2line_cmd,"atos -o %.256s %p", program_name, addr);
  #else
    sprintf(addr2line_cmd,"addr2line -f -p -e %.256s %p\nPAUSE", program_name, addr);
  #endif

  /* This will print a nicely formatted string specifying the
     function and source line of the address */
  //return system(addr2line_cmd);

    char path[1024];
    ZString str;

    FILE *fp = popen(addr2line_cmd, "r");
    if(fp == NULL){
        return 1;
    }
    while(fgets(path, sizeof(path)-1, fp) != NULL){
      str += path;
    }
    pclose(fp);

    ELOG(str);
    return 0;
}

void windows_print_stacktrace(CONTEXT *context){
    //SymInitialize(GetCurrentProcess(), 0, true);

    //STACKFRAME64 frame = { 0 };
    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(frame));

//    bool first = true;
//    FIXED_ARRAY( buf, char, 300 );
//    size_t buflen = sizeof(buf);
//    // Indexes into MSJExceptionHandler data when m_Levels > 0
//    int i_line = 0; // Into char * m_Lines[]
//    int i_buf = 0;  // into char m_Buffer[]

    frame.AddrPC.Mode           = AddrModeFlat;
    frame.AddrStack.Mode        = AddrModeFlat;
    frame.AddrFrame.Mode        = AddrModeFlat;

    frame.AddrPC.Offset         = context->Rip;
    frame.AddrStack.Offset      = context->Rsp;
    frame.AddrFrame.Offset      = context->Rbp;

    HANDLE process = GetCurrentProcess();
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;

    DWORD64 pFrame = 0;
    DWORD64 pPrevFrame = 0;

    bool first = true;

    while(StackWalk64(machineType,
           process,
           GetCurrentThread(),
           &frame,
           context,
           NULL,
           SymFunctionTableAccess64,
           SymGetModuleBase64,
           NULL)){

        addr2line("zephyr.exe", (void*)frame.AddrPC.Offset);
        continue;

        pFrame = frame.AddrFrame.Offset;
        if(pFrame == 0)
            break;
        if(frame.AddrPC.Offset == frame.AddrReturn.Offset)
            break;

        if( ! first ) {
            if( pFrame <= pPrevFrame ) {
                // Sanity check
                break;
            }
            if( (pFrame - pPrevFrame) > 10000000 ) {
                // Sanity check
                break;
            }
        }
        if( (pFrame % sizeof(void *)) != 0 ) {
            // Sanity check
            break;
        }

        pPrevFrame = pFrame;
        first = false;

        // IMAGEHLP is wacky, and requires you to pass in a pointer to an
        // IMAGEHLP_SYMBOL structure.  The problem is that this structure is
        // variable length.  That is, you determine how big the structure is
        // at runtime.  This means that you can't use sizeof(struct).
        // So...make a buffer that's big enough, and make a pointer
        // to the buffer.  We also need to initialize not one, but TWO
        // members of the structure before it can be used.

        enum { emMaxNameLength = 512 };
        // Use union to ensure proper alignment
        union {
            SYMBOL_INFO symb;
            BYTE symbolBuffer[ sizeof(SYMBOL_INFO) + emMaxNameLength ];
        } u;
        PSYMBOL_INFO pSymbol = & u.symb;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = emMaxNameLength;

        PDWORD64 symDisplacement = 0;  // Displacement of the input address,
                                    // relative to the start of the symbol

        DWORD lineDisplacement = 0;
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(line);
        line.LineNumber = 0;
        BOOL bLine = FALSE;

        bLine = SymGetLineFromAddr64( process, frame.AddrPC.Offset, &lineDisplacement, &line );
        if(SymFromAddr(process, frame.AddrPC.Offset, symDisplacement, pSymbol)){
            if( bLine ) {
                LOG(frame.AddrPC.Offset << " " << pFrame << " " << pSymbol->Name << " line " << line.LineNumber);
//                _snprintf( buf, buflen,
//                    ADDR_FORMAT "  " ADDR_FORMAT "   %s() line %d\n",
//                    sf.AddrPC.Offset, pFrame,
//                    pSymbol->Name, line.LineNumber );
            } else {
                LOG(frame.AddrPC.Offset << " " << pFrame << " " << pSymbol->Name << " + " << (zu64)symDisplacement);
//                _snprintf( buf, buflen,
//                    ADDR_FORMAT "  " ADDR_FORMAT "  %s() + %X\n",
//                    sf.AddrPC.Offset, pFrame,
//                    pSymbol->Name, symDisplacement );
            }
        }
        else    // No symbol found.  Print out the logical address instead.
        {
            DWORD err = GetLastError();
//            FIXED_ARRAY( szModule , TCHAR, MAX_PATH );
//            szModule[0] = '\0';
            DWORD section = 0, offset = 0;

//            GetLogicalAddress((PVOID)frame.AddrPC.Offset, szModule, sizeof(szModule), section, offset );

//            _snprintf( buf, buflen,
//                ADDR_FORMAT "  " ADDR_FORMAT "  %04X:%08X %s (err = %d)\n",
//                sf.AddrPC.Offset, pFrame,
//                section, offset, szModule, err );
//            result = true;
        }

//        if( m_Levels == 0 ) {
//            dbbTrace::OutputString( buf, false );
//        } else {
//            // Save line
//            size_t l = strlen(buf);
//            if( i_line >= m_Levels || i_buf + l >= m_Bytes ) {
//                // We have saved all of the stack we can save
//                break;
//            }
//            buf[ l - 1 ] = '\0';    // Remove trailing '\n'
//            char * s = & m_Buffer[ i_buf ];
//            m_Lines[ i_line++ ] = s;
//            strncpy( s, buf, l );
//            i_buf += l;
//        }
    }

  SymCleanup( GetCurrentProcess() );
}

////////////////////////////////////

void appendCallTrace( std::string & errorMessage ){
    using namespace std;
    // Store the names of all the functions called here:
    list<string> functionNames;
    // Initialise the symbol table to get function names:
    SymInitialize(
        GetCurrentProcess(),        // Process to get symbol table for
        0,                          // Where to search for symbol files: dont care
        true                        // Get symbols for all dll's etc. attached to process
    );
    // Store the current stack frame here:
    STACKFRAME frame = { 0 };
    // Get processor information for the current thread:
    CONTEXT context; memset( & context , 0 , sizeof( CONTEXT ) );
    context.ContextFlags = CONTEXT_FULL;
    // Load the RTLCapture context function:
    HINSTANCE kernel32 = LoadLibrary("Kernel32.dll");
    typedef void ( * RtlCaptureContextFunc ) ( CONTEXT * ContextRecord );
    RtlCaptureContextFunc rtlCaptureContext = (RtlCaptureContextFunc) GetProcAddress( kernel32, "RtlCaptureContext" );
    // Capture the thread context
    rtlCaptureContext( & context );
    //context = *((EXCEPTION_POINTERS*)GetExceptionInformation())->ContextRecord;
    // Use this to fill the frame structure:
    frame.AddrPC.Offset         = context.Rip;      // Current location in program
    frame.AddrPC.Mode           = AddrModeFlat;     // Address mode for this pointer: flat 32 bit addressing
    frame.AddrStack.Offset      = context.Rsp;      // Stack pointers current value
    frame.AddrStack.Mode        = AddrModeFlat;     // Address mode for this pointer: flat 32 bit addressing
    frame.AddrFrame.Offset      = context.Rbp;      // Value of register used to access local function variables.
    frame.AddrFrame.Mode        = AddrModeFlat;     // Address mode for this pointer: flat 32 bit addressing

    // Keep getting stack frames from windows till there are no more left:

    while(StackWalk(
            IMAGE_FILE_MACHINE_AMD64 ,       // MachineType - x86
            GetCurrentProcess()     ,	    // Process to get stack trace for
            GetCurrentThread()      ,	    // Thread to get stack trace for
            & frame                 ,	    // Where to store next stack frame
            & context               ,	    // Pointer to processor context record
            0                       ,	    // Routine to read process memory: use the default ReadProcessMemory
            SymFunctionTableAccess  ,	    // Routine to access the modules symbol table
            SymGetModuleBase        ,       // Routine to access the modules base address
            0                               // Something to do with 16-bit code. Not needed.
        )){
            //------------------------------------------------------------------
            // Declare an image help symbol structure to hold symbol info and
            // name up to 256 chars This struct is of variable lenght though so
            // it must be declared as a raw byte buffer.
            //------------------------------------------------------------------
            static char symbolBuffer[ sizeof(IMAGEHLP_SYMBOL) + 255 ];
            memset( symbolBuffer , 0 , sizeof(IMAGEHLP_SYMBOL) + 255 );
            // Cast it to a symbol struct:
            IMAGEHLP_SYMBOL * symbol = (IMAGEHLP_SYMBOL*) symbolBuffer;
            // Need to set the first two fields of this symbol before obtaining name info:
            symbol->SizeOfStruct    = sizeof(IMAGEHLP_SYMBOL) + 255;
            symbol->MaxNameLength   = 254;
            // The displacement from the beginning of the symbol is stored here: pretty useless
            unsigned long long displacement = 0;
            // Get the symbol information from the address of the instruction pointer register:
            if(SymGetSymFromAddr(
                    GetCurrentProcess()     ,   // Process to get symbol information for
                    frame.AddrPC.Offset     ,   // Address to get symbol for: instruction pointer register
                    (DWORD64*) &displacement ,   // Displacement from the beginning of the symbol: whats this for ?
                    symbol                      // Where to save the symbol
                )){
                // Add the name of the function to the function list:
                char buffer[2048]; sprintf( buffer , "0x%08x %s" ,  frame.AddrPC.Offset , symbol->Name );
                functionNames.push_back(buffer);
            } else {
                // Print an unknown location:
                // functionNames.push_back("unknown location");
                char buffer[64]; sprintf( buffer , "0x%08x" ,  frame.AddrPC.Offset );
                functionNames.push_back(buffer);
            }
    }
    // Cleanup the symbol table:
    SymCleanup( GetCurrentProcess() );
    //--------------------------------------------------------------------------
    // Print the names of all the functions called
    //--------------------------------------------------------------------------
    {
        errorMessage += "\nBacktrace of locations visited by program: \n";
        // Store the number of functions printed here:
        unsigned count = 1;
        // Run through the list of function names
        list<string>::iterator i = functionNames.begin();
        list<string>::iterator e = functionNames.end();
        while ( i != e && count < 17 ){
            // Get this function name:
            string & name = * i; i++;
            // Add it to the error message
            errorMessage += "\n";
            static char buffer[8]; itoa(count,buffer,10); errorMessage += buffer;
            errorMessage += " - ";
            errorMessage += name;
            // Increment the number of functions printed
            count++;
        }
    }
}

#endif // FUCK_WINDOWS

ZArray<TraceFrame> getStackTrace(unsigned trim){
    ZArray<TraceFrame> trace;

#ifndef FUCK_WINDOWS
    CONTEXT c;
    //GetThreadContext(GetCurrentThread(), c);
    RtlCaptureContext(&c);
    windows_print_stacktrace(&c);

//    StackWalker sw;
//    sw.ShowCallstack();

//    std::string str;
//    try{

//    } catch(...){
//        appendCallTrace(str);
//    }
//    trace.push(str);

    trace.push(getStack());
#else
    ELOG("FUCK WINDOWS");
#endif // FUCK_WINDOWS

    return trace;
}

#elif LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_MACOSX

ZArray<TraceFrame> getStackTrace(unsigned trim){
    ZArray<TraceFrame> trace;
    void *buffer[256];

    // Get the backtrace
    int nptrs = backtrace(buffer, 256);

    // Get sybmol names for backtrace
    char **strings = backtrace_symbols(buffer, nptrs);
    if(strings != NULL){
        ArZ strs;
        for(int i = (int)trim; i < nptrs; ++i){
            strs.push(strings[i]);
        }
        for(zu64 i = 0; i < strs.size(); ++i){
            ZString tmp = strs[i];

            TraceFrame frame;
            frame.i = i;
            frame.addr = (zu64)buffer[i + trim];
            frame.line = 0;

            ArZ tok = tmp.split(' ');
            if(tok.size() == 6){
                frame.exec = tok[1];
                frame.symbol = tok[3];
                frame.offset = tok[5];
            }

            trace.push(frame);
        }
        free(strings);
    }
    return trace;
}

#else // LIBCHAOS_PLATFORM

struct backtrace_parts {
    ZPath exec;
    ZString func_symbol;
    ZString func_name;
    ZString func_addr;
    ZString addr;
};

struct addr2line_parts {
    ZString symbol;
    ZString name;
    ZPath file;
    unsigned line;
};

addr2line_parts addr2line(ZPath program, const void *addr){
    ZString addrcmd = "addr2line -fCe " + program.str() + " " + ZString::ItoS((zu64)addr, 16);

    char output[256];
    ZString str;

    // Run addr2line
    FILE *fp = popen(addrcmd.cc(), "r");
    if(fp == NULL){
        return { "??", "??", "??", 0 };
    }
    while(fgets(output, 256, fp) != NULL){
        str += output;
    }
    pclose(fp);


    // Parse result
    ArZ lines = str.split('\n');
    if(lines.size() < 2){
        return { "??", "??", "??", 0 };
    }
    ArZ tok = lines[1].split(' ');
    zu64 pos = ZString::findFirst(tok[0], "(");
    ZString name = ZString::substr(tok[0], 0, pos).strip('\n');
    ArZ fname = name.split(':');

    addr2line_parts source;
    source.name = lines[0];
    source.file = fname[0];
    zu64 line = fname[1].toUint();
    if(line == ZU64_MAX)
        source.line = 0;
    else
        source.line = line;
    return source;
}

ZArray<TraceFrame> getStackTrace(unsigned trim){
    ZArray<TraceFrame> trace;
    void *buffer[256];

    // Get the backtrace
    int nptrs = backtrace(buffer, 256);

    // Get sybmol names for backtrace
    char **strings = backtrace_symbols(buffer, nptrs);
    if(strings != NULL){
        ArZ strs;
        for(int i = (int)trim; i < nptrs; ++i){
            strs.push(strings[i]);
        }
        //strs.popFrontCount(trim);
        for(zu64 i = 0; i < strs.size(); ++i){
            ZString tmp = strs[i];

            backtrace_parts frame;

            // Module
            zu64 pos = ZString::findFirst(tmp, "(");
            frame.exec = ZString::substr(tmp, 0, pos);
            tmp.substr(pos);

            // Function
            pos = ZString::findFirst(tmp, "[");
            ZString funcstr = ZString::substr(tmp, 0, pos);
            tmp.substr(pos);
            ArZ fpts = funcstr.strip(' ').strip('(').strip(')').split('+');
            frame.func_symbol = fpts[0];
#ifdef IBERTY_DEMANGLE
            frame.func_name = cplus_demangle(frame.func_symbol.cc(), 0);
#else
            frame.func_name = frame.func_symbol;
#endif
            frame.func_addr = fpts[1];

            // Address
            frame.addr = tmp.strip('[').strip(']');

            // Get source file information
            addr2line_parts source = addr2line(frame.exec, buffer[i+trim]);

            // Make trace frame
            TraceFrame traceframe;
            traceframe.i = i;

            traceframe.exec = frame.exec;
            traceframe.addr = (zu64)buffer[i + trim];

            traceframe.file = source.file;
            traceframe.line = source.line;

            traceframe.symbol = frame.func_symbol;
            traceframe.name = source.name;
            traceframe.offset = frame.func_addr;

            trace.push(traceframe);
        }
        free(strings);
    }
    return trace;
}

#endif // LIBCHAOS_PLATFORM

ZString traceFrameStr(const TraceFrame &frame){
    ZString str;
    str << frame.i << " - ";

    if(frame.exec.isEmpty())
        str << "?? ";
    else
        str << frame.exec.last() << " ";

    str << "[0x" << ZString::ItoS(frame.addr, 16) << "] ";

    if(frame.name.isEmpty() || frame.name == "??")
        if(frame.symbol.isEmpty())
            str << "?? + ?? @ ";
        else
            str << frame.symbol << " + " << frame.offset << " @ ";
    else
        str << frame.name << " + " << frame.offset << " @ ";

    if(frame.file.isEmpty())
        str << "?? : 0";
    else
        str << frame.file.last() << " : " << frame.line;

    return str;
}

void fatalSignalHandler(int sig){
    ELOG("Fatal Error: signal " << sig);

    // Try to log a stack trace
    // Drop the top three frames in the trace:
    //  ZError::getStackTrace
    //  ZError::fatalSignalHnadler
    //  libc signal handler
    ZArray<TraceFrame> trace = ZError::getStackTrace(3);
    for(zu64 i = 0; i < trace.size(); ++i){
        ELOG(ZLog::RAW << traceFrameStr(trace[i]) << ZLog::NEWLN);
    }

    exit(sig);
}

void registerSigSegv(){
#if LIBCHAOS_PLATFORM != LIBCHAOS_PLATFORM_WINDOWS
    signal(SIGSEGV, fatalSignalHandler);
#endif
}

#if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS || LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_CYGWIN

BOOL WINAPI ConsoleHandler(DWORD dwType){
    LOG("Console Exit Handler " << dwType);
//    switch(dwType){
//    case CTRL_C_EVENT:
//        printf("ctrl-c\n");
//        break;
//    case CTRL_BREAK_EVENT:
//        printf("break\n");
//        break;
//    default:
//        printf("Some other event\n");
//        break;
//    }
    return TRUE;
}

#else

void sigHandle(int sig){
    if(sigmap.contains(sig) && sigmap[sig].handler != NULL)
        (sigmap[sig].handler)(sigmap[sig].sigtype);
}

#endif // LIBCHAOS_PLATFORM

bool registerSignalHandler(zerror_signal sigtype, signalHandler handler){

#if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS || LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_CYGWIN

    if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE)){
        return false;
    }

#else

    ZMap<zerror_signal, int> sigsmap = {
        { INTERRUPT,    SIGINT },
        { QUIT,         SIGQUIT },
        { ILLEGAL,      SIGILL },
        { ABORT,        SIGABRT },
        { FPE,          SIGFPE },
        { SEGV,         SIGSEGV },
        { PIPE,         SIGPIPE },
        { ALARM,        SIGALRM },
        { TERMINATE,    SIGTERM },
        { USER1,         SIGUSR1 },
        { USER2,         SIGUSR2 },
    };
    int sig = sigsmap[sigtype];

    sigmap[sig] = { sigtype, handler };

    uint8_t alternate_stack[SIGSTKSZ];
    stack_t ss;
     /* malloc is usually used here, I'm not 100% sure my static allocation
     is valid but it seems to work just fine. */
    ss.ss_sp = (char*)alternate_stack;
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;

    if(sigaltstack(&ss, NULL) != 0){
        throw ZException("sigaltstack");
    }

    struct sigaction action;
    action.sa_handler = sigHandle;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, sig);

    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    if(sigaction(sig, &action, 0) != 0){
        throw ZException("sigaction");
    }

#endif // LIBCHAOS_PLATFORM

    return true;
}

bool registerInterruptHandler(signalHandler handler){
    return registerSignalHandler(INTERRUPT, handler);
}

#if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS
unsigned long getSystemErrorCode(){
    return GetLastError();
}
#else
int getSystemErrorCode(){
    return errno;
}
#endif

ZString getSystemError(){
#if  LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS
    DWORD err = GetLastError();
//    wchar_t *str = nullptr;
//    TCHAR *str = nullptr;
    TCHAR buffer[1024];
    DWORD size = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               err,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               buffer,
                               1024,
                               NULL);
    ZString errstr(buffer, size);
//    LocalFree(str);
    return ZString(err) + ": " + errstr;
#else
    int err = errno;
    return ZString() << err << ": " << strerror(err);
#endif
}

int getSocketErrorCode(){
#if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS
    return WSAGetLastError();
#else
    return errno;
#endif
}

ZString getSocketError(){
#if  LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS
    DWORD err = WSAGetLastError();
//    wchar_t *str = nullptr;
//    TCHAR *str = nullptr;
    TCHAR buffer[1024];
    DWORD size = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               err,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               buffer,
                               1024,
                               NULL);
    ZString errstr(buffer, size);
//    LocalFree(str);
    return ZString(err) + ": " + errstr;
#else
    int err = errno;
    return ZString() << err << ": " << strerror(err);
#endif
}

} // namespace ZError
} // namespace LibChaos
