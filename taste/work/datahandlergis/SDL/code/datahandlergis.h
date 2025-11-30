#ifndef DATAHANDLERGIS_PROCESS_INCLUDE_GUARD_H
#define DATAHANDLERGIS_PROCESS_INCLUDE_GUARD_H


//// Startup
void datahandlergis_startup();


//// Declaration Of Exported Inner Procedures
void datahandlergis_PI_LoadMapRequest(asn1SccFolderPath *in__folder);


//// Input Signals

// Provided interface "LoadMapRequest"
void datahandlergis_loadmaprequest_transition();

//// Output Signals

//// Continuous Signals


//// External Procedures

// Sync Required Interface "MapData
void datahandlergis_RI_MapData(asn1SccGridData * Griddata);

// Sync Required Interface "get_sender
void datahandlergis_RI_get_sender(asn1SccPID * sender);

// Sync Required Interface "get_last_error
void datahandlergis_RI_get_last_error(asn1SccT_Runtime_Error * err);

//// Timers



#endif /* DATAHANDLERGIS_PROCESS_INCLUDE_GUARD_H */