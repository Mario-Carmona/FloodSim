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
void _0_core_normalizeOutflows(asn1SccT_UInt32 cellwaterdepth, asn1SccT_UInt32 totaloutflowdepth, asn1SccOutflowList celloutflowlist, asn1SccOutflowList *normalizecelloutflowlist, asn1SccT_UInt32 *normalizetotaloutflowdepth);
void _0_core_computeWaterFlow(asn1SccCellIndex selfcell, asn1SccCellIndex neighborcell, asn1SccOutflow *outflow);
void _0_core_calculateGridCellNeighbors();
void _0_core_addInitialEvents();
void _0_core_applyModifications(asn1SccGridModificationList modifications);
void _0_core_runSimulationStep();
void core_PI_startsimulationfromgis(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod);
void _0_core_calculateCellMooreNeighbors(asn1SccCellIndex index, asn1SccMooreNeighborList *neighbors);
void _0_core_updateCell(asn1SccCellIndex cellindex, asn1SccGridModificationList *modlist, asn1SccFlowingWaterEventList *nexteventlist);
void _0_core_getFlowCoefficient(asn1SccSurfaceType surfacetype, asn1SccFlowCoefficient *flowcoefficient);


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
void _0_core_normalizeOutflows(asn1SccT_UInt32 cellwaterdepth, asn1SccT_UInt32 totaloutflowdepth, asn1SccOutflowList celloutflowlist, asn1SccOutflowList *normalizecelloutflowlist, asn1SccT_UInt32 *normalizetotaloutflowdepth)
{
   asn1SccMooreNeighborIndex index;
   asn1SccScaleFactor scalefactor;
   // DECISION totalOutflowDepth > cellWaterDepth (47,39)
   // ANSWER false (50,17)
   if((((totaloutflowdepth > cellwaterdepth)) == false))
   {
      // normalizeCellOutflowList := cellOutflowList (53,25)
      (*normalizecelloutflowlist) = (asn1SccOutflowList) celloutflowlist;
      // normalizeTotalOutflowDepth := totalOutflowDepth (56,25)
      (*normalizetotaloutflowdepth) = (asn1SccT_UInt32) totaloutflowdepth;
      // RETURN  (None,None) at 137970063850944, 440
      return;
      // ANSWER true (62,17)
   } else if((((totaloutflowdepth > cellwaterdepth)) == true))
   {
      // scaleFactor := float (cellWaterDepth)  / float ( totalOutflowDepth) (65,25)
      scalefactor = (asn1SccScaleFactor) ((asn1Real64)(cellwaterdepth) / (asn1Real64)(totaloutflowdepth));
      // normalizeTotalOutflowDepth := cellWaterDepth (68,25)
      (*normalizetotaloutflowdepth) = (asn1SccT_UInt32) cellwaterdepth;
      // index := 0 (71,25)
      index = (asn1SccMooreNeighborIndex) 0;
      // JOIN loop (None,None) at None, None
      goto loop;
   } //last
   // CONNECTION loop (74,20)
   loop:
   // normalizeCellOutflowList(index).depth := fix (floor (float (cellOutflowList(index).depth) * scaleFactor)) (77,25)
   (*normalizecelloutflowlist).arr[index].depth = (asn1SccT_UInt32) (asn1SccUint)(floor(((asn1Real64)(celloutflowlist.arr[index].depth) * scalefactor)));
   // index := index + 1 (80,25)
   index = (asn1SccMooreNeighborIndex) (index + 1);
   // DECISION index < length (cellOutflowList) (83,35)
   // ANSWER false (86,25)
   if((((index < celloutflowlist.nCount)) == false))
   {
      // RETURN  (None,None) at 137970062090880, 778
      return;
      // ANSWER true (92,25)
   } else if((((index < celloutflowlist.nCount)) == true))
   {
      // JOIN loop (95,33) at 137970064044352, 778
      goto loop;
   } //last
}


void _0_core_computeWaterFlow(asn1SccCellIndex selfcell, asn1SccCellIndex neighborcell, asn1SccOutflow *outflow)
{
   asn1SccFlowCoefficient flowcoefficient;
   asn1SccHeight selfcellheight;
   asn1SccHeight neighborcellheight;
   // getFlowCoefficient(coreGrid.cells(selfCell).surfaceType, flowCoefficient) (121,17)
   _0_core_getFlowCoefficient(ctxt.coregrid.cells.arr[selfcell].surfaceType, &flowcoefficient);
   // selfCellHeight := coreGrid.cells(selfCell).elevation + coreGrid.cells(selfCell).waterDepth (124,17)
   selfcellheight = (asn1SccHeight) (ctxt.coregrid.cells.arr[selfcell].elevation + ctxt.coregrid.cells.arr[selfcell].waterDepth);
   // neighborCellHeight := coreGrid.cells(neighborCell).elevation + coreGrid.cells(neighborCell).waterDepth (127,17)
   neighborcellheight = (asn1SccHeight) (ctxt.coregrid.cells.arr[neighborcell].elevation + ctxt.coregrid.cells.arr[neighborcell].waterDepth);
   // DECISION selfCellHeight > neighborCellHeight (130,36)
   // ANSWER false (133,17)
   if((((selfcellheight > neighborcellheight)) == false))
   {
      // outflow.depth := 0 (136,25)
      (*outflow).depth = (asn1SccT_UInt32) 0;
      // ANSWER true (139,17)
   } else if((((selfcellheight > neighborcellheight)) == true))
   {
      // outflow.depth := fix (floor (flowCoefficient * float (selfCellHeight - neighborCellHeight))) (142,25)
      (*outflow).depth = (asn1SccT_UInt32) (asn1SccUint)(floor((flowcoefficient * (asn1Real64)((selfcellheight - neighborcellheight)))));
   } //last
   // outflow.neighbor := neighborCell (146,17)
   (*outflow).neighbor = (asn1SccCellIndex) neighborcell;
   // RETURN  (None,None) at 137970062351296, 550
   return;
}


void _0_core_calculateGridCellNeighbors()
{
   asn1SccCellIndex index;
   asn1SccMooreNeighborList neighbors;
   asn1SccUint memcpy_counter_89 = 0;
   asn1SccGridCellNeighborList right_temp_89;
   asn1SccGridCellNeighborList memcpy_temp_89;
   // gridNeighbors := {} (166,17)
   ctxt.gridneighbors = (asn1SccGridCellNeighborList) asn1SccGridCellNeighborList_constant;
   // index := 0 (169,17)
   index = (asn1SccCellIndex) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (172,12)
   loop:
   // neighbors := {} (175,17)
   neighbors = (asn1SccMooreNeighborList) asn1SccMooreNeighborList_constant;
   // calculateCellMooreNeighbors(index, neighbors) (178,17)
   _0_core_calculateCellMooreNeighbors(index, &neighbors);
   // gridNeighbors := gridNeighbors // {neighbors} (181,17)
   {
      right_temp_89 =(asn1SccGridCellNeighborList) {1, {neighbors}};
      memcpy_temp_89 = ctxt.gridneighbors;
      for(memcpy_counter_89 = 0; memcpy_counter_89 < 1; memcpy_counter_89++)
      {
         memcpy_temp_89.arr[memcpy_temp_89.nCount + memcpy_counter_89] = right_temp_89.arr[memcpy_counter_89];
      }
      memcpy_temp_89.nCount += 1;
   }
   ctxt.gridneighbors = (asn1SccGridCellNeighborList) memcpy_temp_89;
   // index := index + 1 (184,17)
   index = (asn1SccCellIndex) (index + 1);
   // DECISION index < length (coreGrid.cells) (187,27)
   // ANSWER false (190,17)
   if((((index < ctxt.coregrid.cells.nCount)) == false))
   {
      // RETURN  (None,None) at 137970062551360, 631
      return;
      // ANSWER true (196,17)
   } else if((((index < ctxt.coregrid.cells.nCount)) == true))
   {
      // JOIN loop (199,25) at 137970062594240, 631
      goto loop;
   } //last
}


void _0_core_addInitialEvents()
{
   asn1SccCellIndex index;
   asn1SccFlowingWaterEvent newevent;
   asn1SccFlowingWaterEventList memcpy_temp_90;
   asn1SccUint memcpy_counter_90 = 0;
   asn1SccFlowingWaterEventList right_temp_90;
   // index := 0 (217,17)
   index = (asn1SccCellIndex) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (220,12)
   loop:
   // DECISION coreGrid.cells(index).waterDepth (-1,-1)
   // ANSWER >0 (226,17)
   if(((ctxt.coregrid.cells.arr[index].waterDepth) > 0))
   {
      // newEvent := {targetCell index} (229,25)
      newevent = (asn1SccFlowingWaterEvent) {.targetCell = index};
      // eventList := eventList // {newEvent} (232,25)
      {
         right_temp_90 =(asn1SccFlowingWaterEventList) {1, {newevent}};
         memcpy_temp_90 = ctxt.eventlist;
         for(memcpy_counter_90 = 0; memcpy_counter_90 < 1; memcpy_counter_90++)
         {
            memcpy_temp_90.arr[memcpy_temp_90.nCount + memcpy_counter_90] = right_temp_90.arr[memcpy_counter_90];
         }
         memcpy_temp_90.nCount += 1;
      }
      ctxt.eventlist = (asn1SccFlowingWaterEventList) memcpy_temp_90;
      // ANSWER ELSE (None,None)
   } // 1
   else
   {
      // nothing done
   } //last
   // index := index +1 (239,17)
   index = (asn1SccCellIndex) (index + 1);
   // DECISION index < length (coreGrid.cells) (242,27)
   // ANSWER false (245,17)
   if((((index < ctxt.coregrid.cells.nCount)) == false))
   {
      // RETURN  (None,None) at 137970062818880, 594
      return;
      // ANSWER true (251,17)
   } else if((((index < ctxt.coregrid.cells.nCount)) == true))
   {
      // JOIN loop (254,25) at 137970062861376, 594
      goto loop;
   } //last
}


void _0_core_applyModifications(asn1SccGridModificationList modifications)
{
   asn1SccT_UInt32 index;
   asn1SccCellState cell;
   // index := 0 (275,17)
   index = (asn1SccT_UInt32) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (278,12)
   loop:
   // cell := coreGrid.cells(modifications(index).targetCell) (281,17)
   cell = (asn1SccCellState) ctxt.coregrid.cells.arr[modifications.arr[index].targetCell];
   // DECISION modifications(index).increment (-1,-1)
   // ANSWER true (287,17)
   if(((modifications.arr[index].increment) == true))
   {
      // cell.waterDepth := cell.waterDepth + modifications(index).delta (290,25)
      cell.waterDepth = (asn1SccT_UInt32) (cell.waterDepth + modifications.arr[index].delta);
      // ANSWER false (293,17)
   } else if(((modifications.arr[index].increment) == false))
   {
      // cell.waterDepth := cell.waterDepth - modifications(index).delta (296,25)
      cell.waterDepth = (asn1SccT_UInt32) (cell.waterDepth - modifications.arr[index].delta);
   } //last
   // index := index + 1 (300,17)
   index = (asn1SccT_UInt32) (index + 1);
   // DECISION index < length (modifications) (303,27)
   // ANSWER false (306,17)
   if((((index < modifications.nCount)) == false))
   {
      // RETURN  (None,None) at 137970060506432, 644
      return;
      // ANSWER true (312,17)
   } else if((((index < modifications.nCount)) == true))
   {
      // JOIN loop (315,25) at 137970060564864, 644
      goto loop;
   } //last
}


void _0_core_runSimulationStep()
{
   asn1SccFlowingWaterEventList nexteventlist;
   asn1SccGridModificationList modlist;
   asn1SccCellIndex index;
   asn1SccFlowingWaterEvent event;
   asn1SccGridModificationList memcpy_temp_91;
   int memcpy_counter_91 = 0;
   // modList := {} (336,17)
   modlist = (asn1SccGridModificationList) asn1SccGridModificationList_constant;
   // nextEventList := {} (339,17)
   nexteventlist = (asn1SccFlowingWaterEventList) asn1SccFlowingWaterEventList_constant;
   // index := 0 (342,17)
   index = (asn1SccCellIndex) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (345,12)
   loop:
   // event := eventList(index) (348,17)
   event = (asn1SccFlowingWaterEvent) ctxt.eventlist.arr[index];
   // updateCell(event.targetCell, modList, nextEventList) (351,17)
   _0_core_updateCell(event.targetCell, &modlist, &nexteventlist);
   // index := index + 1 (354,17)
   index = (asn1SccCellIndex) (index + 1);
   // DECISION index < length (eventList) (357,27)
   // ANSWER false (360,17)
   if((((index < ctxt.eventlist.nCount)) == false))
   {
      // accumModList := accumModList // modList (363,25)
      {
         memcpy_temp_91 = ctxt.accummodlist;
         for(memcpy_counter_91 = 0; memcpy_counter_91 < modlist.nCount; memcpy_counter_91++)
         {
            memcpy_temp_91.arr[memcpy_temp_91.nCount + memcpy_counter_91] = modlist.arr[memcpy_counter_91];
         }
         memcpy_temp_91.nCount += modlist.nCount;
      }
      ctxt.accummodlist = (asn1SccGridModificationList) memcpy_temp_91;
      // eventList := nextEventList (366,25)
      ctxt.eventlist = (asn1SccFlowingWaterEventList) nexteventlist;
      // applyModifications(modList) (369,25)
      _0_core_applyModifications(modlist);
      // COMMENT Advance the counter (375,20)
      // currentTimeStep := currentTimeStep + 1 (372,25)
      ctxt.currenttimestep = (asn1SccTimeStep) (ctxt.currenttimestep + 1);
      // RETURN  (None,None) at 137970062165696, 821
      return;
      // ANSWER true (381,17)
   } else if((((index < ctxt.eventlist.nCount)) == true))
   {
      // JOIN loop (384,25) at 137970060869184, 601
      goto loop;
   } //last
}


void core_PI_startsimulationfromgis(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod)
{
   asn1SccFilePath folder = *in__folder;
   asn1SccDuration timestepduration = *in__timestepduration;
   asn1SccDuration totalduration = *in__totalduration;
   asn1SccTimeStep snapshotperiod = *in__snapshotperiod;
   asn1SccFilePath tmp11286;
   // get_sender(sender) (1,5)
   core_RI_get_sender(&ctxt.sender);
   // currentTimeStep := 0 (405,17)
   ctxt.currenttimestep = (asn1SccTimeStep) 0;
   // totalTimeSteps := totalDuration / timeStepDuration (408,17)
   ctxt.totaltimesteps = (asn1SccTimeStep) (totalduration / timestepduration);
   // coreSnapshotPeriod := snapshotPeriod (411,17)
   ctxt.coresnapshotperiod = (asn1SccTimeStep) snapshotperiod;
   // nextSnapshotTimeStep := coreSnapshotPeriod (414,17)
   ctxt.nextsnapshottimestep = (asn1SccTimeStep) ctxt.coresnapshotperiod;
   // eventList := {} (417,17)
   ctxt.eventlist = (asn1SccFlowingWaterEventList) asn1SccFlowingWaterEventList_constant;
   // accumModList := {} (420,17)
   ctxt.accummodlist = (asn1SccGridModificationList) asn1SccGridModificationList_constant;
   // loadGISData(folder, coreGrid) (423,17)
   tmp11286 = (asn1SccFilePath) folder;
   core_RI_loadGISData(&tmp11286, &ctxt.coregrid);
   // displayInitialGrid(coreGrid) (426,19)
   core_RI_displayInitialGrid(&ctxt.coregrid);
   // addInitialEvents (429,17)
   _0_core_addInitialEvents();
   // calculateGridCellNeighbors (432,17)
   _0_core_calculateGridCellNeighbors();
   // core_startSimulationFromGIS_transition (None,None)
   core_startsimulationfromgis_transition();
   // RETURN  (None,None) at 137970062211968, 648
   return;
}


// Required To Work With TASTE's Wrappers
void core_PI_startSimulationFromGIS(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod)
{
   core_PI_startsimulationfromgis(in__folder, in__timestepduration, in__totalduration, in__snapshotperiod);
}

void _0_core_calculateCellMooreNeighbors(asn1SccCellIndex index, asn1SccMooreNeighborList *neighbors)
{
   // RETURN  (None,None) at 137970061108608, 188
   return;
}


void _0_core_updateCell(asn1SccCellIndex cellindex, asn1SccGridModificationList *modlist, asn1SccFlowingWaterEventList *nexteventlist)
{
   asn1SccMooreNeighborList neighbors;
   asn1SccCellState cell;
   asn1SccCellIndex neighbor;
   asn1SccMooreNeighborIndex neighborindex;
   asn1SccOutflow outflow;
   asn1SccOutflowList celloutflowlist;
   asn1SccT_UInt32 totaloutflowdepth;
   asn1SccFlowingWaterEvent newevent;
   asn1SccMooreNeighborIndex outflowindex;
   asn1SccGridModification modification;
   asn1SccOutflowList normalizecelloutflowlist;
   asn1SccT_UInt32 normalizetotaloutflowdepth;
   asn1SccUint memcpy_counter_94 = 0;
   asn1SccGridModificationList memcpy_temp_96;
   asn1SccFlowingWaterEventList memcpy_temp_95;
   asn1SccGridModificationList memcpy_temp_93;
   asn1SccFlowingWaterEventList right_temp_95;
   asn1SccFlowingWaterEventList right_temp_92;
   asn1SccUint memcpy_counter_92 = 0;
   asn1SccUint memcpy_counter_96 = 0;
   asn1SccOutflowList right_temp_94;
   asn1SccUint memcpy_counter_93 = 0;
   asn1SccGridModificationList right_temp_96;
   asn1SccFlowingWaterEventList memcpy_temp_92;
   asn1SccGridModificationList right_temp_93;
   asn1SccUint memcpy_counter_95 = 0;
   asn1SccOutflowList memcpy_temp_94;
   // neighbors := gridNeighbors(cellIndex) (494,17)
   neighbors = (asn1SccMooreNeighborList) ctxt.gridneighbors.arr[cellindex];
   // neighborIndex := 0 (497,17)
   neighborindex = (asn1SccMooreNeighborIndex) 0;
   // cellOutFlowList := {} (500,17)
   celloutflowlist = (asn1SccOutflowList) asn1SccOutflowList_constant;
   // totalOutflowDepth := 0 (503,17)
   totaloutflowdepth = (asn1SccT_UInt32) 0;
   // JOIN loopComputeWaterFlow (None,None) at None, None
   goto loopcomputewaterflow;
   // CONNECTION loopAddGridModifications (571,28)
   loopaddgridmodifications:
   // newEvent := {targetCell cellOutflowList(outflowIndex).neighbor} (574,33)
   newevent = (asn1SccFlowingWaterEvent) {.targetCell = celloutflowlist.arr[outflowindex].neighbor};
   // nextEventList := nextEventList // {newEvent} (577,33)
   {
      right_temp_92 =(asn1SccFlowingWaterEventList) {1, {newevent}};
      memcpy_temp_92 = (*nexteventlist);
      for(memcpy_counter_92 = 0; memcpy_counter_92 < 1; memcpy_counter_92++)
      {
         memcpy_temp_92.arr[memcpy_temp_92.nCount + memcpy_counter_92] = right_temp_92.arr[memcpy_counter_92];
      }
      memcpy_temp_92.nCount += 1;
   }
   (*nexteventlist) = (asn1SccFlowingWaterEventList) memcpy_temp_92;
   // modification := {
   //   targetCell cellOutflowList(outflowIndex).neighbor, 
   //   increment true,
   //   delta cellOutflowList(outflowIndex).depth
   // } (580,33)
   modification = (asn1SccGridModification) {.targetCell = celloutflowlist.arr[outflowindex].neighbor, .increment = true, .delta = celloutflowlist.arr[outflowindex].depth};
   // modList := modList // {modification} (587,33)
   {
      right_temp_93 =(asn1SccGridModificationList) {1, {modification}};
      memcpy_temp_93 = (*modlist);
      for(memcpy_counter_93 = 0; memcpy_counter_93 < 1; memcpy_counter_93++)
      {
         memcpy_temp_93.arr[memcpy_temp_93.nCount + memcpy_counter_93] = right_temp_93.arr[memcpy_counter_93];
      }
      memcpy_temp_93.nCount += 1;
   }
   (*modlist) = (asn1SccGridModificationList) memcpy_temp_93;
   // outflowIndex := outflowIndex + 1 (590,33)
   outflowindex = (asn1SccMooreNeighborIndex) (outflowindex + 1);
   // DECISION outflowIndex < length (cellOutflowList) (593,50)
   // ANSWER false (596,33)
   if((((outflowindex < celloutflowlist.nCount)) == false))
   {
      // RETURN  (None,None) at 137970058616960, 1912
      return;
      // ANSWER true (602,33)
   } else if((((outflowindex < celloutflowlist.nCount)) == true))
   {
      // JOIN loopAddGridModifications (605,41) at 137970058642624, 1912
      goto loopaddgridmodifications;
   } //last
   // CONNECTION loopComputeWaterFlow (506,12)
   loopcomputewaterflow:
   // computeWaterFlow(cellIndex, neighbors(neighborIndex), outflow) (509,17)
   _0_core_computeWaterFlow(cellindex, neighbors.arr[neighborindex], &outflow);
   // DECISION outflow.depth (-1,-1)
   // ANSWER >0 (515,17)
   if(((outflow.depth) > 0))
   {
      // cellOutflowList := cellOutflowList // {outflow} (518,25)
      {
         right_temp_94 =(asn1SccOutflowList) {1, {outflow}};
         memcpy_temp_94 = celloutflowlist;
         for(memcpy_counter_94 = 0; memcpy_counter_94 < 1; memcpy_counter_94++)
         {
            memcpy_temp_94.arr[memcpy_temp_94.nCount + memcpy_counter_94] = right_temp_94.arr[memcpy_counter_94];
         }
         memcpy_temp_94.nCount += 1;
      }
      celloutflowlist = (asn1SccOutflowList) memcpy_temp_94;
      // totalOutflowDepth := totalOutflowDepth + outflow.depth (521,25)
      totaloutflowdepth = (asn1SccT_UInt32) (totaloutflowdepth + outflow.depth);
      // ANSWER ELSE (None,None)
   } // 1
   else
   {
      // nothing done
   } //last
   // neighborIndex := neighborIndex + 1 (528,17)
   neighborindex = (asn1SccMooreNeighborIndex) (neighborindex + 1);
   // DECISION neighborIndex < length (neighbors) (531,35)
   // ANSWER true (534,17)
   if((((neighborindex < neighbors.nCount)) == true))
   {
      // JOIN loopComputeWaterFlow (537,25) at 137970061406208, 930
      goto loopcomputewaterflow;
      // ANSWER false (540,17)
   } else if((((neighborindex < neighbors.nCount)) == false))
   {
      // DECISION length (cellOutflowList) (-1,-1)
      // ANSWER >0 (546,25)
      if(((celloutflowlist.nCount) > 0))
      {
         // normalizeOutflows(coreGrid.cells(cellIndex).waterDepth, totalOutflowDepth, cellOutflowList, normalizeCellOutflowList, normalizeTotalOutflowDepth) (549,33)
         _0_core_normalizeOutflows(ctxt.coregrid.cells.arr[cellindex].waterDepth, totaloutflowdepth, celloutflowlist, &normalizecelloutflowlist, &normalizetotaloutflowdepth);
         // newEvent := {targetCell cellIndex} (552,33)
         newevent = (asn1SccFlowingWaterEvent) {.targetCell = cellindex};
         // nextEventList := nextEventList // {newEvent} (555,33)
         {
            right_temp_95 =(asn1SccFlowingWaterEventList) {1, {newevent}};
            memcpy_temp_95 = (*nexteventlist);
            for(memcpy_counter_95 = 0; memcpy_counter_95 < 1; memcpy_counter_95++)
            {
               memcpy_temp_95.arr[memcpy_temp_95.nCount + memcpy_counter_95] = right_temp_95.arr[memcpy_counter_95];
            }
            memcpy_temp_95.nCount += 1;
         }
         (*nexteventlist) = (asn1SccFlowingWaterEventList) memcpy_temp_95;
         // modification := {
         //   targetCell cellIndex, 
         //   increment false,
         //   delta normalizeTotalOutflowDepth
         // } (558,33)
         modification = (asn1SccGridModification) {.targetCell = cellindex, .increment = false, .delta = normalizetotaloutflowdepth};
         // modList := modList // {modification} (565,33)
         {
            right_temp_96 =(asn1SccGridModificationList) {1, {modification}};
            memcpy_temp_96 = (*modlist);
            for(memcpy_counter_96 = 0; memcpy_counter_96 < 1; memcpy_counter_96++)
            {
               memcpy_temp_96.arr[memcpy_temp_96.nCount + memcpy_counter_96] = right_temp_96.arr[memcpy_counter_96];
            }
            memcpy_temp_96.nCount += 1;
         }
         (*modlist) = (asn1SccGridModificationList) memcpy_temp_96;
         // outflowIndex := 0 (568,33)
         outflowindex = (asn1SccMooreNeighborIndex) 0;
         // JOIN loopAddGridModifications (None,None) at None, None
         goto loopaddgridmodifications;
         // ANSWER ELSE (None,None)
      } // 1
      else
      {
         // RETURN  (None,None) at 137970058669824, 1043
         return;
      } //last
   } //last
}


void _0_core_getFlowCoefficient(asn1SccSurfaceType surfacetype, asn1SccFlowCoefficient *flowcoefficient)
{
   // DECISION surfaceType (-1,-1)
   // ANSWER rock (635,17)
   if(((surfacetype) == asn1SccSurfaceType_rock))
   {
      // flowCoefficient := 0.2 (638,25)
      (*flowcoefficient) = (asn1SccFlowCoefficient) 0.2;
      // ANSWER asphalt (641,17)
   } else if(((surfacetype) == asn1SccSurfaceType_asphalt))
   {
      // flowCoefficient := 0.2 (644,25)
      (*flowcoefficient) = (asn1SccFlowCoefficient) 0.2;
      // ANSWER forest (647,17)
   } else if(((surfacetype) == asn1SccSurfaceType_forest))
   {
      // flowCoefficient := 0.2 (650,25)
      (*flowcoefficient) = (asn1SccFlowCoefficient) 0.2;
      // ANSWER waterSurface (653,17)
   } else if(((surfacetype) == asn1SccSurfaceType_waterSurface))
   {
      // flowCoefficient := 0.2 (656,25)
      (*flowcoefficient) = (asn1SccFlowCoefficient) 0.2;
      // ANSWER soil (659,17)
   } else if(((surfacetype) == asn1SccSurfaceType_soil))
   {
      // flowCoefficient := 0.2 (662,25)
      (*flowcoefficient) = (asn1SccFlowCoefficient) 0.2;
   } //last
   // RETURN  (None,None) at 137970058919232, 303
   return;
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
            // NEXT_STATE Idle (673,18) at 137970063418432, 99
            trId = -1;
            ctxt.state = asn1SccCore_States_idle;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // runSimulationStep (683,17)
            _0_core_runSimulationStep();
            // finished := currentTimeStep >= totalTimeSteps (686,17)
            ctxt.finished = (asn1SccT_Boolean) (ctxt.currenttimestep >= ctxt.totaltimesteps);
            // DECISION currentTimeStep >= nextSnapshotTimeStep (689,37)
            // ANSWER false (692,17)
            if((((ctxt.currenttimestep >= ctxt.nextsnapshottimestep)) == false))
            {
               ;
               // ANSWER true (695,17)
            } else if((((ctxt.currenttimestep >= ctxt.nextsnapshottimestep)) == true))
            {
               // displaySnapShot(accumModList, finished) (698,27)
               core_RI_displaySnapshot(&ctxt.accummodlist, &ctxt.finished);
               // nextSnapshotTimeStep := nextSnapshotTimeStep + coreSnapshotPeriod (701,25)
               ctxt.nextsnapshottimestep = (asn1SccTimeStep) (ctxt.nextsnapshottimestep + ctxt.coresnapshotperiod);
            } //last
            // DECISION finished (-1,-1)
            // ANSWER false (708,17)
            if(((ctxt.finished) == false))
            {
               // NEXT_STATE Running (711,30) at 137970063849856, 756
               trId = -1;
               ctxt.state = asn1SccCore_States_running;
               goto continuous_signals;
               // ANSWER true (714,17)
            } else if(((ctxt.finished) == true))
            {
               // NEXT_STATE Idle (717,30) at 137970063878848, 756
               trId = -1;
               ctxt.state = asn1SccCore_States_idle;
               goto continuous_signals;
            } //last
            break;
         }
         case 2:
         {
            // NEXT_STATE Running (729,22) at 137970063657600, 209
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