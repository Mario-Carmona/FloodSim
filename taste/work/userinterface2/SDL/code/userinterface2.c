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

#define CS_Only 1
void runTransitionUserinterface(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures


//// Startup
void CInituserinterface()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionUserinterface(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void userinterface_startup()
{
   CInituserinterface();
}

//// Input Signals
void userinterface_updategridrequest_transition()
{
   switch(ctxt.state)
   {
      default:
      {
         runTransitionUserinterface(CS_Only);
         break;
      }
   }
}



//// Output Signals

//// Definition Of Inner Procedures


//// Definition Of Run Transition
void runTransitionUserinterface(int Id)
{
   int trId = Id;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // NEXT_STATE Wait (10,18) at 124040270461120, 60
            trId = -1;
            ctxt.state = asn1SccUserinterface_States_wait;
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