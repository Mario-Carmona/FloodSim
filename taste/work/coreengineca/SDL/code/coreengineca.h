#ifndef COREENGINECA_PROCESS_INCLUDE_GUARD_H
#define COREENGINECA_PROCESS_INCLUDE_GUARD_H


//// Startup
void coreengineca_startup();


//// Declaration Of Exported Inner Procedures
void coreengineca_PI_BufferFreed();
void coreengineca_PI_StartSimulation(asn1SccSimConfig *in__cfg);
void coreengineca_PI_GISDataLoaded(asn1SccCellularAutomatonState *in__loadedstate);


//// Input Signals

// Provided interface "BufferFreed"
void coreengineca_bufferfreed_transition();

// Provided interface "GISDataLoaded"
void coreengineca_gisdataloaded_transition();

// Provided interface "StartSimulation"
void coreengineca_startsimulation_transition();

//// Output Signals

//// Continuous Signals
void coreengineca_check_queue(bool* has_pending_msg);


//// External Procedures

// Sync Required Interface "LoadGISData
void coreengineca_RI_LoadGISData(asn1SccFolderPath * Gisdatapath);

// Sync Required Interface "SimulationFinished
void coreengineca_RI_SimulationFinished();

// Sync Required Interface "SnapshotAvailable
void coreengineca_RI_SnapshotAvailable(asn1SccCellularAutomatonState * Receivedstate);

// Sync Required Interface "get_sender
void coreengineca_RI_get_sender(asn1SccPID * sender);

// Sync Required Interface "get_last_error
void coreengineca_RI_get_last_error(asn1SccT_Runtime_Error * err);

//// Timers



#endif /* COREENGINECA_PROCESS_INCLUDE_GUARD_H */