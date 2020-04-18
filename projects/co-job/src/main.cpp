#include "engine/pch.h"

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include "engine/application/Application.hpp"
#include <easy/profiler.h>
#include "schedule/scheduler.hpp"
#include <cppcoro/when_all.hpp>
#include <chrono>
#include "schedule/token.hpp"
#include "cppcoro/sync_wait.hpp"
#include <fmt/color.h>

#include "engine/async/async.hpp"
#include "engine/core/random.hpp"
#include "pt/tracer.hpp"
#include "schedule/task.hpp"
// #include "pt/tracer.hpp"

void BindCrtHandlesToStdHandles( bool bindStdIn, bool bindStdOut, bool bindStdErr )
{
   // Re-initialize the C runtime "FILE" handles with clean handles bound to "nul". We do this because it has been
   // observed that the file number of our standard handle file objects can be assigned internally to a value of -2
   // when not bound to a valid target, which represents some kind of unknown internal invalid state. In this state our
   // call to "_dup2" fails, as it specifically tests to ensure that the target file number isn't equal to this value
   // before allowing the operation to continue. We can resolve this issue by first "re-opening" the target files to
   // use the "nul" device, which will place them into a valid state, after which we can redirect them to our target
   // using the "_dup2" function.
   if(bindStdIn) {
      FILE* dummyFile;
      freopen_s( &dummyFile, "nul", "r", stdin );
   }
   if(bindStdOut) {
      FILE* dummyFile;
      freopen_s( &dummyFile, "nul", "w", stdout );
   }
   if(bindStdErr) {
      FILE* dummyFile;
      freopen_s( &dummyFile, "nul", "w", stderr );
   }

   // Redirect unbuffered stdin from the current standard input handle
   if(bindStdIn) {
      HANDLE stdHandle = GetStdHandle( STD_INPUT_HANDLE );
      if(stdHandle != INVALID_HANDLE_VALUE) {
         int fileDescriptor = _open_osfhandle( (intptr_t)stdHandle, _O_TEXT );
         if(fileDescriptor != -1) {
            FILE* file = _fdopen( fileDescriptor, "r" );
            if(file != NULL) {
               int dup2Result = _dup2( _fileno( file ), _fileno( stdin ) );
               if(dup2Result == 0) { setvbuf( stdin, NULL, _IONBF, 0 ); }
            }
         }
      }
   }

   // Redirect unbuffered stdout to the current standard output handle
   if(bindStdOut) {
      HANDLE stdHandle = GetStdHandle( STD_OUTPUT_HANDLE );
      if(stdHandle != INVALID_HANDLE_VALUE) {
         int fileDescriptor = _open_osfhandle( (intptr_t)stdHandle, _O_TEXT );
         if(fileDescriptor != -1) {
            FILE* file = _fdopen( fileDescriptor, "w" );
            if(file != NULL) {
               int dup2Result = _dup2( _fileno( file ), _fileno( stdout ) );
               if(dup2Result == 0) { setvbuf( stdout, NULL, _IONBF, 0 ); }
            }
         }
      }
   }

   // Redirect unbuffered stderr to the current standard error handle
   if(bindStdErr) {
      HANDLE stdHandle = GetStdHandle( STD_ERROR_HANDLE );
      if(stdHandle != INVALID_HANDLE_VALUE) {
         int fileDescriptor = _open_osfhandle( (intptr_t)stdHandle, _O_TEXT );
         if(fileDescriptor != -1) {
            FILE* file = _fdopen( fileDescriptor, "w" );
            if(file != NULL) {
               int dup2Result = _dup2( _fileno( file ), _fileno( stderr ) );
               if(dup2Result == 0) { setvbuf( stderr, NULL, _IONBF, 0 ); }
            }
         }
      }
   }

   // Clear the error state for each of the C++ standard stream objects. We need to do this, as attempts to access the
   // standard streams before they refer to a valid target will cause the iostream objects to enter an error state. In
   // versions of Visual Studio after 2005, this seems to always occur during startup regardless of whether anything
   // has been read from or written to the targets or not.
   if(bindStdIn) {
      std::wcin.clear();
      std::cin.clear();
   }
   if(bindStdOut) {
      std::wcout.clear();
      std::cout.clear();
   }
   if(bindStdErr) {
      std::wcerr.clear();
      std::cerr.clear();
   }
}

co::token<> dependent_task(int majorid, int minorid)
{
   using namespace std::chrono_literals;
   DWORD time = random::Between( 1, 100 );
   Sleep( time );
   printf( "run [dependent_task %d, %d] on co job thread: %u \n", majorid, minorid, co::Scheduler::Get().GetThreadIndex() );

   co_return;
}
//-----------------------------------------------------------------------------------------------
co::deferred_token<> basic_coroutine_task(size_t& result, int id)
{
   {
      printf( "run [basic_coroutine_task %d] on co job thread: %u \n", id, co::Scheduler::Get().GetThreadIndex() );
      auto re = 0;
      for(int i = 0; i < 100; ++i) {
         re += i;
      }
      auto token1 = dependent_task(id, 1);
      auto token2 = dependent_task(id, 2);
      auto token3 = dependent_task(id, 3);
      auto token4 = dependent_task(id, 4);
      auto token5 = dependent_task(id, 5);
      auto token6 = dependent_task(id, 6);

      co_await token1;
      co_await token2;
      co_await token3;
      co_await token4;
      co_await token5;
      co_await token6;

      using namespace std::chrono_literals;

      std::this_thread::sleep_for( 100ms );
   }
   printf( "finish [basic_coroutine_task %d] on co job thread: %u \n", id, co::Scheduler::Get().GetThreadIndex() );
   result++;
}

class GameApplication final: public Application {
public:
   void OnInit() override;
   void OnUpdate() override;
   void OnRender() const override;
protected:
   // co::token<> mFinishToken;
   Tracer mTracer;
};

void GameApplication::OnInit()
{
   auto bigtask = []() -> co::task<size_t>
   {

      size_t result = 0;
      std::vector<co::deferred_token<>> tokens;
      tokens.push_back( basic_coroutine_task(result, 1) );
      tokens.push_back( basic_coroutine_task(result, 2) );
      tokens.push_back( basic_coroutine_task(result, 3) );
      tokens.push_back( basic_coroutine_task(result, 4) );
      tokens.push_back( basic_coroutine_task(result, 5) );
      tokens.push_back( basic_coroutine_task(result, 6) );
      tokens.push_back( basic_coroutine_task(result, 7) );

      co_await co::sequencial_for( tokens );
      co_return result;
   };

   printf( "final result from task: %u", bigtask().Result() );
   mTracer.OnInit();
}

void GameApplication::OnUpdate()
{
   mTracer.OnUpdate();
}

void GameApplication::OnRender() const
{
   mTracer.OnRender();
}



int __stdcall WinMain( HINSTANCE, HINSTANCE, LPSTR /*commandLineString*/, int )
{
   auto id = GetCurrentProcessId();
   AllocConsole();

   GameApplication app;
   // BindCrtHandlesToStdHandles(true, true, true);
   freopen_s( (FILE**)stdout, "CONOUT$", "w", stdout ); //just works

   auto stdinfo =  GetStdHandle(STD_OUTPUT_HANDLE);

   DWORD mode;
   GetConsoleMode( stdinfo, &mode );

   mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
   SetConsoleMode( stdinfo, mode );

   while(app.runFrame());

   FreeConsole();

   profiler::dumpBlocksToFile( "test_profile.prof" );
   return 0;
}
