//// Includes
#include <math.h>
#include "dataview-uniq.h"
#include "core_datamodel.h"
#include "core.h"

//// Nested States


//// SDL Constants
static const asn1SccPID self = asn1SccPID_core;


//// Aliases


//// Context
static asn1SccCore_Context ctxt = {0};

#define CS_Only 3
void runTransitionCore(int Id);

//// State Aggregations Start functions
//// Declaration Of Inner Procedures
void _0_core_getFlowCoefficient(asn1SccSurfaceType surfacetype, asn1SccCoefficient *flowcoefficient);
void _0_core_updateCell(asn1SccCellIndex cellindex, asn1SccGridModificationList *modlist, asn1SccWaterIncrementEventList *nexteventlist);
void _0_core_calculateCellMooreNeighbors(asn1SccCellIndex index, asn1SccMooreNeighborList *neighbors);
void core_PI_startsimulationfromgis(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod);
void _0_core_runSimulationStep();
void _0_core_applyModifications(asn1SccGridModificationList modifications);
void _0_core_addInitialEvents();
void _0_core_calculateGridCellNeighbors();
void _0_core_computeWaterFlow(asn1SccCellIndex selfcell, asn1SccCellIndex neighborcell, asn1SccOutflow *outflow);
void _0_core_normalizeOutflows(asn1SccT_UReal32 cellwatervolume, asn1SccT_UReal32 totaloutflowvolume, asn1SccOutflowList *celloutflowlist, asn1SccWaterVolume *normalizetotaloutflowvolume);


//// Startup
void CInitcore()
{
   ctxt.sender = asn1SccPID_env;
   ctxt.offspring = asn1SccPID_env;
   

   runTransitionCore(0);
   ctxt.init_done = true;
}

// Required To Work With TASTE's Wrappers
void core_startup()
{
   CInitcore();
}

//// Input Signals
void core_startsimulationfromgis_transition()
{
   switch(ctxt.state)
   {
      case asn1SccCore_States_idle:
      {
         runTransitionCore(2);
         break;
      }
      default:
      {
         runTransitionCore(CS_Only);
         break;
      }
   }
}



//// Output Signals

//// Definition Of Inner Procedures
void _0_core_getFlowCoefficient(asn1SccSurfaceType surfacetype, asn1SccCoefficient *flowcoefficient)
{
   // DECISION surfaceType (-1,-1)
   // ANSWER rock (43,17)
   if(((surfacetype) == asn1SccSurfaceType_rock))
   {
      // flowCoefficient := 0.2 (46,25)
      (*flowcoefficient) = (asn1SccCoefficient) 0.2;
      // ANSWER asphalt (49,17)
   } else if(((surfacetype) == asn1SccSurfaceType_asphalt))
   {
      // flowCoefficient := 0.2 (52,25)
      (*flowcoefficient) = (asn1SccCoefficient) 0.2;
      // ANSWER forest (55,17)
   } else if(((surfacetype) == asn1SccSurfaceType_forest))
   {
      // flowCoefficient := 0.2 (58,25)
      (*flowcoefficient) = (asn1SccCoefficient) 0.2;
      // ANSWER waterSurface (61,17)
   } else if(((surfacetype) == asn1SccSurfaceType_waterSurface))
   {
      // flowCoefficient := 0.2 (64,25)
      (*flowcoefficient) = (asn1SccCoefficient) 0.2;
      // ANSWER soil (67,17)
   } else if(((surfacetype) == asn1SccSurfaceType_soil))
   {
      // flowCoefficient := 0.2 (70,25)
      (*flowcoefficient) = (asn1SccCoefficient) 0.2;
   } //last
   // RETURN  (None,None) at 126879243134208, 303
   return;
}


void _0_core_updateCell(asn1SccCellIndex cellindex, asn1SccGridModificationList *modlist, asn1SccWaterIncrementEventList *nexteventlist)
{
   asn1SccMooreNeighborList neighbors;
   asn1SccCellState cell;
   asn1SccCellIndex neighbor;
   asn1SccMooreNeighborIndex neighborindex;
   asn1SccOutflow outflow;
   asn1SccOutflowList celloutflowlist;
   asn1SccTotalOutflow totaloutflowvolume;
   asn1SccWaterIncrementEvent newevent;
   asn1SccMooreNeighborIndex outflowindex;
   asn1SccGridModification modification;
   asn1SccWaterVolume normalizetotaloutflowvolume;
   asn1SccUint memcpy_counter_5 = 0;
   asn1SccWaterIncrementEventList memcpy_temp_4;
   asn1SccGridModificationList right_temp_5;
   asn1SccWaterIncrementEventList right_temp_7;
   asn1SccUint memcpy_counter_6 = 0;
   asn1SccOutflowList right_temp_6;
   asn1SccGridModificationList memcpy_temp_8;
   asn1SccOutflowList memcpy_temp_6;
   asn1SccGridModificationList memcpy_temp_5;
   asn1SccWaterIncrementEventList memcpy_temp_7;
   asn1SccGridModificationList right_temp_8;
   asn1SccUint memcpy_counter_8 = 0;
   asn1SccWaterIncrementEventList right_temp_4;
   asn1SccUint memcpy_counter_7 = 0;
   asn1SccUint memcpy_counter_4 = 0;
   // neighbors := gridNeighbors(cellIndex) (114,17)
   neighbors = (asn1SccMooreNeighborList) ctxt.gridneighbors.arr[cellindex];
   // neighborIndex := 0 (117,17)
   neighborindex = (asn1SccMooreNeighborIndex) 0;
   // cellOutFlowList := {} (120,17)
   celloutflowlist = (asn1SccOutflowList) asn1SccOutflowList_constant;
   // totalOutflowVolume := 0.0 (123,17)
   totaloutflowvolume = (asn1SccTotalOutflow) 0.0;
   // JOIN loopComputeWaterFlow (None,None) at None, None
   goto loopcomputewaterflow;
   // CONNECTION loopAddGridModifications (190,28)
   loopaddgridmodifications:
   // newEvent := {targetCell cellOutflowList(outflowIndex).neighbor} (193,33)
   newevent = (asn1SccWaterIncrementEvent) {.targetCell = celloutflowlist.arr[outflowindex].neighbor};
   // nextEventList := nextEventList // {newEvent} (196,33)
   {
      right_temp_4 =(asn1SccWaterIncrementEventList) {1, {newevent}};
      memcpy_temp_4 = (*nexteventlist);
      for(memcpy_counter_4 = 0; memcpy_counter_4 < 1; memcpy_counter_4++)
      {
         memcpy_temp_4.arr[memcpy_temp_4.nCount + memcpy_counter_4] = right_temp_4.arr[memcpy_counter_4];
      }
      memcpy_temp_4.nCount += 1;
   }
   (*nexteventlist) = (asn1SccWaterIncrementEventList) memcpy_temp_4;
   // modification := {
   //   targetCell cellOutflowList(outflowIndex).neighbor, 
   //   delta cellOutflowList(outflowIndex).volume
   // } (199,33)
   modification = (asn1SccGridModification) {.targetCell = celloutflowlist.arr[outflowindex].neighbor, .delta = celloutflowlist.arr[outflowindex].volume};
   // modList := modList // {modification} (205,33)
   {
      right_temp_5 =(asn1SccGridModificationList) {1, {modification}};
      memcpy_temp_5 = (*modlist);
      for(memcpy_counter_5 = 0; memcpy_counter_5 < 1; memcpy_counter_5++)
      {
         memcpy_temp_5.arr[memcpy_temp_5.nCount + memcpy_counter_5] = right_temp_5.arr[memcpy_counter_5];
      }
      memcpy_temp_5.nCount += 1;
   }
   (*modlist) = (asn1SccGridModificationList) memcpy_temp_5;
   // outflowIndex := outflowIndex + 1 (208,33)
   outflowindex = (asn1SccMooreNeighborIndex) (outflowindex + 1);
   // DECISION outflowIndex < length (cellOutflowList) (211,50)
   // ANSWER false (214,33)
   if((((outflowindex < celloutflowlist.nCount)) == false))
   {
      // RETURN  (None,None) at 126879241417728, 1862
      return;
      // ANSWER true (220,33)
   } else if((((outflowindex < celloutflowlist.nCount)) == true))
   {
      // JOIN loopAddGridModifications (223,41) at 126879241572032, 1862
      goto loopaddgridmodifications;
   } //last
   // CONNECTION loopComputeWaterFlow (126,12)
   loopcomputewaterflow:
   // computeWaterFlow(cellIndex, neighbors(neighborIndex), outflow) (129,17)
   _0_core_computeWaterFlow(cellindex, neighbors.arr[neighborindex], &outflow);
   // DECISION outflow.volume (-1,-1)
   // ANSWER >0.0 (135,17)
   if(((outflow.volume) > 0.0))
   {
      // cellOutflowList := cellOutflowList // {outflow} (138,25)
      {
         right_temp_6 =(asn1SccOutflowList) {1, {outflow}};
         memcpy_temp_6 = celloutflowlist;
         for(memcpy_counter_6 = 0; memcpy_counter_6 < 1; memcpy_counter_6++)
         {
            memcpy_temp_6.arr[memcpy_temp_6.nCount + memcpy_counter_6] = right_temp_6.arr[memcpy_counter_6];
         }
         memcpy_temp_6.nCount += 1;
      }
      celloutflowlist = (asn1SccOutflowList) memcpy_temp_6;
      // totalOutflowVolume := totalOutflowVolume + outflow.volume (141,25)
      totaloutflowvolume = (asn1SccTotalOutflow) (totaloutflowvolume + outflow.volume);
      // ANSWER ELSE (None,None)
   } // 1
   else
   {
      // nothing done
   } //last
   // neighborIndex := neighborIndex + 1 (148,17)
   neighborindex = (asn1SccMooreNeighborIndex) (neighborindex + 1);
   // DECISION neighborIndex < length (neighbors) (151,35)
   // ANSWER true (154,17)
   if((((neighborindex < neighbors.nCount)) == true))
   {
      // JOIN loopComputeWaterFlow (157,25) at 126879241187904, 930
      goto loopcomputewaterflow;
      // ANSWER false (160,17)
   } else if((((neighborindex < neighbors.nCount)) == false))
   {
      // DECISION length (cellOutflowList) (-1,-1)
      // ANSWER >0 (166,25)
      if(((celloutflowlist.nCount) > 0))
      {
         // normalizeOutflows(coreGrid.cells(cellIndex).waterVolume, totalOutflowVolume, cellOutflowList, normalizeTotalOutflowVolume) (169,33)
         _0_core_normalizeOutflows(ctxt.coregrid.cells.arr[cellindex].waterVolume, totaloutflowvolume, &celloutflowlist, &normalizetotaloutflowvolume);
         // newEvent := {targetCell cellIndex} (172,33)
         newevent = (asn1SccWaterIncrementEvent) {.targetCell = cellindex};
         // nextEventList := nextEventList // {newEvent} (175,33)
         {
            right_temp_7 =(asn1SccWaterIncrementEventList) {1, {newevent}};
            memcpy_temp_7 = (*nexteventlist);
            for(memcpy_counter_7 = 0; memcpy_counter_7 < 1; memcpy_counter_7++)
            {
               memcpy_temp_7.arr[memcpy_temp_7.nCount + memcpy_counter_7] = right_temp_7.arr[memcpy_counter_7];
            }
            memcpy_temp_7.nCount += 1;
         }
         (*nexteventlist) = (asn1SccWaterIncrementEventList) memcpy_temp_7;
         // modification := {
         //   targetCell cellIndex, 
         //   delta -normalizeTotalOutflowVolume
         // } (178,33)
         modification = (asn1SccGridModification) {.targetCell = cellindex, .delta = (-normalizetotaloutflowvolume)};
         // modList := modList // {modification} (184,33)
         {
            right_temp_8 =(asn1SccGridModificationList) {1, {modification}};
            memcpy_temp_8 = (*modlist);
            for(memcpy_counter_8 = 0; memcpy_counter_8 < 1; memcpy_counter_8++)
            {
               memcpy_temp_8.arr[memcpy_temp_8.nCount + memcpy_counter_8] = right_temp_8.arr[memcpy_counter_8];
            }
            memcpy_temp_8.nCount += 1;
         }
         (*modlist) = (asn1SccGridModificationList) memcpy_temp_8;
         // outflowIndex := 0 (187,33)
         outflowindex = (asn1SccMooreNeighborIndex) 0;
         // JOIN loopAddGridModifications (None,None) at None, None
         goto loopaddgridmodifications;
         // ANSWER ELSE (None,None)
      } // 1
      else
      {
         // RETURN  (None,None) at 126879241612992, 1043
         return;
      } //last
   } //last
}


void _0_core_calculateCellMooreNeighbors(asn1SccCellIndex index, asn1SccMooreNeighborList *neighbors)
{
   // RETURN  (None,None) at 126879242705792, 188
   return;
}


void core_PI_startsimulationfromgis(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod)
{
   asn1SccFilePath folder = *in__folder;
   asn1SccDuration timestepduration = *in__timestepduration;
   asn1SccDuration totalduration = *in__totalduration;
   asn1SccTimeStep snapshotperiod = *in__snapshotperiod;
   asn1SccFilePath tmp1787;
   // get_sender(sender) (1,5)
   core_RI_get_sender(&ctxt.sender);
   // currentTimeStep := 0 (270,17)
   ctxt.currenttimestep = (asn1SccTimeStep) 0;
   // totalTimeSteps := fix (round (totalDuration / timeStepDuration)) (273,17)
   ctxt.totaltimesteps = (asn1SccTimeStep) (asn1SccUint)(roundf((totalduration / timestepduration)));
   // coreSnapshotPeriod := snapshotPeriod (276,17)
   ctxt.coresnapshotperiod = (asn1SccTimeStep) snapshotperiod;
   // nextSnapshotTimeStep := coreSnapshotPeriod (279,17)
   ctxt.nextsnapshottimestep = (asn1SccTimeStep) ctxt.coresnapshotperiod;
   // eventList := {} (282,17)
   ctxt.eventlist = (asn1SccWaterIncrementEventList) asn1SccWaterIncrementEventList_constant;
   // accumModList := {} (285,17)
   ctxt.accummodlist = (asn1SccGridModificationList) asn1SccGridModificationList_constant;
   // loadGISData(folder, coreGrid) (288,17)
   tmp1787 = (asn1SccFilePath) folder;
   core_RI_loadGISData(&tmp1787, &ctxt.coregrid);
   // displayInitialGrid(coreGrid) (291,19)
   core_RI_displayInitialGrid(&ctxt.coregrid);
   // addInitialEvents (294,17)
   _0_core_addInitialEvents();
   // calculateGridCellNeighbors (297,17)
   _0_core_calculateGridCellNeighbors();
   // core_startSimulationFromGIS_transition (None,None)
   core_startsimulationfromgis_transition();
   // RETURN  (None,None) at 126879241742784, 648
   return;
}


// Required To Work With TASTE's Wrappers
void core_PI_startSimulationFromGIS(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod)
{
   core_PI_startsimulationfromgis(in__folder, in__timestepduration, in__totalduration, in__snapshotperiod);
}

void _0_core_runSimulationStep()
{
   asn1SccWaterIncrementEventList nexteventlist;
   asn1SccGridModificationList modlist;
   asn1SccCellIndex index;
   asn1SccWaterIncrementEvent event;
   asn1SccGridModificationList memcpy_temp_9;
   int memcpy_counter_9 = 0;
   // modList := {} (320,17)
   modlist = (asn1SccGridModificationList) asn1SccGridModificationList_constant;
   // nextEventList := {} (323,17)
   nexteventlist = (asn1SccWaterIncrementEventList) asn1SccWaterIncrementEventList_constant;
   // index := 0 (326,17)
   index = (asn1SccCellIndex) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (329,12)
   loop:
   // event := eventList(index) (332,17)
   event = (asn1SccWaterIncrementEvent) ctxt.eventlist.arr[index];
   // updateCell(event.targetCell, modList, nextEventList) (335,17)
   _0_core_updateCell(event.targetCell, &modlist, &nexteventlist);
   // index := index + 1 (338,17)
   index = (asn1SccCellIndex) (index + 1);
   // DECISION index < length (eventList) (341,27)
   // ANSWER false (344,17)
   if((((index < ctxt.eventlist.nCount)) == false))
   {
      // accumModList := accumModList // modList (347,25)
      {
         memcpy_temp_9 = ctxt.accummodlist;
         for(memcpy_counter_9 = 0; memcpy_counter_9 < modlist.nCount; memcpy_counter_9++)
         {
            memcpy_temp_9.arr[memcpy_temp_9.nCount + memcpy_counter_9] = modlist.arr[memcpy_counter_9];
         }
         memcpy_temp_9.nCount += modlist.nCount;
      }
      ctxt.accummodlist = (asn1SccGridModificationList) memcpy_temp_9;
      // eventList := nextEventList (350,25)
      ctxt.eventlist = (asn1SccWaterIncrementEventList) nexteventlist;
      // applyModifications(modList) (353,25)
      _0_core_applyModifications(modlist);
      // COMMENT Advance the counter (359,20)
      // currentTimeStep := currentTimeStep + 1 (356,25)
      ctxt.currenttimestep = (asn1SccTimeStep) (ctxt.currenttimestep + 1);
      // RETURN  (None,None) at 126879241323136, 821
      return;
      // ANSWER true (365,17)
   } else if((((index < ctxt.eventlist.nCount)) == true))
   {
      // JOIN loop (368,25) at 126879171513344, 601
      goto loop;
   } //last
}


void _0_core_applyModifications(asn1SccGridModificationList modifications)
{
   asn1SccT_UInt32 index;
   asn1SccCellState cell;
   // index := 0 (389,17)
   index = (asn1SccT_UInt32) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (392,12)
   loop:
   // cell := coreGrid.cells(modifications(index).targetCell) (395,17)
   cell = (asn1SccCellState) ctxt.coregrid.cells.arr[modifications.arr[index].targetCell];
   // cell.waterVolume := cell.waterVolume + modifications(index).delta (398,17)
   cell.waterVolume = (asn1SccReal) (cell.waterVolume + modifications.arr[index].delta);
   // index := index + 1 (401,17)
   index = (asn1SccT_UInt32) (index + 1);
   // DECISION index < length (modifications) (404,27)
   // ANSWER false (407,17)
   if((((index < modifications.nCount)) == false))
   {
      // RETURN  (None,None) at 126879241818816, 530
      return;
      // ANSWER true (413,17)
   } else if((((index < modifications.nCount)) == true))
   {
      // JOIN loop (416,25) at 126879171720064, 530
      goto loop;
   } //last
}


void _0_core_addInitialEvents()
{
   asn1SccCellIndex index;
   asn1SccWaterIncrementEvent newevent;
   asn1SccUint memcpy_counter_10 = 0;
   asn1SccWaterIncrementEventList right_temp_10;
   asn1SccWaterIncrementEventList memcpy_temp_10;
   // index := 0 (434,17)
   index = (asn1SccCellIndex) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (437,12)
   loop:
   // DECISION coreGrid.cells(index).waterVolume (-1,-1)
   // ANSWER >0.0 (443,17)
   if(((ctxt.coregrid.cells.arr[index].waterVolume) > 0.0))
   {
      // newEvent := {targetCell index} (446,25)
      newevent = (asn1SccWaterIncrementEvent) {.targetCell = index};
      // eventList := eventList // {newEvent} (449,25)
      {
         right_temp_10 =(asn1SccWaterIncrementEventList) {1, {newevent}};
         memcpy_temp_10 = ctxt.eventlist;
         for(memcpy_counter_10 = 0; memcpy_counter_10 < 1; memcpy_counter_10++)
         {
            memcpy_temp_10.arr[memcpy_temp_10.nCount + memcpy_counter_10] = right_temp_10.arr[memcpy_counter_10];
         }
         memcpy_temp_10.nCount += 1;
      }
      ctxt.eventlist = (asn1SccWaterIncrementEventList) memcpy_temp_10;
      // ANSWER ELSE (None,None)
   } // 1
   else
   {
      // nothing done
   } //last
   // index := index +1 (456,17)
   index = (asn1SccCellIndex) (index + 1);
   // DECISION index < length (coreGrid.cells) (459,27)
   // ANSWER false (462,17)
   if((((index < ctxt.coregrid.cells.nCount)) == false))
   {
      // RETURN  (None,None) at 126879171949120, 594
      return;
      // ANSWER true (468,17)
   } else if((((index < ctxt.coregrid.cells.nCount)) == true))
   {
      // JOIN loop (471,25) at 126879171512448, 594
      goto loop;
   } //last
}


void _0_core_calculateGridCellNeighbors()
{
   asn1SccCellIndex index;
   asn1SccMooreNeighborList neighbors;
   asn1SccGridCellNeighborList right_temp_11;
   asn1SccGridCellNeighborList memcpy_temp_11;
   asn1SccUint memcpy_counter_11 = 0;
   // gridNeighbors := {} (489,17)
   ctxt.gridneighbors = (asn1SccGridCellNeighborList) asn1SccGridCellNeighborList_constant;
   // index := 0 (492,17)
   index = (asn1SccCellIndex) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (495,12)
   loop:
   // neighbors := {} (498,17)
   neighbors = (asn1SccMooreNeighborList) asn1SccMooreNeighborList_constant;
   // calculateCellMooreNeighbors(index, neighbors) (501,17)
   _0_core_calculateCellMooreNeighbors(index, &neighbors);
   // gridNeighbors := gridNeighbors // {neighbors} (504,17)
   {
      right_temp_11 =(asn1SccGridCellNeighborList) {1, {neighbors}};
      memcpy_temp_11 = ctxt.gridneighbors;
      for(memcpy_counter_11 = 0; memcpy_counter_11 < 1; memcpy_counter_11++)
      {
         memcpy_temp_11.arr[memcpy_temp_11.nCount + memcpy_counter_11] = right_temp_11.arr[memcpy_counter_11];
      }
      memcpy_temp_11.nCount += 1;
   }
   ctxt.gridneighbors = (asn1SccGridCellNeighborList) memcpy_temp_11;
   // index := index + 1 (507,17)
   index = (asn1SccCellIndex) (index + 1);
   // DECISION index < length (coreGrid.cells) (510,27)
   // ANSWER false (513,17)
   if((((index < ctxt.coregrid.cells.nCount)) == false))
   {
      // RETURN  (None,None) at 126879172203968, 631
      return;
      // ANSWER true (519,17)
   } else if((((index < ctxt.coregrid.cells.nCount)) == true))
   {
      // JOIN loop (522,25) at 126879172229696, 631
      goto loop;
   } //last
}


void _0_core_computeWaterFlow(asn1SccCellIndex selfcell, asn1SccCellIndex neighborcell, asn1SccOutflow *outflow)
{
   asn1SccCoefficient flowcoefficient;
   asn1SccT_UReal32 selfcellheight;
   asn1SccT_UReal32 neighborcellheight;
   // getFlowCoefficient(coreGrid.cells(selfCell).surfaceType, flowCoefficient) (547,17)
   _0_core_getFlowCoefficient(ctxt.coregrid.cells.arr[selfcell].surfaceType, &flowcoefficient);
   // selfCellHeight := coreGrid.cells(selfCell).elevation + coreGrid.cells(selfCell).waterVolume (550,17)
   selfcellheight = (asn1SccT_UReal32) (ctxt.coregrid.cells.arr[selfcell].elevation + ctxt.coregrid.cells.arr[selfcell].waterVolume);
   // neighborCellHeight := coreGrid.cells(neighborCell).elevation + coreGrid.cells(neighborCell).waterVolume (553,17)
   neighborcellheight = (asn1SccT_UReal32) (ctxt.coregrid.cells.arr[neighborcell].elevation + ctxt.coregrid.cells.arr[neighborcell].waterVolume);
   // DECISION selfCellHeight > neighborCellHeight (556,36)
   // ANSWER true (559,17)
   if((((selfcellheight > neighborcellheight)) == true))
   {
      // outflow.volume := flowcoefficient * (selfCellHeight - neighborCellHeight) (562,25)
      (*outflow).volume = (asn1SccReal) (flowcoefficient * (selfcellheight - neighborcellheight));
      // ANSWER false (565,17)
   } else if((((selfcellheight > neighborcellheight)) == false))
   {
      // outflow.volume := 0.0 (568,25)
      (*outflow).volume = (asn1SccReal) 0.0;
   } //last
   // outflow.neighbor := neighborCell (572,17)
   (*outflow).neighbor = (asn1SccCellIndex) neighborcell;
   // RETURN  (None,None) at 126879172021696, 550
   return;
}


void _0_core_normalizeOutflows(asn1SccT_UReal32 cellwatervolume, asn1SccT_UReal32 totaloutflowvolume, asn1SccOutflowList *celloutflowlist, asn1SccWaterVolume *normalizetotaloutflowvolume)
{
   asn1SccMooreNeighborIndex index;
   asn1SccT_UReal32 scalefactor;
   // DECISION totalOutflowVolume > cellWaterVolume (599,40)
   // ANSWER false (602,17)
   if((((totaloutflowvolume > cellwatervolume)) == false))
   {
      // RETURN  (None,None) at 126878932376256, 340
      return;
      // ANSWER true (608,17)
   } else if((((totaloutflowvolume > cellwatervolume)) == true))
   {
      // scaleFactor := cellWaterVolume / totalOutflowVolume (611,25)
      scalefactor = (asn1SccT_UReal32) (cellwatervolume / totaloutflowvolume);
      // normalizeTotalOutflowVolume := totalOutflowVolume * scaleFactor (614,25)
      (*normalizetotaloutflowvolume) = (asn1SccWaterVolume) (totaloutflowvolume * scalefactor);
      // index := 0 (617,25)
      index = (asn1SccMooreNeighborIndex) 0;
      // JOIN loop (None,None) at None, None
      goto loop;
   } //last
   // CONNECTION loop (620,20)
   loop:
   // cellOutflowList(index).volume := cellOutflowList(index).volume * scaleFactor (623,25)
   (*celloutflowlist).arr[index].volume = (asn1SccReal) ((*celloutflowlist).arr[index].volume * scalefactor);
   // index := index + 1 (626,25)
   index = (asn1SccMooreNeighborIndex) (index + 1);
   // DECISION index < length (cellOutflowList) (629,35)
   // ANSWER false (632,25)
   if((((index < (*celloutflowlist).nCount)) == false))
   {
      // RETURN  (None,None) at 126878932553728, 778
      return;
      // ANSWER true (638,25)
   } else if((((index < (*celloutflowlist).nCount)) == true))
   {
      // JOIN loop (641,33) at 126878932596416, 778
      goto loop;
   } //last
}




//// Definition Of Run Transition
void runTransitionCore(int Id)
{
   int trId = Id;
   flag message_pending = true;
   while (trId != -1)
   {
      switch(trId)
      {
         case 0:
         {
            // NEXT_STATE Idle (650,18) at 126879242555008, 99
            trId = -1;
            ctxt.state = asn1SccCore_States_idle;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // runSimulationStep (660,17)
            _0_core_runSimulationStep();
            // finished := currentTimeStep >= totalTimeSteps (663,17)
            ctxt.finished = (asn1SccT_Boolean) (ctxt.currenttimestep >= ctxt.totaltimesteps);
            // DECISION currentTimeStep >= nextSnapshotTimeStep (666,37)
            // ANSWER false (669,17)
            if((((ctxt.currenttimestep >= ctxt.nextsnapshottimestep)) == false))
            {
               ;
               // ANSWER true (672,17)
            } else if((((ctxt.currenttimestep >= ctxt.nextsnapshottimestep)) == true))
            {
               // displaySnapShot(accumModList, finished) (675,27)
               core_RI_displaySnapshot(&ctxt.accummodlist, &ctxt.finished);
               // nextSnapshotTimeStep := nextSnapshotTimeStep + coreSnapshotPeriod (678,25)
               ctxt.nextsnapshottimestep = (asn1SccTimeStep) (ctxt.nextsnapshottimestep + ctxt.coresnapshotperiod);
            } //last
            // DECISION finished (-1,-1)
            // ANSWER false (685,17)
            if(((ctxt.finished) == false))
            {
               // NEXT_STATE Running (688,30) at 126879242791424, 756
               trId = -1;
               ctxt.state = asn1SccCore_States_running;
               goto continuous_signals;
               // ANSWER true (691,17)
            } else if(((ctxt.finished) == true))
            {
               // NEXT_STATE Idle (694,30) at 126879242835584, 756
               trId = -1;
               ctxt.state = asn1SccCore_States_idle;
               goto continuous_signals;
            } //last
            break;
         }
         case 2:
         {
            // NEXT_STATE Running (706,22) at 126879242582144, 209
            trId = -1;
            ctxt.state = asn1SccCore_States_running;
            goto continuous_signals;
            break;
         }
         case CS_Only:
         {
            trId = -1;
            goto continuous_signals;
         }
         default:
         {
            break;
         }
      }
      continuous_signals:

      // Process Continuous Signals
      if(ctxt.init_done)
      {
         core_check_queue(&message_pending);
      }

      if(message_pending || trId != -1)
      {
         goto next_transition;
      }

      if(ctxt.state == asn1SccCore_States_running)
      {
         // Priority: 1
         // DECISION true (-1,-1)
         // ANSWER true (None,None)
         if(((true) == true))
         {
            trId = 1;
         } // inner if
      } // current state
      next_transition:
      ;
   }
}


//// Current State To String
char* core_state(void)
{
     return "Not_supported_in_C__Use_the_Ada_backend";
}