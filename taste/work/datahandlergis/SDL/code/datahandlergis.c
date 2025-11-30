//// Includes
#include "dataview-uniq.h"
#include "datahandlergis_datamodel.h"
#include "datahandlergis.h"

//// Nested States


//// SDL Constants
static const asn1SccPID self = asn1SccPID_datahandlergis;


//// Aliases


//// Context
static asn1SccDatahandlergis_Context ctxt = {0};

#define CS_Only 2
void runTransitionDataHandlerGIS(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures
void datahandlergis_PI_loadmaprequest(asn1SccFolderPath *in__folder);


//// Startup
void CInitdatahandlergis()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionDataHandlerGIS(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void datahandlergis_startup()
{
   CInitdatahandlergis();
}

//// Input Signals
void datahandlergis_loadmaprequest_transition()
{
   switch(ctxt.state)
   {
      case asn1SccDatahandlergis_States_waitingforrequest:
      {
         runTransitionDataHandlerGIS(1);
         break;
      }
      default:
      {
         runTransitionDataHandlerGIS(CS_Only);
         break;
      }
   }
}



//// Output Signals

//// Definition Of Inner Procedures
void datahandlergis_PI_loadmaprequest(asn1SccFolderPath *in__folder)
{
   asn1SccFolderPath folder = *in__folder;
   asn1SccGridData griddata;
   asn1SccGridData tmp21;
   // get_sender(sender) (1,5)
   datahandlergis_RI_get_sender(&ctxt.sender);
   // read_IDRISI32_Layers(folder, OUT gridData)
   // MapData(gridData) TO CoreEngineCA (25,19)
   tmp21 = (asn1SccGridData) griddata;
   datahandlergis_RI_MapData(&tmp21);
   // datahandlergis_loadMapRequest_transition (None,None)
   datahandlergis_loadmaprequest_transition();
   // RETURN  (None,None) at 128102890296000, 297
   return;
}


// Required To Work With TASTE's Wrappers
void datahandlergis_PI_LoadMapRequest(asn1SccFolderPath *in__folder)
{
   datahandlergis_PI_loadmaprequest(in__folder);
}



//// Definition Of Run Transition
void runTransitionDataHandlerGIS(int Id)
{
   int trId = Id;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // NEXT_STATE WaitingForRequest (35,18) at 128102381451072, 60
            trId = -1;
            ctxt.state = asn1SccDatahandlergis_States_waitingforrequest;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // NEXT_STATE WaitingForRequest (45,22) at 128102382206016, 170
            trId = -1;
            ctxt.state = asn1SccDatahandlergis_States_waitingforrequest;
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
char* datahandlergis_state(void)
{
     return "Not_supported_in_C__Use_the_Ada_backend";
}