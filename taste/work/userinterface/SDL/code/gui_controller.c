//// Includes
#include "dataview-uniq.h"
#include "gui_controller_datamodel.h"
#include "gui_controller.h"

//// Nested States


//// SDL Constants
static const asn1SccPID self = asn1SccPID_env;


//// Aliases


//// Context
static asn1SccGui_controller_Context ctxt = {0};

#define CS_Only 1
void runTransitionGUI_Controller(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures
void _0_gui_controller_UserPressStart();


//// Startup
void CInitgui_controller()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionGUI_Controller(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void gui_controller_startup()
{
   CInitgui_controller();
}

//// Input Signals



//// Output Signals

//// Definition Of Inner Procedures
void _0_gui_controller_UserPressStart()
{
   // Empty Procedure
}




//// Definition Of Run Transition
void runTransitionGUI_Controller(int Id)
{
   int trId = Id;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // NEXT_STATE Idle (23,18) at 135260335109376, 60
            trId = -1;
            ctxt.state = asn1SccGui_controller_States_idle;
            goto continuous_signals;
            break;
         }
         case CS_Only:
         {
            trId = -1;
            goto continuous_signals;
         }
         default:
         {
            break;
         }
      }
      continuous_signals:

      next_transition:
      ;
   }
}


//// Current State To String
char* gui_controller_state(void)
{
     return "Not_supported_in_C__Use_the_Ada_backend";
}