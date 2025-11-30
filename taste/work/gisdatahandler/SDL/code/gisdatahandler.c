//// Includes
#include "dataview-uniq.h"
#include "gisdatahandler_datamodel.h"
#include "gisdatahandler.h"

//// Nested States


//// SDL Constants
static const asn1SccPID self = asn1SccPID_gisdatahandler;


//// Aliases


//// Context
static asn1SccGisdatahandler_Context ctxt = {0};

#define CS_Only 2
void runTransitionGisDataHandler(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures
void gisdatahandler_PI_loadgisdata(asn1SccFolderPath *in__gisdatapath);


//// Startup
void CInitgisdatahandler()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionGisDataHandler(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void gisdatahandler_startup()
{
   CInitgisdatahandler();
}

//// Input Signals
void gisdatahandler_loadgisdata_transition()
{
   switch(ctxt.state)
   {
      case asn1SccGisdatahandler_States_idle:
      {
         runTransitionGisDataHandler(1);
         break;
      }
      default:
      {
         runTransitionGisDataHandler(CS_Only);
         break;
      }
   }
}



//// Output Signals

//// Definition Of Inner Procedures
void gisdatahandler_PI_loadgisdata(asn1SccFolderPath *in__gisdatapath)
{
   asn1SccFolderPath gisdatapath = *in__gisdatapath;
   asn1SccFolderPath folderpath;
   asn1SccCellularAutomatonState tempstate;
   asn1SccCellularAutomatonState tmp27;
   // get_sender(sender) (1,5)
   gisdatahandler_RI_get_sender(&ctxt.sender);
   // folderPath := gisDataPath (23,17)
   folderpath = (asn1SccFolderPath) gisdatapath;
   // tempState.rows := getForm(elevation.rdc)
   // tempState.cols := getForm(elevation.rdc)
   // tempState.elevation := readIDRISI(elevation.rst)
   // tempState.waterDepth := readIDRISI(waterInit.rst)
   // GISDataLoaded(tempState) (38,19)
   tmp27 = (asn1SccCellularAutomatonState) tempstate;
   gisdatahandler_RI_GISDataLoaded(&tmp27);
   // gisdatahandler_LoadGISData_transition (None,None)
   gisdatahandler_loadgisdata_transition();
   // RETURN  (None,None) at 131822008404288, 513
   return;
}


// Required To Work With TASTE's Wrappers
void gisdatahandler_PI_LoadGISData(asn1SccFolderPath *in__gisdatapath)
{
   gisdatahandler_PI_loadgisdata(in__gisdatapath);
}



//// Definition Of Run Transition
void runTransitionGisDataHandler(int Id)
{
   int trId = Id;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // NEXT_STATE Idle (48,18) at 131822008196864, 168
            trId = -1;
            ctxt.state = asn1SccGisdatahandler_States_idle;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // NEXT_STATE Idle (58,22) at 131822008239808, 278
            trId = -1;
            ctxt.state = asn1SccGisdatahandler_States_idle;
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
char* gisdatahandler_state(void)
{
     return "Not_supported_in_C__Use_the_Ada_backend";
}