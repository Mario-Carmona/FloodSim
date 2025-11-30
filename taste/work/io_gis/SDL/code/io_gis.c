//// Includes
#include "dataview-uniq.h"
#include "io_gis_datamodel.h"
#include "io_gis.h"

//// Nested States


//// SDL Constants
static const asn1SccPID self = asn1SccPID_io_gis;


//// Aliases


//// Context
static asn1SccIo_gis_Context ctxt = {0};

#define CS_Only 2
void runTransitionIo_Gis(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures
void io_gis_PI_loadgisdata(asn1SccFilePath *in__folder, asn1SccGrid *grid);


//// Startup
void CInitio_gis()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionIo_Gis(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void io_gis_startup()
{
   CInitio_gis();
}

//// Input Signals
void io_gis_loadgisdata_transition()
{
   switch(ctxt.state)
   {
      case asn1SccIo_gis_States_idle:
      {
         runTransitionIo_Gis(1);
         break;
      }
      default:
      {
         runTransitionIo_Gis(CS_Only);
         break;
      }
   }
}



//// Output Signals

//// Definition Of Inner Procedures
void io_gis_PI_loadgisdata(asn1SccFilePath *in__folder, asn1SccGrid *grid)
{
   asn1SccFilePath folder = *in__folder;
   // get_sender(sender) (1,5)
   io_gis_RI_get_sender(&ctxt.sender);
   // COMMENT Call C++ function to load the grid (24,12)
   // grid = loadGISFolder(folder)
   // grid = addSinkBoundary(grid)
   // io_gis_loadGISData_transition (None,None)
   io_gis_loadgisdata_transition();
   // RETURN  (None,None) at 138361116134912, 225
   return;
}


// Required To Work With TASTE's Wrappers
void io_gis_PI_loadGISData(asn1SccFilePath *in__folder, asn1SccGrid *grid)
{
   io_gis_PI_loadgisdata(in__folder, grid);
}



//// Definition Of Run Transition
void runTransitionIo_Gis(int Id)
{
   int trId = Id;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // NEXT_STATE Idle (37,18) at 138361116135680, 125
            // COMMENT Waiting for LoadGISDataSignal (40,8)
            trId = -1;
            ctxt.state = asn1SccIo_gis_States_idle;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // NEXT_STATE Idle (53,22) at 138361116811584, 235
            trId = -1;
            ctxt.state = asn1SccIo_gis_States_idle;
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
char* io_gis_state(void)
{
     return "Not_supported_in_C__Use_the_Ada_backend";
}