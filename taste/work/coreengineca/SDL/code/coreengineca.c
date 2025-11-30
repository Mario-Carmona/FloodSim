//// Includes
#include "dataview-uniq.h"
#include "coreengineca_datamodel.h"
#include "coreengineca.h"

//// Nested States


//// SDL Constants
static const asn1SccPID self = asn1SccPID_coreengineca;


//// Aliases


//// Context
static asn1SccCoreengineca_Context ctxt = {0};

#define CS_Only 6
void runTransitionCoreEngineCA(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures
void _0_coreengineca_SendSnapshot();
void coreengineca_PI_bufferfreed();
void coreengineca_PI_startsimulation(asn1SccSimConfig *in__cfg);
void coreengineca_PI_gisdataloaded(asn1SccCellularAutomatonState *in__loadedstate);


//// Startup
void CInitcoreengineca()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionCoreEngineCA(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void coreengineca_startup()
{
   CInitcoreengineca();
}

//// Input Signals
void coreengineca_bufferfreed_transition()
{
   switch(ctxt.state)
   {
      case asn1SccCoreengineca_States_blockedbyui:
      {
         runTransitionCoreEngineCA(3);
         break;
      }
      case asn1SccCoreengineca_States_finalizingwait:
      {
         runTransitionCoreEngineCA(1);
         break;
      }
      default:
      {
         runTransitionCoreEngineCA(CS_Only);
         break;
      }
   }
}
void coreengineca_gisdataloaded_transition()
{
   switch(ctxt.state)
   {
      case asn1SccCoreengineca_States_loadingdata:
      {
         runTransitionCoreEngineCA(4);
         break;
      }
      default:
      {
         runTransitionCoreEngineCA(CS_Only);
         break;
      }
   }
}
void coreengineca_startsimulation_transition()
{
   switch(ctxt.state)
   {
      case asn1SccCoreengineca_States_waitingconfig:
      {
         runTransitionCoreEngineCA(5);
         break;
      }
      default:
      {
         runTransitionCoreEngineCA(CS_Only);
         break;
      }
   }
}



//// Output Signals

//// Definition Of Inner Procedures
void _0_coreengineca_SendSnapshot()
{
   // COMMENT 1. Prepare a copy (32,12)
   // snapshotCopy := internalState (29,17)
   ctxt.snapshotcopy = (asn1SccCellularAutomatonState) ctxt.internalstate;
   // snapshotCopy.timestamp := simTime (35,17)
   ctxt.snapshotcopy.timestamp = (asn1SccT_Float) ctxt.simtime;
   // snapshotCopy.stepIndex := stepCount (38,17)
   ctxt.snapshotcopy.stepIndex = (asn1SccT_UInt32) ctxt.stepcount;
   // SnapshotAvailable(snapshotCopy) (41,19)
   // COMMENT 2. Send (44,12)
   coreengineca_RI_SnapshotAvailable(&ctxt.snapshotcopy);
   // RETURN  (None,None) at 138587655438464, 394
   return;
}


void coreengineca_PI_bufferfreed()
{
   // get_sender(sender) (1,5)
   coreengineca_RI_get_sender(&ctxt.sender);
   // coreengineca_BufferFreed_transition (None,None)
   coreengineca_bufferfreed_transition();
   // RETURN  (None,None) at 138587763925760, 223
   return;
}


// Required To Work With TASTE's Wrappers
void coreengineca_PI_BufferFreed()
{
   coreengineca_PI_bufferfreed();
}

void coreengineca_PI_startsimulation(asn1SccSimConfig *in__cfg)
{
   asn1SccSimConfig cfg = *in__cfg;
   asn1SccFolderPath tmp9791;
   // get_sender(sender) (1,5)
   coreengineca_RI_get_sender(&ctxt.sender);
   // config := cfg (78,17)
   ctxt.config = (asn1SccSimConfig) cfg;
   // LoadGISData(config.gisDataPath) (81,19)
   tmp9791 = (asn1SccFolderPath) ctxt.config.gisDataPath;
   coreengineca_RI_LoadGISData(&tmp9791);
   // coreengineca_StartSimulation_transition (None,None)
   coreengineca_startsimulation_transition();
   // RETURN  (None,None) at 138587723692288, 239
   return;
}


// Required To Work With TASTE's Wrappers
void coreengineca_PI_StartSimulation(asn1SccSimConfig *in__cfg)
{
   coreengineca_PI_startsimulation(in__cfg);
}

void coreengineca_PI_gisdataloaded(asn1SccCellularAutomatonState *in__loadedstate)
{
   asn1SccCellularAutomatonState loadedstate = *in__loadedstate;
   // get_sender(sender) (1,5)
   coreengineca_RI_get_sender(&ctxt.sender);
   // internalState := loadedState (101,17)
   ctxt.internalstate = (asn1SccCellularAutomatonState) loadedstate;
   // Initialize TensorFlow Graph
   // simTime := 0.0 (107,17)
   ctxt.simtime = (asn1SccT_Float) 0.0;
   // stepCount := 0 (110,17)
   ctxt.stepcount = (asn1SccT_UInt32) 0;
   // stepToNextSnapshot := config.snapshotFreq (113,17)
   ctxt.steptonextsnapshot = (asn1SccT_UInt32) ctxt.config.snapshotFreq;
   // SendSnapshot (116,17)
   // COMMENT Send initial snapshot (t=0) (119,12)
   _0_coreengineca_SendSnapshot();
   // coreengineca_GISDataLoaded_transition (None,None)
   coreengineca_gisdataloaded_transition();
   // RETURN  (None,None) at 138587655009600, 524
   return;
}


// Required To Work With TASTE's Wrappers
void coreengineca_PI_GISDataLoaded(asn1SccCellularAutomatonState *in__loadedstate)
{
   coreengineca_PI_gisdataloaded(in__loadedstate);
}



//// Definition Of Run Transition
void runTransitionCoreEngineCA(int Id)
{
   int trId = Id;
   flag message_pending = true;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // NEXT_STATE WaitingConfig (129,18) at 138588214687040, 180
            trId = -1;
            ctxt.state = asn1SccCoreengineca_States_waitingconfig;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // SimulationFinished (139,19)
            coreengineca_RI_SimulationFinished();
            // NEXT_STATE WaitingConfig (142,22) at 138587402093120, 1064
            trId = -1;
            ctxt.state = asn1SccCoreengineca_States_waitingconfig;
            goto continuous_signals;
            break;
         }
         case 2:
         {
            // DECISION simTime < config.totalDuration (157,29)
            // ANSWER false (160,17)
            if((((ctxt.simtime < ctxt.config.totalDuration)) == false))
            {
               // SendSnapshot (163,25)
               // COMMENT Send final snapshot (166,20)
               _0_coreengineca_SendSnapshot();
               // DECISION config.mode (-1,-1)
               // ANSWER realTimeSkipFrames (172,25)
               if(((ctxt.config.mode) == asn1SccSyncMode_realTimeSkipFrames))
               {
                  // SimulationFinished (175,35)
                  coreengineca_RI_SimulationFinished();
                  // NEXT_STATE WaitingConfig (178,38) at 138587407966400, 959
                  trId = -1;
                  ctxt.state = asn1SccCoreengineca_States_waitingconfig;
                  goto continuous_signals;
                  // ANSWER losslessAllFrames (181,25)
               } else if(((ctxt.config.mode) == asn1SccSyncMode_losslessAllFrames))
               {
                  // NEXT_STATE FinalizingWait (184,38) at 138587402603840, 904
                  trId = -1;
                  ctxt.state = asn1SccCoreengineca_States_finalizingwait;
                  goto continuous_signals;
               } //last
               // ANSWER true (188,17)
            } else if((((ctxt.simtime < ctxt.config.totalDuration)) == true))
            {
               // COMMENT 1. Physical Calculation (TensorFlow) (194,20)
               // TF: Run Step (Moore + Torricelli)
               // Update internalState
               // stepCount := stepCount + 1 (200,25)
               ctxt.stepcount = (asn1SccT_UInt32) (ctxt.stepcount + 1);
               // simTime := simTime + config.timeStep (203,25)
               ctxt.simtime = (asn1SccT_Float) (ctxt.simtime + ctxt.config.timeStep);
               // DECISION stepCount = stepToNextSnapshot (206,39)
               // COMMENT 2. Check Snapshot (209,20)
               // ANSWER false (212,25)
               if((((ctxt.stepcount == ctxt.steptonextsnapshot)) == false))
               {
                  // NEXT_STATE Evolving (215,38) at 138587402674432, 1064
                  trId = -1;
                  ctxt.state = asn1SccCoreengineca_States_evolving;
                  goto continuous_signals;
                  // ANSWER true (218,25)
               } else if((((ctxt.stepcount == ctxt.steptonextsnapshot)) == true))
               {
                  // SendSnapshot (221,33)
                  _0_coreengineca_SendSnapshot();
                  // stepToNextSnapshot := stepToNextSnapshot + config.snapshotFreq (224,33)
                  ctxt.steptonextsnapshot = (asn1SccT_UInt32) (ctxt.steptonextsnapshot + ctxt.config.snapshotFreq);
                  // DECISION config.mode (-1,-1)
                  // ANSWER realTimeSkipFrames (230,33)
                  if(((ctxt.config.mode) == asn1SccSyncMode_realTimeSkipFrames))
                  {
                     // NEXT_STATE Evolving (233,46) at 138587393319424, 1282
                     trId = -1;
                     ctxt.state = asn1SccCoreengineca_States_evolving;
                     goto continuous_signals;
                     // ANSWER losslessAllFrames (236,33)
                  } else if(((ctxt.config.mode) == asn1SccSyncMode_losslessAllFrames))
                  {
                     // NEXT_STATE BlockedByUI (239,46) at 138587394036736, 1282
                     trId = -1;
                     ctxt.state = asn1SccCoreengineca_States_blockedbyui;
                     goto continuous_signals;
                  } //last
               } //last
            } //last
            break;
         }
         case 3:
         {
            // NEXT_STATE Evolving (262,22) at 138587393707840, 623
            trId = -1;
            ctxt.state = asn1SccCoreengineca_States_evolving;
            goto continuous_signals;
            break;
         }
         case 4:
         {
            // DECISION config.mode (-1,-1)
            // ANSWER losslessAllFrames (276,17)
            if(((ctxt.config.mode) == asn1SccSyncMode_losslessAllFrames))
            {
               // NEXT_STATE BlockedByUI (279,30) at 138587401312000, 513
               // COMMENT In Lossless mode, we must wait for 
               // the UI to render the initial state
               // 
               // Note: The BlockedByUI state is designed 
               // to return to Evolving upon exit
               // 
               // Represents: cv_producer.wait() in C++ (282,20)
               trId = -1;
               ctxt.state = asn1SccCoreengineca_States_blockedbyui;
               goto continuous_signals;
               // ANSWER realTimeSkipFrames (291,17)
            } else if(((ctxt.config.mode) == asn1SccSyncMode_realTimeSkipFrames))
            {
               // NEXT_STATE Evolving (294,30) at 138587401230144, 513
               // COMMENT In RealTime mode, we are moving 
               // directly toward evolution (297,20)
               trId = -1;
               ctxt.state = asn1SccCoreengineca_States_evolving;
               goto continuous_signals;
            } //last
            break;
         }
         case 5:
         {
            // NEXT_STATE LoadingData (310,22) at 138587724061952, 290
            trId = -1;
            ctxt.state = asn1SccCoreengineca_States_loadingdata;
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

      // Process Continuous Signals
      if(ctxt.init_done)
      {
         coreengineca_check_queue(&message_pending);
      }

      if(message_pending || trId != -1)
      {
         goto next_transition;
      }

      if(ctxt.state == asn1SccCoreengineca_States_evolving)
      {
         // Priority: 1
         // DECISION true (-1,-1)
         // ANSWER true (None,None)
         if(((true) == true))
         {
            trId = 2;
         } // inner if
      } // current state
      next_transition:
      ;
   }
}


//// Current State To String
char* coreengineca_state(void)
{
     return "Not_supported_in_C__Use_the_Ada_backend";
}