'use strict';

(function () {
  const scenario  = (window.DANASIM_CONFIG ?? {}).scenario ?? 'default';

  const sceneEl   = document.getElementById('x3d-scene');
  const phaseEl   = document.getElementById('status-phase');
  const timeEl    = document.getElementById('status-time');
  const cellsEl   = document.getElementById('status-cells');
  const overlayEl = document.getElementById('overlay');

  const manager = new window.X3DSceneManager(sceneEl);

  const connection = new signalR.HubConnectionBuilder()
    .withUrl('/simulationHub')
    .withAutomaticReconnect()
    .build();

  connection.on('InitialState', (msg) => {
    manager.buildInitialScene(msg);
    phaseEl.textContent  = 'Running';
    timeEl.textContent   = msg.simulationTime ? `sim time: ${msg.simulationTime}` : '';
    cellsEl.textContent  = '';
  });

  connection.on('FrameUpdate', (msg) => {
    manager.applyFrameUpdate(msg);
    timeEl.textContent  = msg.simulationTime ? `sim time: ${msg.simulationTime}` : '';
    cellsEl.textContent = `changed: ${msg.changes.length}`;
  });

  connection.on('SimulationEnded', () => {
    phaseEl.textContent       = 'Ended';
    overlayEl.style.display   = 'flex';
  });

  connection.onreconnecting(() => { phaseEl.textContent = 'Reconnecting…'; });
  connection.onreconnected(() =>   { phaseEl.textContent = 'Running'; });
  connection.onclose(() =>         { phaseEl.textContent = 'Disconnected'; });

  async function start() {
    phaseEl.textContent = 'Connecting…';
    try {
      await connection.start();
      await connection.invoke('JoinSimulation', scenario);
      phaseEl.textContent = 'Connected — waiting for simulation…';
    } catch (err) {
      console.error('SignalR connect error:', err);
      phaseEl.textContent = 'Connection error — retrying…';
      setTimeout(start, 5000);
    }
  }

  start();
})();
