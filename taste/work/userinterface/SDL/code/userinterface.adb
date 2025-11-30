-- This file was generated automatically by OpenGEODE: DO NOT MODIFY IT !




package body UserInterface is
   procedure userStartsSimulation is
      begin
         null;  --  Empty procedure
      end userStartsSimulation;
      

   procedure userSelectsFolder(folder: in out asn1SccFolderPath) is
      begin
         --  get_sender(sender) (1,5)
         RI_0_get_sender(ctxt.sender);
         --  userSelectsFolder_Transition (None,None)
         userSelectsFolder_Transition;
         --  RETURN  (None,None) at 137485127408960, 147
         return;
      end userSelectsFolder;
      

   procedure updateGridRequest(newGridData: in out asn1SccGridData) is
      begin
         --  get_sender(sender) (1,5)
         RI_0_get_sender(ctxt.sender);
         -- renderGrid(newGridData)
         --  updateGridRequest_Transition (None,None)
         updateGridRequest_Transition;
         --  RETURN  (None,None) at 137485602625408, 202
         return;
      end updateGridRequest;
      

   procedure SimulationCompleted_Transition is
      begin
         case ctxt.state is
            when others =>
               Execute_Transition (CS_Only);
         end case;
      end SimulationCompleted_Transition;
      

   procedure UpdateGridRequest_Transition is
      begin
         case ctxt.state is
            when others =>
               Execute_Transition (CS_Only);
         end case;
      end UpdateGridRequest_Transition;
      

   procedure UserSelectsFolder_Transition is
      begin
         case ctxt.state is
            when asn1Sccinitialscreen =>
               Execute_Transition (2);
            when others =>
               Execute_Transition (CS_Only);
         end case;
      end UserSelectsFolder_Transition;
      

   procedure UserStartsSimulation_Transition is
      begin
         case ctxt.state is
            when asn1Sccmaploaded =>
               Execute_Transition (1);
            when others =>
               Execute_Transition (CS_Only);
         end case;
      end UserStartsSimulation_Transition;
      

   procedure Execute_Transition (Id : Integer) is
      trId : Integer := Id;
      begin
         if not ctxt.Init_Done and trId /= 0 then
            return;
         end if;
         while (trId /= -1) loop
            case trId is
               when 0 =>
                  --  NEXT_STATE InitialScreen (57,18) at 137485602452928, 60
                  trId := -1;
                  ctxt.State := asn1SccInitialScreen;
                  goto Continuous_Signals;
               when 1 =>
                  --  SimulationStart TO CoreEngineCA (75,19)
                  RI_0_SimulationStart(Dest_PID => asn1Scccoreengineca);
                  --  NEXT_STATE DisplayingSimulation (78,22) at 137485128162432, 390
                  trId := -1;
                  ctxt.State := asn1SccDisplayingSimulation;
                  goto Continuous_Signals;
               when 2 =>
                  --  LoadMapRequest(folder) TO DataHandlerGIS (89,19)
                  RI_0_LoadMapRequest(ctxt.folder, Dest_PID => asn1Sccdatahandlergis);
                  --  NEXT_STATE MapLoaded (92,22) at 137485127387328, 225
                  trId := -1;
                  ctxt.State := asn1SccMapLoaded;
                  goto Continuous_Signals;
               when CS_Only =>
                  trId := -1;
                  goto Continuous_Signals;
               when others =>
                  null;
            end case;
            <<Continuous_Signals>>
            <<Next_Transition>>
         end loop;
      end Execute_Transition;
      

   procedure Startup is
      begin
         Execute_Transition (0);
         ctxt.Init_Done := True;
      end Startup;
end UserInterface;