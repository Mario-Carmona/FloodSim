-- This file was generated automatically by OpenGEODE: DO NOT MODIFY IT !

with Interfaces,
     Interfaces.C.Strings,
     Ada.Characters.Handling;

use Interfaces,
    Interfaces.C.Strings,
    Ada.Characters.Handling;

with DANASIM_DATAVIEW;
use DANASIM_DATAVIEW;
with TASTE_BasicTypes;
use TASTE_BasicTypes;
with System_Dataview;
use System_Dataview;
with adaasn1rtl;
use adaasn1rtl;
with UserInterface_Datamodel; use UserInterface_Datamodel;

with UserInterface_RI;
package UserInterface with Elaborate_Body is
   
   self : constant asn1SccPID := asn1Sccuserinterface;
   Default_Context: constant asn1SccUserinterface_Context :=
      (Init_Done => False,
       sender => asn1Sccenv,
       offspring => asn1Sccenv,
       others => <>);
   ctxt : aliased asn1SccUserinterface_Context := Default_Context;
   function To_T_Runtime_Error_Selection (Src : TASTE_BasicTypes.asn1SccT_Runtime_Error_Selection) return UserInterface_Datamodel.asn1SccUserInterface_T_Runtime_Error_Selection is (UserInterface_Datamodel.asn1SccUserInterface_T_Runtime_Error_Selection'Enum_Val (Src'Enum_Rep));
   function Get_State return Chars_Ptr is (Userinterface_RI.To_C_Pointer (asn1SccUserInterface_States'Image (ctxt.State))) with Export, Convention => C, Link_Name => "userinterface_state";
   procedure Startup with Export, Convention => C, Link_Name => "userinterface_startup";
   procedure userStartsSimulation;
   pragma Export (C, userStartsSimulation, "userinterface_PI_UserStartsSimulation");
   procedure userSelectsFolder(folder: in out asn1SccFolderPath);
   pragma Export (C, userSelectsFolder, "userinterface_PI_UserSelectsFolder");
   procedure updateGridRequest(newGridData: in out asn1SccGridData);
   pragma Export (C, updateGridRequest, "userinterface_PI_UpdateGridRequest");
   --  Provided interface "SimulationCompleted"
   procedure SimulationCompleted_Transition;
   --  Provided interface "UpdateGridRequest"
   procedure UpdateGridRequest_Transition;
   --  Provided interface "UserSelectsFolder"
   procedure UserSelectsFolder_Transition;
   --  Provided interface "UserStartsSimulation"
   procedure UserStartsSimulation_Transition;
   --  Synchronous Required Interface "LoadMapRequest"
   procedure RI_0_LoadMapRequest (Folder : in out asn1SccFolderPath; Dest_PID : asn1SccPID := asn1SccEnv) renames UserInterface_RI.LoadMapRequest;
   --  Synchronous Required Interface "SimulationStart"
   procedure RI_0_SimulationStart (Dest_PID : asn1SccPID := asn1SccEnv) renames UserInterface_RI.SimulationStart;
   --  Synchronous Required Interface "get_sender"
   procedure RI_0_get_sender (sender : out asn1SccPID; Dest_PID : asn1SccPID := asn1SccEnv) renames UserInterface_RI.get_sender;
   --  Synchronous Required Interface "get_last_error"
   procedure RI_0_get_last_error (err : out asn1SccT_Runtime_Error; Dest_PID : asn1SccPID := asn1SccEnv) renames UserInterface_RI.get_last_error;
   procedure Execute_Transition (Id : Integer);
   CS_Only : constant := 3;
end UserInterface;