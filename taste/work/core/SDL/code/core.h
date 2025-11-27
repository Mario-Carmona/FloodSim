#ifndef CORE_PROCESS_INCLUDE_GUARD_H
#define CORE_PROCESS_INCLUDE_GUARD_H


//// Startup
void core_startup();


//// Declaration Of Exported Inner Procedures
void core_PI_startSimulationFromGIS(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod);


//// Input Signals

// Provided interface "startSimulationFromGIS"
void core_startsimulationfromgis_transition();

//// Output Signals

//// Continuous Signals
void core_check_queue(bool* has_pending_msg);


//// External Procedures

// Sync Required Interface "displayInitialGrid
void core_RI_displayInitialGrid(asn1SccGrid * Grid);

// Sync Required Interface "displaySnapshot
void core_RI_displaySnapshot(asn1SccGridModificationList * Modifications,asn1SccT_Boolean * Finished);

// Sync Required Interface "loadGISData
void core_RI_loadGISData(asn1SccFilePath * Folder,asn1SccGrid * Grid);

// Sync Required Interface "get_sender
void core_RI_get_sender(asn1SccPID * sender);

// Sync Required Interface "get_last_error
void core_RI_get_last_error(asn1SccT_Runtime_Error * err);

//// Timers



#endif /* CORE_PROCESS_INCLUDE_GUARD_H */