let currentSrc = null;

export function initViewer() {
  // nothing to set up — iframe starts with about:blank
}

export function updateViewer(playerUrl) {
  if (!playerUrl || playerUrl === currentSrc) return;
  currentSrc = playerUrl;
  const iframe = document.getElementById('viewerFrame');
  if (iframe) iframe.src = playerUrl;
}
