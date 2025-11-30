#ifndef USERINTERFACE_PROCESS_INCLUDE_GUARD_H
#define USERINTERFACE_PROCESS_INCLUDE_GUARD_H


//// Startup
void userinterface_startup();


//// Declaration Of Exported Inner Procedures
void userinterface_PI_SnapshotAvailable(asn1SccCellularAutomatonState *in__receivedstate);
void userinterface_PI_UserPressStart(asn1SccFolderPath *in__datapath, asn1SccT_Float *in__dur, asn1SccT_Float *in__dt, asn1SccT_UInt32 *in__freq, asn1SccSyncMode *in__syncmode);
void userinterface_PI_SimulationFinished();


//// Input Signals

// Provided interface "SimulationFinished"
void userinterface_simulationfinished_transition();

// Provided interface "SnapshotAvailable"
void userinterface_snapshotavailable_transition();

// Provided interface "UserPressStart"
void userinterface_userpressstart_transition();

//// Output Signals

//// Continuous Signals


//// External Procedures

// Sync Required Interface "BufferFreed
void userinterface_RI_BufferFreed();

// Sync Required Interface "StartSimulation
void userinterface_RI_StartSimulation(asn1SccSimConfig * Cfg);

// Sync Required Interface "get_sender
void userinterface_RI_get_sender(asn1SccPID * sender);

// Sync Required Interface "get_last_error
void userinterface_RI_get_last_error(asn1SccT_Runtime_Error * err);

//// Timers



#endif /* USERINTERFACE_PROCESS_INCLUDE_GUARD_H */