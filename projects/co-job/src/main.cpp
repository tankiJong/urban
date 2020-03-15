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
#include "pt/tracer.hpp"

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

//-----------------------------------------------------------------------------------------------

co::token<int> printTask(uint i)
{
   auto& scheduler = co::Scheduler::Get();
   using namespace std::chrono;
   std::this_thread::sleep_for( 1s );
   printf( "task [%u] -- I am running on thread %u\n", i, scheduler.GetThreadIndex() );
   co_return i;
}


void prints()
{
   
   auto& scheduler = co::Scheduler::Get();
   printf( "prints runs on thread %u\n", scheduler.GetThreadIndex() );

   std::vector<co::token<int>> tokens;
   for( int i = 0; i < 100; ++i ) {
      tokens.push_back( printTask(i) );
   }
   for(auto& token: tokens) {
      int xx = cppcoro::sync_wait( token );
      printf( "task %i, thread %u\n", xx, scheduler.GetThreadIndex() );
   }
}

co::token<int> Sum(int* data, uint start, uint end)
{
   EASY_BLOCK( fmt::format( "sum {}-{}", start, end ).c_str() );
   auto& scheduler = co::Scheduler::Get();
   printf( "Sum [%u - %u] runs on thread %u\n", start, end, scheduler.GetThreadIndex() );
   if (end - start <= 10) {
      int total = 0;
      for(uint i = start; i < end; i++) {
         total += data[i];
      }
      using namespace std::chrono;
      std::this_thread::sleep_for( 1s );
      co_return total;
   }

   uint mid = (start + end) >> 1;
   int left = co_await Sum( data, start, mid );
   int right = co_await Sum( data, mid, end );

   printf( "Sum [%u - %u] finished on thread %u\n", start, end, scheduler.GetThreadIndex() );
   co_return left + right;
}

co::token<> ParallelFor()
{
   auto func = []( uint i ) -> co::token<>
   {
      using namespace std::chrono;
      std::this_thread::sleep_for( 5s );
      auto& scheduler = co::Scheduler::Get();
      printf( "task %i, thread %u\n", i, scheduler.GetThreadIndex() );
      co_return;
   };

   std::vector<co::token<>> tokens;
   for(uint i = 0; i < 10; i++) {
      tokens.emplace_back( func( i ) );
   }

   auto& scheduler = co::Scheduler::Get();
   printf( "ParallelFor - after loop, thread %u\n", scheduler.GetThreadIndex() );

   co_await cppcoro::when_all_ready( std::move(tokens) );
   printf( "ParallelFor - after wait, thread %u\n", scheduler.GetThreadIndex() );
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
   mTracer.OnInit();
   // mFinishToken = std::move(ParallelFor());
   // int data[100];
   // for(int i = 0; i < 100; i++) {
   //    data[i] = i;
   // }
   //
   // Sum( data, 0, 100 );
   // Sum( data, 0, 100 );
   // Sum( data, 0, 100 );
   // // printf( "sum is %d", sum );
   // printf( "end init\n" );
}

void GameApplication::OnUpdate()
{
   // static bool complete = false;
   // if(mFinishToken.is_ready() && !complete) {
   //    printf( "Parallel task complete" );
   //    complete = true;
   // }
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
