#ifndef UI_PROCESS_INCLUDE_GUARD_H
#define UI_PROCESS_INCLUDE_GUARD_H


//// Startup
void ui_startup();


//// Declaration Of Exported Inner Procedures
void ui_PI_displayInitialGrid(asn1SccGrid *in__grid);
void ui_PI_displaySnapshot(asn1SccGridModificationList *in__modifications, asn1SccT_Boolean *in__finished);


//// Input Signals

// Provided interface "displayInitialGrid"
void ui_displayinitialgrid_transition();

// Provided interface "displaySnapshot"
void ui_displaysnapshot_transition();

//// Output Signals

//// Continuous Signals


//// External Procedures

// Sync Required Interface "startSimulationFromGIS
void ui_RI_startSimulationFromGIS(asn1SccFilePath * Folder,asn1SccDuration * Timestepduration,asn1SccDuration * Totalduration,asn1SccTimeStep * Snapshotperiod);

// Sync Required Interface "get_sender
void ui_RI_get_sender(asn1SccPID * sender);

// Sync Required Interface "get_last_error
void ui_RI_get_last_error(asn1SccT_Runtime_Error * err);

//// Timers



#endif /* UI_PROCESS_INCLUDE_GUARD_H */