//// Includes
#include "dataview-uniq.h"
#include "ui_datamodel.h"
#include "ui.h"

//// Nested States


//// SDL Constants
static const asn1SccPID self = asn1SccPID_ui;


//// Aliases


//// Context
static asn1SccUi_Context ctxt = {0};

#define CS_Only 3
void runTransitionUi(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures
void ui_PI_displayinitialgrid(asn1SccGrid *in__grid);
void ui_PI_displaysnapshot(asn1SccGridModificationList *in__modifications, asn1SccT_Boolean *in__finished);
void _0_ui_askUser(asn1SccFilePath *folder, asn1SccDuration *timestepduration, asn1SccDuration *totalduration, asn1SccTimeStep *snapshotperiod);
void _0_ui_applyModificationsToUIGrid(asn1SccGridModificationList modifications, asn1SccGrid *grid);
void _0_ui_renderGrid(asn1SccGrid grid);


//// Startup
void CInitui()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionUi(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void ui_startup()
{
   CInitui();
}

//// Input Signals
void ui_displayinitialgrid_transition()
{
   switch(ctxt.state)
   {
      case asn1SccUi_States_waitinginitialgrid:
      {
         runTransitionUi(2);
         break;
      }
      default:
      {
         runTransitionUi(CS_Only);
         break;
      }
   }
}
void ui_displaysnapshot_transition()
{
   switch(ctxt.state)
   {
      case asn1SccUi_States_waitingsnapshots:
      {
         runTransitionUi(1);
         break;
      }
      default:
      {
         runTransitionUi(CS_Only);
         break;
      }
   }
}



//// Output Signals

//// Definition Of Inner Procedures
void ui_PI_displayinitialgrid(asn1SccGrid *in__grid)
{
   asn1SccGrid grid = *in__grid;
   // get_sender(sender) (1,5)
   ui_RI_get_sender(&ctxt.sender);
   // uiGrid := grid (30,17)
   ctxt.uigrid = (asn1SccGrid) grid;
   // renderGrid(uiGrid) (33,17)
   _0_ui_renderGrid(ctxt.uigrid);
   // ui_displayInitialGrid_transition (None,None)
   ui_displayinitialgrid_transition();
   // RETURN  (None,None) at 137170139457024, 245
   return;
}


// Required To Work With TASTE's Wrappers
void ui_PI_displayInitialGrid(asn1SccGrid *in__grid)
{
   ui_PI_displayinitialgrid(in__grid);
}

void ui_PI_displaysnapshot(asn1SccGridModificationList *in__modifications, asn1SccT_Boolean *in__finished)
{
   asn1SccGridModificationList modifications = *in__modifications;
   asn1SccT_Boolean finished = *in__finished;
   // get_sender(sender) (1,5)
   ui_RI_get_sender(&ctxt.sender);
   // uiFinished := finished (54,17)
   ctxt.uifinished = (asn1SccT_Boolean) finished;
   // applyModificationsToUIGrid(modifications, uiGrid) (57,17)
   _0_ui_applyModificationsToUIGrid(modifications, &ctxt.uigrid);
   // renderGrid(uiGrid) (60,17)
   _0_ui_renderGrid(ctxt.uigrid);
   // ui_displaySnapshot_transition (None,None)
   ui_displaysnapshot_transition();
   // RETURN  (None,None) at 137170139182848, 298
   return;
}


// Required To Work With TASTE's Wrappers
void ui_PI_displaySnapshot(asn1SccGridModificationList *in__modifications, asn1SccT_Boolean *in__finished)
{
   ui_PI_displaysnapshot(in__modifications, in__finished);
}

void _0_ui_askUser(asn1SccFilePath *folder, asn1SccDuration *timestepduration, asn1SccDuration *totalduration, asn1SccTimeStep *snapshotperiod)
{
   // folder = askForFolder()
   // timeStepDuration = askForTimeStep()
   // totalDuration = askForTotalDuration()
   // snapshotPeriod = askForSnapshotPeriod()
   // RETURN  (None,None) at 137170139232832, 344
   return;
}


void _0_ui_applyModificationsToUIGrid(asn1SccGridModificationList modifications, asn1SccGrid *grid)
{
   // RETURN  (None,None) at 137170119206400, 137
   return;
}


void _0_ui_renderGrid(asn1SccGrid grid)
{
   // RETURN  (None,None) at 137170119250624, 151
   return;
}




//// Definition Of Run Transition
void runTransitionUi(int Id)
{
   int trId = Id;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // askUser(folder, timeStepDuration, totalDuration, snapshotPeriod) (136,13)
            _0_ui_askUser(&ctxt.folder, &ctxt.timestepduration, &ctxt.totalduration, &ctxt.snapshotperiod);
            // startSimulationFromGIS(folder, timeStepDuration, totalDuration, snapshotPeriod) (139,15)
            ui_RI_startSimulationFromGIS(&ctxt.folder, &ctxt.timestepduration, &ctxt.totalduration, &ctxt.snapshotperiod);
            // NEXT_STATE WaitingInitialGrid (142,18) at 137170865315584, 224
            trId = -1;
            ctxt.state = asn1SccUi_States_waitinginitialgrid;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // DECISION uiFinished (-1,-1)
            // ANSWER true (155,17)
            if(((ctxt.uifinished) == true))
            {
               // STOP  (None,None) at 137170139696448, 557
               // ANSWER false (161,17)
            } else if(((ctxt.uifinished) == false))
            {
               // NEXT_STATE WaitingSnapshots (164,30) at 137170139367680, 557
               trId = -1;
               ctxt.state = asn1SccUi_States_waitingsnapshots;
               goto continuous_signals;
            } //last
            break;
         }
         case 2:
         {
            // NEXT_STATE WaitingSnapshots (176,22) at 137170119182016, 334
            trId = -1;
            ctxt.state = asn1SccUi_States_waitingsnapshots;
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
char* ui_state(void)
{
     return "Not_supported_in_C__Use_the_Ada_backend";
}