#ifndef GISDATAHANDLER_PROCESS_INCLUDE_GUARD_H
#define GISDATAHANDLER_PROCESS_INCLUDE_GUARD_H


//// Startup
void gisdatahandler_startup();


//// Declaration Of Exported Inner Procedures
void gisdatahandler_PI_LoadGISData(asn1SccFolderPath *in__gisdatapath);


//// Input Signals

// Provided interface "LoadGISData"
void gisdatahandler_loadgisdata_transition();

//// Output Signals

//// Continuous Signals


//// External Procedures

// Sync Required Interface "GISDataLoaded
void gisdatahandler_RI_GISDataLoaded(asn1SccCellularAutomatonState * Loadedstate);

// Sync Required Interface "get_sender
void gisdatahandler_RI_get_sender(asn1SccPID * sender);

// Sync Required Interface "get_last_error
void gisdatahandler_RI_get_last_error(asn1SccT_Runtime_Error * err);

//// Timers



#endif /* GISDATAHANDLER_PROCESS_INCLUDE_GUARD_H */