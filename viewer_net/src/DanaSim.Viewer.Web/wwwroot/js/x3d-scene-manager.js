'use strict';

// Flood state colour palette — matches python/visualizer/renderers/x3d/colors.py
// Index = FloodState enum value sent by the server (0-7)
const FLOOD_PALETTE = [
  [0.62, 0.50, 0.25],  // 0 Dry
  [0.60, 0.90, 1.00],  // 1 Risk1 — very shallow
  [0.20, 0.60, 0.90],  // 2 Risk2 — low depth
  [0.10, 0.40, 0.80],  // 3 Risk3 — medium depth
  [0.05, 0.20, 0.70],  // 4 Risk4 — high depth
  [0.10, 0.10, 0.60],  // 5 Risk5 — extreme depth
  [0.40, 0.40, 0.40],  // 6 Obstacle
  [0.70, 0.10, 0.10],  // 7 ObstacleDestroyed
];

/**
 * Manages the X3D scene built from simulation data received via SignalR.
 *
 * Rendering strategy:
 *   - ElevationGrid: terrain heights, one quad per cell.
 *   - Color (colorPerVertex=false): one colour per face mapped to flood state.
 *   - Frame updates only mutate the Float32Array and schedule a single
 *     requestAnimationFrame DOM write, keeping updates cheap.
 */
window.X3DSceneManager = class {
  constructor(sceneElement) {
    this._scene = sceneElement;
    this._colorNode = null;
    this._colorArray = null; // Float32Array, length = (rows-1)*(cols-1)*3
    this._rows = 0;
    this._cols = 0;
    this._dirty = false;
    this._rafId = null;
  }

  /**
   * Builds the full X3D scene from the InitialState SignalR message.
   * Called once when Init_EOF is received by the server.
   */
  buildInitialScene(msg) {
    const { rows, cols, cellSizeM } = msg.gridMeta;
    this._rows = rows;
    this._cols = cols;

    const heights = msg.terrain
      ? msg.terrain.join(' ')
      : new Array(rows * cols).fill('0').join(' ');

    // ElevationGrid with colorPerVertex=false has (rows-1)*(cols-1) faces
    const faceCols = cols - 1;
    const faceRows = rows - 1;
    const numFaces = faceRows * faceCols;
    this._colorArray = new Float32Array(numFaces * 3);

    for (let faceRow = 0; faceRow < faceRows; faceRow++) {
      for (let faceCol = 0; faceCol < faceCols; faceCol++) {
        const cellIdx = faceRow * cols + faceCol;
        const state = msg.cells[cellIdx] ?? 0;
        const [r, g, b] = FLOOD_PALETTE[state] ?? FLOOD_PALETTE[0];
        const base = (faceRow * faceCols + faceCol) * 3;
        this._colorArray[base]     = r;
        this._colorArray[base + 1] = g;
        this._colorArray[base + 2] = b;
      }
    }

    const colorStr = Array.from(this._colorArray).join(' ');
    const centerX = (cols * cellSizeM) / 2;
    const centerZ = (rows * cellSizeM) / 2;
    const viewHeight = Math.max(rows, cols) * cellSizeM * 0.9;

    this._scene.innerHTML = `
      <Background skyColor="0.53 0.81 0.98"/>
      <NavigationInfo type='"EXAMINE" "ANY"' speed="50"/>
      <Viewpoint
        DEF="Overview"
        position="${centerX} ${viewHeight} ${centerZ}"
        orientation="1 0 0 -0.7854"
        description="Overview"/>
      <Shape>
        <Appearance>
          <Material ambientIntensity="0.4" shininess="0.05"/>
        </Appearance>
        <ElevationGrid
          xDimension="${cols}" zDimension="${rows}"
          xSpacing="${cellSizeM}" zSpacing="${cellSizeM}"
          height="${heights}"
          colorPerVertex="false"
          solid="false"
          creaseAngle="0.5">
          <Color DEF="FloodColors" color="${colorStr}"/>
        </ElevationGrid>
      </Shape>
    `;

    // Wait one frame for X_ITE to parse and register the new nodes
    requestAnimationFrame(() => {
      this._colorNode = this._scene.querySelector('[DEF="FloodColors"]');
    });
  }

  /**
   * Applies incremental cell changes from a FrameUpdate SignalR message.
   * Mutates the Float32Array in O(changes) and schedules one DOM write via RAF.
   */
  applyFrameUpdate(msg) {
    if (!this._colorArray) return;

    const faceCols = this._cols - 1;
    const faceRows = this._rows - 1;

    for (const { index, state } of msg.changes) {
      const row = Math.floor(index / this._cols);
      const col = index % this._cols;
      if (row >= faceRows || col >= faceCols) continue;

      const face = row * faceCols + col;
      const [r, g, b] = FLOOD_PALETTE[state] ?? FLOOD_PALETTE[0];
      this._colorArray[face * 3]     = r;
      this._colorArray[face * 3 + 1] = g;
      this._colorArray[face * 3 + 2] = b;
    }

    this._dirty = true;
    this._scheduleColorFlush();
  }

  _scheduleColorFlush() {
    if (this._rafId !== null) return; // already scheduled
    this._rafId = requestAnimationFrame(() => {
      this._rafId = null;
      if (!this._dirty || !this._colorNode) return;
      this._colorNode.setAttribute('color', Array.from(this._colorArray).join(' '));
      this._dirty = false;
    });
  }
};
