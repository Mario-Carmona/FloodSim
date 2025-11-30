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
void _0_core_normalizeOutflows(asn1SccT_UInt32 cellwaterdepth, asn1SccT_UInt32 totaloutflowdepth, asn1SccOutflowList celloutflowlist, asn1SccT_UInt32 *normalizetotaloutflowdepth, asn1SccOutflowList *normalizecelloutflowlist);
void _0_core_computeOutflow(asn1SccT_UInt64 selfcellheight, asn1SccT_UInt32 selfcelldepth, asn1SccFrictionCoefficient frictioncoefficient, asn1SccCellIndex neighborcell, asn1SccOutflow *outflow);
void _0_core_calculateGridCellNeighbors();
void _0_core_addInitialEvents();
void _0_core_applyModifications(asn1SccGridModificationList modifications);
void _0_core_runSimulationStep();
void core_PI_startsimulationfromgis(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod);
void _0_core_calculateCellMooreNeighbors(asn1SccCellIndex index, asn1SccMooreNeighborList *neighbors);
void _0_core_updateCell(asn1SccCellIndex cellindex, asn1SccGridModificationList *modificationlist, asn1SccFlowingWaterEventList *nextflowingwatereventlist);
void _0_core_getFrictionCoefficient(asn1SccSurfaceType surfacetype, asn1SccFrictionCoefficient *frictioncoefficient);
void _0_core_addFlowingWaterEvent(asn1SccCellIndex targetcell, asn1SccFlowingWaterEventList *eventlist);
void _0_core_addGridModification(asn1SccCellIndex targetcell, asn1SccT_Boolean increment, asn1SccT_UInt32 delta, asn1SccGridModificationList *modificationlist);


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
void _0_core_normalizeOutflows(asn1SccT_UInt32 cellwaterdepth, asn1SccT_UInt32 totaloutflowdepth, asn1SccOutflowList celloutflowlist, asn1SccT_UInt32 *normalizetotaloutflowdepth, asn1SccOutflowList *normalizecelloutflowlist)
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
      // RETURN  (None,None) at 132427198483392, 440
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
      // RETURN  (None,None) at 132427200170880, 778
      return;
      // ANSWER true (92,25)
   } else if((((index < celloutflowlist.nCount)) == true))
   {
      // JOIN loop (95,33) at 132427198196736, 778
      goto loop;
   } //last
}


void _0_core_computeOutflow(asn1SccT_UInt64 selfcellheight, asn1SccT_UInt32 selfcelldepth, asn1SccFrictionCoefficient frictioncoefficient, asn1SccCellIndex neighborcell, asn1SccOutflow *outflow)
{
   asn1SccT_UInt64 neighborcellheight;
   asn1SccDouble g = 9.8;
   asn1SccT_UInt64 dh_mm;
   asn1SccDouble dh_m;
   asn1SccT_Boolean diag;
   asn1SccDouble distance_ij_m;
   asn1SccDouble m_sqrt2;
   asn1SccDouble height_avg_m;
   asn1SccDouble speed;
   asn1SccDouble area_face;
   asn1SccDouble flow;
   asn1SccDouble volume;
   // neighborCellHeight := coreGrid.cells(neighborCell).elevation + coreGrid.cells(neighborCell).waterDepth (142,17)
   neighborcellheight = (asn1SccT_UInt64) (ctxt.coregrid.cells.arr[neighborcell].elevation + ctxt.coregrid.cells.arr[neighborcell].waterDepth);
   // DECISION selfCellHeight > neighborCellHeight (145,36)
   // ANSWER false (148,17)
   if((((selfcellheight > neighborcellheight)) == false))
   {
      // outflow.depth := 0 (151,25)
      (*outflow).depth = (asn1SccT_UInt32) 0;
      // outflow.neighbor := neighborCell (154,25)
      (*outflow).neighbor = (asn1SccCellIndex) neighborcell;
      // RETURN  (None,None) at 132426688957568, 449
      return;
      // ANSWER true (160,17)
   } else if((((selfcellheight > neighborcellheight)) == true))
   {
      // dH_mm := selfCellHeight - neighborCellHeight (163,25)
      dh_mm = (asn1SccT_UInt64) (selfcellheight - neighborcellheight);
      // dH_m := float (dH_mm) * 0.001 (166,25)
      dh_m = (asn1SccDouble) ((asn1Real64)(dh_mm) * 0.001);
      // DECISION diag (-1,-1)
      // ANSWER true (172,25)
      if(((diag) == true))
      {
         // distance_ij_m := coreGrid.cellSize * M_SQRT2 (175,33)
         distance_ij_m = (asn1SccDouble) (ctxt.coregrid.cellSize * m_sqrt2);
         // ANSWER false (178,25)
      } else if(((diag) == false))
      {
         // distance_ij_m := coreGrid.cellSize (181,33)
         distance_ij_m = (asn1SccDouble) ctxt.coregrid.cellSize;
      } //last
      // height_avg_m := (selfCellDepth + coreGrid.cells(neighborCell).waterDepth) * 0.5 * 0.001 (185,25)
      height_avg_m = (asn1SccDouble) (((selfcelldepth + ctxt.coregrid.cells.arr[neighborcell].waterDepth) * 0.5) * 0.001);
      // DECISION height_avg_m (-1,-1)
      // ANSWER <0.001 (191,25)
      if(((height_avg_m) < 0.001))
      {
         // height_avg_m := 0.001 (194,33)
         height_avg_m = (asn1SccDouble) 0.001;
         // ANSWER ELSE (None,None)
      } // 1
      else
      {
         // nothing done
      } //last
      // speed := frictionCoefficient * sqrt ((2.0 * g *dH_m) / distance_ij_m) (201,25)
      speed = (asn1SccDouble) (frictioncoefficient * sqrt((((2.0 * g) * dh_m) / distance_ij_m)));
      // area_face := height_avg_m * coreGrid.cellSize (204,25)
      area_face = (asn1SccDouble) (height_avg_m * ctxt.coregrid.cellSize);
      // flow := speed * area_face (207,25)
      flow = (asn1SccDouble) (speed * area_face);
      // volume := flow * timeStepDuration (210,25)
      volume = (asn1SccDouble) (flow * timeStepDuration);
      // outflow.depth := fix (floor (flowCoefficient * float (selfCellHeight - neighborCellHeight))) (213,25)
      (*outflow).depth = (asn1SccT_UInt32) (asn1SccUint)(floor((flowCoefficient * (asn1Real64)((selfcellheight - neighborcellheight)))));
      // outflow.neighbor := neighborCell (216,25)
      (*outflow).neighbor = (asn1SccCellIndex) neighborcell;
      // RETURN  (None,None) at 132426688954880, 1147
      return;
   } //last
}


void _0_core_calculateGridCellNeighbors()
{
   asn1SccCellIndex index;
   asn1SccMooreNeighborList neighbors;
   asn1SccGridCellNeighborList right_temp_148;
   asn1SccGridCellNeighborList memcpy_temp_148;
   asn1SccUint memcpy_counter_148 = 0;
   // gridNeighbors := {} (237,17)
   ctxt.gridneighbors = (asn1SccGridCellNeighborList) asn1SccGridCellNeighborList_constant;
   // index := 0 (240,17)
   index = (asn1SccCellIndex) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (243,12)
   loop:
   // neighbors := {} (246,17)
   neighbors = (asn1SccMooreNeighborList) asn1SccMooreNeighborList_constant;
   // calculateCellMooreNeighbors(index, neighbors) (249,17)
   _0_core_calculateCellMooreNeighbors(index, &neighbors);
   // gridNeighbors := gridNeighbors // {neighbors} (252,17)
   {
      right_temp_148 =(asn1SccGridCellNeighborList) {1, {neighbors}};
      memcpy_temp_148 = ctxt.gridneighbors;
      for(memcpy_counter_148 = 0; memcpy_counter_148 < 1; memcpy_counter_148++)
      {
         memcpy_temp_148.arr[memcpy_temp_148.nCount + memcpy_counter_148] = right_temp_148.arr[memcpy_counter_148];
      }
      memcpy_temp_148.nCount += 1;
   }
   ctxt.gridneighbors = (asn1SccGridCellNeighborList) memcpy_temp_148;
   // index := index + 1 (255,17)
   index = (asn1SccCellIndex) (index + 1);
   // DECISION index < length (coreGrid.cells) (258,27)
   // ANSWER false (261,17)
   if((((index < ctxt.coregrid.cells.nCount)) == false))
   {
      // RETURN  (None,None) at 132427199030080, 631
      return;
      // ANSWER true (267,17)
   } else if((((index < ctxt.coregrid.cells.nCount)) == true))
   {
      // JOIN loop (270,25) at 132427199114560, 631
      goto loop;
   } //last
}


void _0_core_addInitialEvents()
{
   asn1SccCellIndex index;
   asn1SccFlowingWaterEvent newevent;
   asn1SccFlowingWaterEventList right_temp_149;
   asn1SccFlowingWaterEventList memcpy_temp_150;
   asn1SccUint memcpy_counter_149 = 0;
   asn1SccFlowingWaterEventList memcpy_temp_149;
   asn1SccFlowingWaterEventList right_temp_150;
   asn1SccUint memcpy_counter_150 = 0;
   // index := 0 (288,17)
   index = (asn1SccCellIndex) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (291,12)
   loop:
   // DECISION coreGrid.cells(index).cellType (-1,-1)
   // ANSWER normal (297,17)
   if(((ctxt.coregrid.cells.arr[index].cellType) == asn1SccCellType_normal))
   {
      // DECISION coreGrid.cells(index).waterDepth (-1,-1)
      // ANSWER >0 (303,25)
      if(((ctxt.coregrid.cells.arr[index].waterDepth) > 0))
      {
         // newEvent := {targetCell index} (306,33)
         newevent = (asn1SccFlowingWaterEvent) {.targetCell = index};
         // flowingWaterEventList := flowingWaterEventList // {newEvent} (309,33)
         {
            right_temp_149 =(asn1SccFlowingWaterEventList) {1, {newevent}};
            memcpy_temp_149 = ctxt.flowingwatereventlist;
            for(memcpy_counter_149 = 0; memcpy_counter_149 < 1; memcpy_counter_149++)
            {
               memcpy_temp_149.arr[memcpy_temp_149.nCount + memcpy_counter_149] = right_temp_149.arr[memcpy_counter_149];
            }
            memcpy_temp_149.nCount += 1;
         }
         ctxt.flowingwatereventlist = (asn1SccFlowingWaterEventList) memcpy_temp_149;
         // ANSWER ELSE (None,None)
      } // 1
      else
      {
         // nothing done
      } //last
      // ANSWER source (316,17)
   } else if(((ctxt.coregrid.cells.arr[index].cellType) == asn1SccCellType_source))
   {
      // newEvent := {targetCell index} (319,25)
      newevent = (asn1SccFlowingWaterEvent) {.targetCell = index};
      // flowingWaterEventList := flowingWaterEventList // {newEvent} (322,25)
      {
         right_temp_150 =(asn1SccFlowingWaterEventList) {1, {newevent}};
         memcpy_temp_150 = ctxt.flowingwatereventlist;
         for(memcpy_counter_150 = 0; memcpy_counter_150 < 1; memcpy_counter_150++)
         {
            memcpy_temp_150.arr[memcpy_temp_150.nCount + memcpy_counter_150] = right_temp_150.arr[memcpy_counter_150];
         }
         memcpy_temp_150.nCount += 1;
      }
      ctxt.flowingwatereventlist = (asn1SccFlowingWaterEventList) memcpy_temp_150;
      // ANSWER ELSE (None,None)
   } // 1
   else
   {
      // nothing done
   } //last
   // index := index +1 (329,17)
   index = (asn1SccCellIndex) (index + 1);
   // DECISION index < length (coreGrid.cells) (332,27)
   // ANSWER false (335,17)
   if((((index < ctxt.coregrid.cells.nCount)) == false))
   {
      // RETURN  (None,None) at 132427196860480, 722
      return;
      // ANSWER true (341,17)
   } else if((((index < ctxt.coregrid.cells.nCount)) == true))
   {
      // JOIN loop (344,25) at 132427196885184, 722
      goto loop;
   } //last
}


void _0_core_applyModifications(asn1SccGridModificationList modifications)
{
   asn1SccT_UInt32 index;
   asn1SccCellState cell;
   // index := 0 (365,17)
   index = (asn1SccT_UInt32) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (368,12)
   loop:
   // cell := coreGrid.cells(modifications(index).targetCell) (371,17)
   cell = (asn1SccCellState) ctxt.coregrid.cells.arr[modifications.arr[index].targetCell];
   // DECISION modifications(index).increment (-1,-1)
   // ANSWER true (377,17)
   if(((modifications.arr[index].increment) == true))
   {
      // cell.waterDepth := cell.waterDepth + modifications(index).delta (380,25)
      cell.waterDepth = (asn1SccT_UInt32) (cell.waterDepth + modifications.arr[index].delta);
      // ANSWER false (383,17)
   } else if(((modifications.arr[index].increment) == false))
   {
      // cell.waterDepth := cell.waterDepth - modifications(index).delta (386,25)
      cell.waterDepth = (asn1SccT_UInt32) (cell.waterDepth - modifications.arr[index].delta);
   } //last
   // index := index + 1 (390,17)
   index = (asn1SccT_UInt32) (index + 1);
   // DECISION index < length (modifications) (393,27)
   // ANSWER false (396,17)
   if((((index < modifications.nCount)) == false))
   {
      // RETURN  (None,None) at 132427197135360, 644
      return;
      // ANSWER true (402,17)
   } else if((((index < modifications.nCount)) == true))
   {
      // JOIN loop (405,25) at 132427197179392, 644
      goto loop;
   } //last
}


void _0_core_runSimulationStep()
{
   asn1SccFlowingWaterEventList nextflowingwatereventlist;
   asn1SccGridModificationList modificationlist;
   asn1SccCellIndex index;
   int memcpy_counter_151 = 0;
   asn1SccGridModificationList memcpy_temp_151;
   // modificationList := {} (424,17)
   modificationlist = (asn1SccGridModificationList) asn1SccGridModificationList_constant;
   // nextFlowingWaterEventList := {} (427,17)
   nextflowingwatereventlist = (asn1SccFlowingWaterEventList) asn1SccFlowingWaterEventList_constant;
   // index := 0 (430,17)
   index = (asn1SccCellIndex) 0;
   // JOIN loop (None,None) at None, None
   goto loop;
   // CONNECTION loop (433,12)
   loop:
   // updateCell(
   //     flowingWaterEventList(index).targetCell, 
   //     modificationList, 
   //     nextFlowingWaterEventList
   // ) (436,17)
   _0_core_updateCell(ctxt.flowingwatereventlist.arr[index].targetCell, &modificationlist, &nextflowingwatereventlist);
   // index := index + 1 (443,17)
   index = (asn1SccCellIndex) (index + 1);
   // DECISION index < length (flowingWaterEventList) (446,27)
   // ANSWER true (449,17)
   if((((index < ctxt.flowingwatereventlist.nCount)) == true))
   {
      // JOIN loop (452,25) at 132427197212352, 609
      goto loop;
      // ANSWER false (455,17)
   } else if((((index < ctxt.flowingwatereventlist.nCount)) == false))
   {
      // accumModList := accumModList // modificationList (458,25)
      {
         memcpy_temp_151 = ctxt.accummodlist;
         for(memcpy_counter_151 = 0; memcpy_counter_151 < modificationlist.nCount; memcpy_counter_151++)
         {
            memcpy_temp_151.arr[memcpy_temp_151.nCount + memcpy_counter_151] = modificationlist.arr[memcpy_counter_151];
         }
         memcpy_temp_151.nCount += modificationlist.nCount;
      }
      ctxt.accummodlist = (asn1SccGridModificationList) memcpy_temp_151;
      // flowingWaterEventList := nextFlowingWaterEventList (461,25)
      ctxt.flowingwatereventlist = (asn1SccFlowingWaterEventList) nextflowingwatereventlist;
      // applyModifications(modificationList) (464,25)
      _0_core_applyModifications(modificationlist);
      // COMMENT Advance the counter (470,20)
      // currentTimeStep := currentTimeStep + 1 (467,25)
      ctxt.currenttimestep = (asn1SccTimeStep) (ctxt.currenttimestep + 1);
      // RETURN  (None,None) at 132427199913088, 829
      return;
   } //last
}


void core_PI_startsimulationfromgis(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod)
{
   asn1SccFilePath folder = *in__folder;
   asn1SccDuration timestepduration = *in__timestepduration;
   asn1SccDuration totalduration = *in__totalduration;
   asn1SccTimeStep snapshotperiod = *in__snapshotperiod;
   asn1SccFilePath tmp24325;
   // get_sender(sender) (1,5)
   core_RI_get_sender(&ctxt.sender);
   // currentTimeStep := 0 (494,17)
   ctxt.currenttimestep = (asn1SccTimeStep) 0;
   // totalTimeSteps := totalDuration / timeStepDuration (497,17)
   ctxt.totaltimesteps = (asn1SccTimeStep) (totalduration / timestepduration);
   // coreSnapshotPeriod := snapshotPeriod (500,17)
   ctxt.coresnapshotperiod = (asn1SccTimeStep) snapshotperiod;
   // nextSnapshotTimeStep := coreSnapshotPeriod (503,17)
   ctxt.nextsnapshottimestep = (asn1SccTimeStep) ctxt.coresnapshotperiod;
   // flowingWaterEventList := {} (506,17)
   ctxt.flowingwatereventlist = (asn1SccFlowingWaterEventList) asn1SccFlowingWaterEventList_constant;
   // accumModList := {} (509,17)
   ctxt.accummodlist = (asn1SccGridModificationList) asn1SccGridModificationList_constant;
   // loadGISData(folder, coreGrid) (512,17)
   tmp24325 = (asn1SccFilePath) folder;
   core_RI_loadGISData(&tmp24325, &ctxt.coregrid);
   // displayInitialGrid(coreGrid) (515,19)
   core_RI_displayInitialGrid(&ctxt.coregrid);
   // addInitialEvents (518,17)
   _0_core_addInitialEvents();
   // calculateGridCellNeighbors (521,17)
   _0_core_calculateGridCellNeighbors();
   // core_startSimulationFromGIS_transition (None,None)
   core_startsimulationfromgis_transition();
   // RETURN  (None,None) at 132427196984576, 653
   return;
}


// Required To Work With TASTE's Wrappers
void core_PI_startSimulationFromGIS(asn1SccFilePath *in__folder, asn1SccDuration *in__timestepduration, asn1SccDuration *in__totalduration, asn1SccTimeStep *in__snapshotperiod)
{
   core_PI_startsimulationfromgis(in__folder, in__timestepduration, in__totalduration, in__snapshotperiod);
}

void _0_core_calculateCellMooreNeighbors(asn1SccCellIndex index, asn1SccMooreNeighborList *neighbors)
{
   // RETURN  (None,None) at 132427010213952, 188
   return;
}


void _0_core_updateCell(asn1SccCellIndex cellindex, asn1SccGridModificationList *modificationlist, asn1SccFlowingWaterEventList *nextflowingwatereventlist)
{
   asn1SccMooreNeighborIndex neighborindex;
   asn1SccMooreNeighborList neighbors;
   asn1SccOutflow outflow;
   asn1SccMooreNeighborIndex outflowindex;
   asn1SccOutflowList celloutflowlist;
   asn1SccT_UInt32 totaloutflowdepth;
   asn1SccOutflowList normalizecelloutflowlist;
   asn1SccT_UInt32 normalizetotaloutflowdepth;
   asn1SccT_UInt64 selfcellheight;
   asn1SccFrictionCoefficient frictioncoefficient;
   asn1SccOutflowList memcpy_temp_152;
   asn1SccUint memcpy_counter_152 = 0;
   asn1SccOutflowList right_temp_152;
   // DECISION coreGrid.cells(cellIndex).cellType (-1,-1)
   // ANSWER source (581,17)
   if(((ctxt.coregrid.cells.arr[cellindex].cellType) == asn1SccCellType_source))
   {
      // addFlowingWaterEvent(
      //     cellIndex, 
      //     nextFlowingWaterEventList
      // ) (584,25)
      _0_core_addFlowingWaterEvent(cellindex, &(*nextflowingwatereventlist));
      // addGridModification(
      //     cellIndex,
      //     true,
      //     coreGrid.cells(cellIndex).inflowPerTimeStep,
      //     modificationList
      // ) (590,25)
      _0_core_addGridModification(cellindex, true, ctxt.coregrid.cells.arr[cellindex].inflowPerTimeStep, &(*modificationlist));
      // ANSWER ELSE (None,None)
   } // 1
   else
   {
      // nothing done
   } //last
   // neighbors := gridNeighbors(cellIndex) (602,17)
   neighbors = (asn1SccMooreNeighborList) ctxt.gridneighbors.arr[cellindex];
   // neighborIndex := 0 (605,17)
   neighborindex = (asn1SccMooreNeighborIndex) 0;
   // cellOutFlowList := {} (608,17)
   celloutflowlist = (asn1SccOutflowList) asn1SccOutflowList_constant;
   // totalOutflowDepth := 0 (611,17)
   totaloutflowdepth = (asn1SccT_UInt32) 0;
   // selfCellHeight := coreGrid.cells(cellIndex).elevation + coreGrid.cells(cellIndex).waterDepth (614,17)
   selfcellheight = (asn1SccT_UInt64) (ctxt.coregrid.cells.arr[cellindex].elevation + ctxt.coregrid.cells.arr[cellindex].waterDepth);
   // getFrictionCoefficient(coreGrid.cells(cellIndex).surfaceType, frictionCoefficient) (617,17)
   _0_core_getFrictionCoefficient(ctxt.coregrid.cells.arr[cellindex].surfaceType, &frictioncoefficient);
   // JOIN loopComputeWaterFlow (None,None) at None, None
   goto loopcomputewaterflow;
   // CONNECTION loopAddGridModifications (704,28)
   loopaddgridmodifications:
   // DECISION coreGrid.cells(cellOutflowList(outflowIndex).neighbor).cellType (-1,-1)
   // ANSWER sink (710,33)
   if(((ctxt.coregrid.cells.arr[celloutflowlist.arr[outflowindex].neighbor].cellType) == asn1SccCellType_sink))
   {
      ;
      // ANSWER ELSE (None,None)
   } // 1
   else
   {
      // addFlowingWaterEvent(
      //     cellOutflowList(outflowIndex).neighbor, 
      //     nextFlowingWaterEventList
      // ) (716,41)
      _0_core_addFlowingWaterEvent(celloutflowlist.arr[outflowindex].neighbor, &(*nextflowingwatereventlist));
      // addGridModification(
      //     cellOutflowList(outflowIndex).neighbor,
      //     true,
      //     cellOutflowList(outflowIndex).depth,
      //     modificationList
      // ) (722,41)
      _0_core_addGridModification(celloutflowlist.arr[outflowindex].neighbor, true, celloutflowlist.arr[outflowindex].depth, &(*modificationlist));
   } //last
   // outflowIndex := outflowIndex + 1 (731,33)
   outflowindex = (asn1SccMooreNeighborIndex) (outflowindex + 1);
   // DECISION outflowIndex < length (cellOutflowList) (734,50)
   // ANSWER false (737,33)
   if((((outflowindex < celloutflowlist.nCount)) == false))
   {
      // RETURN  (None,None) at 132427010965888, 2607
      return;
      // ANSWER true (743,33)
   } else if((((outflowindex < celloutflowlist.nCount)) == true))
   {
      // JOIN loopAddGridModifications (746,41) at 132427011024960, 2607
      goto loopaddgridmodifications;
   } //last
   // CONNECTION loopComputeWaterFlow (620,12)
   loopcomputewaterflow:
   // computeOutflow(
   //     selfCellHeight, 
   //     frictionCoefficient,
   //     neighbors(neighborIndex), 
   //     outflow
   // ) (623,17)
   _0_core_computeOutflow(selfcellheight, frictioncoefficient, neighbors.arr[neighborindex], outflow);
   // DECISION outflow.depth (-1,-1)
   // ANSWER >0 (634,17)
   if(((outflow.depth) > 0))
   {
      // cellOutflowList := cellOutflowList // {outflow} (637,25)
      {
         right_temp_152 =(asn1SccOutflowList) {1, {outflow}};
         memcpy_temp_152 = celloutflowlist;
         for(memcpy_counter_152 = 0; memcpy_counter_152 < 1; memcpy_counter_152++)
         {
            memcpy_temp_152.arr[memcpy_temp_152.nCount + memcpy_counter_152] = right_temp_152.arr[memcpy_counter_152];
         }
         memcpy_temp_152.nCount += 1;
      }
      celloutflowlist = (asn1SccOutflowList) memcpy_temp_152;
      // totalOutflowDepth := totalOutflowDepth + outflow.depth (640,25)
      totaloutflowdepth = (asn1SccT_UInt32) (totaloutflowdepth + outflow.depth);
      // ANSWER ELSE (None,None)
   } // 1
   else
   {
      // nothing done
   } //last
   // neighborIndex := neighborIndex + 1 (647,17)
   neighborindex = (asn1SccMooreNeighborIndex) (neighborindex + 1);
   // DECISION neighborIndex < length (neighbors) (650,35)
   // ANSWER true (653,17)
   if((((neighborindex < neighbors.nCount)) == true))
   {
      // JOIN loopComputeWaterFlow (656,25) at 132427010603136, 1428
      goto loopcomputewaterflow;
      // ANSWER false (659,17)
   } else if((((neighborindex < neighbors.nCount)) == false))
   {
      // DECISION length (cellOutflowList) (-1,-1)
      // ANSWER >0 (665,25)
      if(((celloutflowlist.nCount) > 0))
      {
         // normalizeOutflows(
         //     coreGrid.cells(cellIndex).waterDepth, 
         //     totalOutflowDepth, 
         //     cellOutflowList,  
         //     normalizeTotalOutflowDepth,
         //     normalizeCellOutflowList
         // ) (668,33)
         _0_core_normalizeOutflows(ctxt.coregrid.cells.arr[cellindex].waterDepth, totaloutflowdepth, celloutflowlist, &normalizetotaloutflowdepth, &normalizecelloutflowlist);
         // DECISION coreGrid.cells(cellIndex).cellType (-1,-1)
         // ANSWER source (680,33)
         if(((ctxt.coregrid.cells.arr[cellindex].cellType) == asn1SccCellType_source))
         {
            ;
            // ANSWER ELSE (None,None)
         } // 1
         else
         {
            // addFlowingWaterEvent(
            //     cellIndex, 
            //     nextFlowingWaterEventList
            // ) (686,41)
            _0_core_addFlowingWaterEvent(cellindex, &(*nextflowingwatereventlist));
         } //last
         // addGridModification(
         //     cellIndex,
         //     false,
         //     normalizeTotalOutflowDepth,
         //     modificationList
         // ) (693,33)
         _0_core_addGridModification(cellindex, false, normalizetotaloutflowdepth, &(*modificationlist));
         // outflowIndex := 0 (701,33)
         outflowindex = (asn1SccMooreNeighborIndex) 0;
         // JOIN loopAddGridModifications (None,None) at None, None
         goto loopaddgridmodifications;
         // ANSWER ELSE (None,None)
      } // 1
      else
      {
         // RETURN  (None,None) at 132427011066240, 1541
         return;
      } //last
   } //last
}


void _0_core_getFrictionCoefficient(asn1SccSurfaceType surfacetype, asn1SccFrictionCoefficient *frictioncoefficient)
{
   // DECISION surfaceType (-1,-1)
   // ANSWER waterSurface (776,17)
   if(((surfacetype) == asn1SccSurfaceType_waterSurface))
   {
      // frictionCoefficient := 0.2 (779,25)
      (*frictioncoefficient) = (asn1SccFrictionCoefficient) 0.2;
      // ANSWER rock (782,17)
   } else if(((surfacetype) == asn1SccSurfaceType_rock))
   {
      // frictionCoefficient := 0.2 (785,25)
      (*frictioncoefficient) = (asn1SccFrictionCoefficient) 0.2;
      // ANSWER asphalt (788,17)
   } else if(((surfacetype) == asn1SccSurfaceType_asphalt))
   {
      // frictionCoefficient := 0.2 (791,25)
      (*frictioncoefficient) = (asn1SccFrictionCoefficient) 0.2;
      // ANSWER forest (794,17)
   } else if(((surfacetype) == asn1SccSurfaceType_forest))
   {
      // frictionCoefficient := 0.2 (797,25)
      (*frictioncoefficient) = (asn1SccFrictionCoefficient) 0.2;
      // ANSWER soil (800,17)
   } else if(((surfacetype) == asn1SccSurfaceType_soil))
   {
      // frictionCoefficient := 0.2 (803,25)
      (*frictioncoefficient) = (asn1SccFrictionCoefficient) 0.2;
   } //last
   // RETURN  (None,None) at 132427010798848, 303
   return;
}


void _0_core_addFlowingWaterEvent(asn1SccCellIndex targetcell, asn1SccFlowingWaterEventList *eventlist)
{
   asn1SccFlowingWaterEvent event;
   asn1SccUint memcpy_counter_153 = 0;
   asn1SccFlowingWaterEventList right_temp_153;
   asn1SccFlowingWaterEventList memcpy_temp_153;
   // event := {targetCell targetCell} (827,17)
   event = (asn1SccFlowingWaterEvent) {.targetCell = targetcell};
   // eventList := eventList // {event} (830,17)
   {
      right_temp_153 =(asn1SccFlowingWaterEventList) {1, {event}};
      memcpy_temp_153 = (*eventlist);
      for(memcpy_counter_153 = 0; memcpy_counter_153 < 1; memcpy_counter_153++)
      {
         memcpy_temp_153.arr[memcpy_temp_153.nCount + memcpy_counter_153] = right_temp_153.arr[memcpy_counter_153];
      }
      memcpy_temp_153.nCount += 1;
   }
   (*eventlist) = (asn1SccFlowingWaterEventList) memcpy_temp_153;
   // RETURN  (None,None) at 132427010337792, 260
   return;
}


void _0_core_addGridModification(asn1SccCellIndex targetcell, asn1SccT_Boolean increment, asn1SccT_UInt32 delta, asn1SccGridModificationList *modificationlist)
{
   asn1SccGridModification modification;
   asn1SccGridModificationList memcpy_temp_154;
   asn1SccGridModificationList right_temp_154;
   asn1SccUint memcpy_counter_154 = 0;
   // modification := {
   //   targetCell targetCell,
   //   increment increment,
   //   delta delta
   // } (855,17)
   modification = (asn1SccGridModification) {.targetCell = targetcell, .increment = increment, .delta = delta};
   // modificationList := modificationList // {modification} (862,17)
   {
      right_temp_154 =(asn1SccGridModificationList) {1, {modification}};
      memcpy_temp_154 = (*modificationlist);
      for(memcpy_counter_154 = 0; memcpy_counter_154 < 1; memcpy_counter_154++)
      {
         memcpy_temp_154.arr[memcpy_temp_154.nCount + memcpy_counter_154] = right_temp_154.arr[memcpy_counter_154];
      }
      memcpy_temp_154.nCount += 1;
   }
   (*modificationlist) = (asn1SccGridModificationList) memcpy_temp_154;
   // RETURN  (None,None) at 132427008129600, 397
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
            // NEXT_STATE Idle (872,18) at 132427199821504, 150
            trId = -1;
            ctxt.state = asn1SccCore_States_idle;
            goto continuous_signals;
            break;
         }
         case 1:
         {
            // runSimulationStep (882,17)
            _0_core_runSimulationStep();
            // finished := currentTimeStep >= totalTimeSteps (885,17)
            ctxt.finished = (asn1SccT_Boolean) (ctxt.currenttimestep >= ctxt.totaltimesteps);
            // DECISION currentTimeStep >= nextSnapshotTimeStep (888,37)
            // ANSWER false (891,17)
            if((((ctxt.currenttimestep >= ctxt.nextsnapshottimestep)) == false))
            {
               ;
               // ANSWER true (894,17)
            } else if((((ctxt.currenttimestep >= ctxt.nextsnapshottimestep)) == true))
            {
               // displaySnapShot(accumModList, finished) (897,27)
               core_RI_displaySnapshot(&ctxt.accummodlist, &ctxt.finished);
               // nextSnapshotTimeStep := nextSnapshotTimeStep + coreSnapshotPeriod (900,25)
               ctxt.nextsnapshottimestep = (asn1SccTimeStep) (ctxt.nextsnapshottimestep + ctxt.coresnapshotperiod);
            } //last
            // DECISION finished (-1,-1)
            // ANSWER false (907,17)
            if(((ctxt.finished) == false))
            {
               // NEXT_STATE Running (910,30) at 132427198267264, 807
               trId = -1;
               ctxt.state = asn1SccCore_States_running;
               goto continuous_signals;
               // ANSWER true (913,17)
            } else if(((ctxt.finished) == true))
            {
               // NEXT_STATE Idle (916,30) at 132427198295488, 807
               trId = -1;
               ctxt.state = asn1SccCore_States_idle;
               goto continuous_signals;
            } //last
            break;
         }
         case 2:
         {
            // NEXT_STATE Running (928,22) at 132427200143744, 260
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