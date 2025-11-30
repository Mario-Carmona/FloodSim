//// Includes
#include "dataview-uniq.h"
#include "ui_manager_datamodel.h"
#include "ui_manager.h"

//// Nested States


//// SDL Constants
static const asn1SccPID self = asn1SccPID_env;


//// Aliases


//// Context
static asn1SccUi_manager_Context ctxt = {0};

#define CS_Only 5
void runTransitionUI_Manager(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures
void _0_ui_manager_renderGrid(asn1SccGridData griddata);


//// Startup
void CInitui_manager()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionUI_Manager(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void ui_manager_startup()
{
   CInitui_manager();
}

//// Input Signals



//// Output Signals

//// Definition Of Inner Procedures
void _0_ui_manager_renderGrid(asn1SccGridData griddata)
{
   // RETURN  (None,None) at 126364832398848, 175
   return;
}




//// Definition Of Run Transition
void runTransitionUI_Manager(int Id)
{
   int trId = Id;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // NEXT_STATE InitialScreen (32,18) at 126365120987712, 60
            trId = -1;
            ctxt.state = asn1SccUi_manager_States_initialscreen;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // renderGrid(newGridData) (42,17)
            _0_ui_manager_renderGrid(ctxt.newgriddata);
            // NEXT_STATE DisplayingSimulation (45,22) at 126365121236864, 555
            trId = -1;
            ctxt.state = asn1SccUi_manager_States_displayingsimulation;
            goto continuous_signals;
            break;
         }
         case 2:
         {
            // NEXT_STATE InitialScreen (51,22) at 126365121781440, 500
            trId = -1;
            ctxt.state = asn1SccUi_manager_States_initialscreen;
            goto continuous_signals;
            break;
         }
         case 3:
         {
            // SimulationStart (62,19)
            // NEXT_STATE DisplayingSimulation (65,22) at 126365121233856, 390
            trId = -1;
            ctxt.state = asn1SccUi_manager_States_displayingsimulation;
            goto continuous_signals;
            break;
         }
         case 4:
         {
            // LoadMapRequest(folder) (76,19)
            // NEXT_STATE MapLoaded (79,22) at 126365121723968, 225
            trId = -1;
            ctxt.state = asn1SccUi_manager_States_maploaded;
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
char* ui_manager_state(void)
{
     return "Not_supported_in_C__Use_the_Ada_backend";
}