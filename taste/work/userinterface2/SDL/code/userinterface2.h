#ifndef USERINTERFACE_PROCESS_INCLUDE_GUARD_H
#define USERINTERFACE_PROCESS_INCLUDE_GUARD_H


//// Startup
void userinterface_startup();


//// Declaration Of Exported Inner Procedures


//// Input Signals

// Provided interface "updateGridRequest"
void userinterface_updategridrequest_transition();

//// Output Signals

//// Continuous Signals


//// External Procedures

// Sync Required Interface "LoadMapRequest
void userinterface_RI_LoadMapRequest(asn1SccFolderPath * Folder);

// Sync Required Interface "simulationStart
void userinterface_RI_simulationStart();

// Sync Required Interface "get_sender
void userinterface_RI_get_sender(asn1SccPID * sender);

// Sync Required Interface "get_last_error
void userinterface_RI_get_last_error(asn1SccT_Runtime_Error * err);

//// Timers



#endif /* USERINTERFACE_PROCESS_INCLUDE_GUARD_H */