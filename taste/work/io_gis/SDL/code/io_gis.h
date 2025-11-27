#ifndef IO_GIS_PROCESS_INCLUDE_GUARD_H
#define IO_GIS_PROCESS_INCLUDE_GUARD_H


//// Startup
void io_gis_startup();


//// Declaration Of Exported Inner Procedures
void io_gis_PI_loadGISData(asn1SccFilePath *in__folder, asn1SccGrid *grid);


//// Input Signals

// Provided interface "loadGISData"
void io_gis_loadgisdata_transition();

//// Output Signals

//// Continuous Signals


//// External Procedures

// Sync Required Interface "get_sender
void io_gis_RI_get_sender(asn1SccPID * sender);

// Sync Required Interface "get_last_error
void io_gis_RI_get_last_error(asn1SccT_Runtime_Error * err);

//// Timers



#endif /* IO_GIS_PROCESS_INCLUDE_GUARD_H */