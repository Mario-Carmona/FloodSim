//// Includes
#include "dataview-uniq.h"
#include "userinterface_datamodel.h"
#include "userinterface.h"

//// Nested States


//// SDL Constants
static const asn1SccPID self = asn1SccPID_userinterface;


//// Aliases


//// Context
static asn1SccUserinterface_Context ctxt = {0};

#define CS_Only 4
void runTransitionUserInterface(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures
void userinterface_PI_snapshotavailable(asn1SccCellularAutomatonState *in__receivedstate);
void userinterface_PI_userpressstart(asn1SccFolderPath *in__datapath, asn1SccT_Float *in__dur, asn1SccT_Float *in__dt, asn1SccT_UInt32 *in__freq, asn1SccSyncMode *in__syncmode);
void userinterface_PI_simulationfinished();


//// Startup
void CInituserinterface()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionUserInterface(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void userinterface_startup()
{
   CInituserinterface();
}

//// Input Signals
void userinterface_simulationfinished_transition()
{
   switch(ctxt.state)
   {
      case asn1SccUserinterface_States_renderingloop:
      {
         runTransitionUserInterface(1);
         break;
      }
      default:
      {
         runTransitionUserInterface(CS_Only);
         break;
      }
   }
}
void userinterface_snapshotavailable_transition()
{
   switch(ctxt.state)
   {
      case asn1SccUserinterface_States_renderingloop:
      {
         runTransitionUserInterface(2);
         break;
      }
      default:
      {
         runTransitionUserInterface(CS_Only);
         break;
      }
   }
}
void userinterface_userpressstart_transition()
{
   switch(ctxt.state)
   {
      case asn1SccUserinterface_States_idle:
      {
         runTransitionUserInterface(3);
         break;
      }
      default:
      {
         runTransitionUserInterface(CS_Only);
         break;
      }
   }
}



//// Output Signals

//// Definition Of Inner Procedures
void userinterface_PI_snapshotavailable(asn1SccCellularAutomatonState *in__receivedstate)
{
   asn1SccCellularAutomatonState receivedstate = *in__receivedstate;
   // get_sender(sender) (1,5)
   userinterface_RI_get_sender(&ctxt.sender);
   // currentState := receivedState (27,17)
   ctxt.currentstate = (asn1SccCellularAutomatonState) receivedstate;
   // render(receivedState)
   // DECISION cfg.mode (-1,-1)
   // COMMENT BRIDGE LOGIC (Consumer Side) (36,12)
   // ANSWER losslessAllFrames (39,17)
   if(((ctxt.cfg.mode) == asn1SccSyncMode_losslessAllFrames))
   {
      // BufferFreed (42,27)
      // COMMENT In Lossless mode, we must notify the Core that 
      // we have finished and freed the buffer (45,20)
      userinterface_RI_BufferFreed();
      // ANSWER realTimeSkipFrames (49,17)
   } else if(((ctxt.cfg.mode) == asn1SccSyncMode_realTimeSkipFrames))
   {
   } //last
   // userinterface_SnapshotAvailable_transition (None,None)
   userinterface_snapshotavailable_transition();
   // RETURN  (None,None) at 133637776995072, 429
   return;
}


// Required To Work With TASTE's Wrappers
void userinterface_PI_SnapshotAvailable(asn1SccCellularAutomatonState *in__receivedstate)
{
   userinterface_PI_snapshotavailable(in__receivedstate);
}

void userinterface_PI_userpressstart(asn1SccFolderPath *in__datapath, asn1SccT_Float *in__dur, asn1SccT_Float *in__dt, asn1SccT_UInt32 *in__freq, asn1SccSyncMode *in__syncmode)
{
   asn1SccFolderPath datapath = *in__datapath;
   asn1SccT_Float dur = *in__dur;
   asn1SccT_Float dt = *in__dt;
   asn1SccT_UInt32 freq = *in__freq;
   asn1SccSyncMode syncmode = *in__syncmode;
   // get_sender(sender) (1,5)
   userinterface_RI_get_sender(&ctxt.sender);
   // cfg.gisDataPath := dataPath (78,17)
   ctxt.cfg.gisDataPath = (asn1SccFolderPath) datapath;
   // cfg.totalDuration := dur (81,17)
   ctxt.cfg.totalDuration = (asn1SccT_Float) dur;
   // cfg.timeStep := dt (84,17)
   ctxt.cfg.timeStep = (asn1SccT_Float) dt;
   // cfg.snapshotFreq := freq (87,17)
   ctxt.cfg.snapshotFreq = (asn1SccT_UInt32) freq;
   // cfg.mode := syncMode (90,17)
   ctxt.cfg.mode = (asn1SccSyncMode) syncmode;
   // StartSimulation(cfg) (93,19)
   userinterface_RI_StartSimulation(&ctxt.cfg);
   // userinterface_UserPressStart_transition (None,None)
   userinterface_userpressstart_transition();
   // RETURN  (None,None) at 133637778969536, 510
   return;
}


// Required To Work With TASTE's Wrappers
void userinterface_PI_UserPressStart(asn1SccFolderPath *in__datapath, asn1SccT_Float *in__dur, asn1SccT_Float *in__dt, asn1SccT_UInt32 *in__freq, asn1SccSyncMode *in__syncmode)
{
   userinterface_PI_userpressstart(in__datapath, in__dur, in__dt, in__freq, in__syncmode);
}

void userinterface_PI_simulationfinished()
{
   // get_sender(sender) (1,5)
   userinterface_RI_get_sender(&ctxt.sender);
   // Display: Simulation Complete
   // userinterface_SimulationFinished_transition (None,None)
   userinterface_simulationfinished_transition();
   // RETURN  (None,None) at 133637777051648, 263
   return;
}


// Required To Work With TASTE's Wrappers
void userinterface_PI_SimulationFinished()
{
   userinterface_PI_simulationfinished();
}



//// Definition Of Run Transition
void runTransitionUserInterface(int Id)
{
   int trId = Id;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // NEXT_STATE Idle (117,18) at 133638287490880, 61
            trId = -1;
            ctxt.state = asn1SccUserinterface_States_idle;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // NEXT_STATE Idle (127,22) at 133638287493184, 281
            trId = -1;
            ctxt.state = asn1SccUserinterface_States_idle;
            goto continuous_signals;
            break;
         }
         case 2:
         {
            // NEXT_STATE RenderingLoop (136,22) at 133638287536128, 281
            trId = -1;
            ctxt.state = asn1SccUserinterface_States_renderingloop;
            goto continuous_signals;
            break;
         }
         case 3:
         {
            // NEXT_STATE RenderingLoop (151,22) at 133637776995648, 171
            trId = -1;
            ctxt.state = asn1SccUserinterface_States_renderingloop;
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
char* userinterface_state(void)
{
     return "Not_supported_in_C__Use_the_Ada_backend";
}