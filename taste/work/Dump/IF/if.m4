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
define(`m4_core_displayinitialgrid',`ui_displayinitialgrid')dnl
define(`m4_core_displayInitialGrid_provider',`ui')dnl
define(`m4_core_displaysnapshot',`ui_displaysnapshot')dnl
define(`m4_core_displaySnapshot_provider',`ui')dnl
define(`m4_core_loadgisdata',`io_gis_loadgisdata')dnl
define(`m4_core_loadGISData_provider',`io_gis')dnl
define(`m4_ui_startsimulationfromgis',`core_startsimulationfromgis')dnl
define(`m4_ui_startSimulationFromGIS_provider',`core')dnl
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
// ERROR: Interface "startSimulationFromGIS" in function "Core" has unsupported kind: "ANY_OPERATION"






include(core.if)

// ERROR: Interface "loadGISData" in function "IO_GIS" has unsupported kind: "ANY_OPERATION"



include(io_gis.if)

// ERROR: Interface "displayInitialGrid" in function "UI" has unsupported kind: "ANY_OPERATION"

// ERROR: Interface "displaySnapshot" in function "UI" has unsupported kind: "ANY_OPERATION"




include(ui.if)



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
