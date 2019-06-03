#pragma once

#include "engine/pch.h"

class Application {
   enum eRunStatus {
      APP_CREATED,
      APP_RUNNING,
      APP_QUITTING,
   };

public:

   virtual ~Application() = default;

   virtual void OnInit() {}

   virtual void OnStartFrame() {}

   virtual void OnInput() {}

   virtual void OnUpdate() {}
   virtual void PostUpdate() {}

   virtual void OnRender() const {}

   virtual void OnGui() {}
   virtual void OnQuit() {}

   virtual void OnEndFrame() {}

   virtual void OnDestroy() {}

   bool runFrame();
protected:

private:
   void _init();
   void _destroy();
   void _update();
   void _input();

   void __stdcall windowProc( uint wmMessageCode, size_t wParam, size_t lParam );
   eRunStatus     mRunStatus = APP_CREATED;
};
