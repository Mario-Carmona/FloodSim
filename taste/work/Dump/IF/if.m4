divert(-1)
/*
*  This m4 file uses the following diverts:
*    1 for overall structure
*    5 for cast functions for GUI parameter subtypes
*    7 for num functions on Enum types
*    10 for signals
*    20 for functions
*/dnl
include(templates.m4)
divert(-1)
define(`m4_coreengineca_loadgisdata',`gisdatahandler_loadgisdata')dnl
define(`m4_coreengineca_LoadGISData_provider',`gisdatahandler')dnl
define(`m4_coreengineca_simulationfinished',`userinterface_simulationfinished')dnl
define(`m4_coreengineca_SimulationFinished_provider',`userinterface')dnl
define(`m4_coreengineca_snapshotavailable',`userinterface_snapshotavailable')dnl
define(`m4_coreengineca_SnapshotAvailable_provider',`userinterface')dnl
define(`m4_gisdatahandler_gisdataloaded',`coreengineca_gisdataloaded')dnl
define(`m4_gisdatahandler_GISDataLoaded_provider',`coreengineca')dnl
define(`m4_userinterface_bufferfreed',`coreengineca_bufferfreed')dnl
define(`m4_userinterface_BufferFreed_provider',`coreengineca')dnl
define(`m4_userinterface_startsimulation',`coreengineca_startsimulation')dnl
define(`m4_userinterface_StartSimulation_provider',`coreengineca')dnl
divert(1)dnl
system taste;
/*
 *
 * Data View
 *
 */
include(dataview.if)

type math = abstract
    integer abs(integer);
    real abs(real);
    integer fix(real);
    real power(real, real);
    integer Shift_Left(integer, integer);
    integer Shift_Right(integer, integer);
    integer ceil(real);
    integer floor(real);
    real float(integer);
    integer round(real);
    real sin(real);
    real cos(real);
    integer trunc(real);
endabstract;

type enum_functions = abstract
undivert(7)
endabstract;


divert(20)
// ERROR: Interface "BufferFreed" in function "CoreEngineCA" has unsupported kind: "ANY_OPERATION"

// ERROR: Interface "GISDataLoaded" in function "CoreEngineCA" has unsupported kind: "ANY_OPERATION"

// ERROR: Interface "StartSimulation" in function "CoreEngineCA" has unsupported kind: "ANY_OPERATION"






include(coreengineca.if)

// ERROR: Interface "LoadGISData" in function "GISDataHandler" has unsupported kind: "ANY_OPERATION"




include(gisdatahandler.if)

// ERROR: Interface "SimulationFinished" in function "UserInterface" has unsupported kind: "ANY_OPERATION"

// ERROR: Interface "SnapshotAvailable" in function "UserInterface" has unsupported kind: "ANY_OPERATION"

// ERROR: Interface "UserPressStart" in function "UserInterface" has unsupported kind: "ANY_OPERATION"





include(userinterface.if)



divert(1)
type assign = abstract
undivert(5)
endabstract;

/*
 *
 * Interface View
 *
 */
signal set_timer(integer);
signal reset_timer();

undivert(10)

undivert(20)

endsystem;

priorityrules
undivert(30)
endpriorityrules;
