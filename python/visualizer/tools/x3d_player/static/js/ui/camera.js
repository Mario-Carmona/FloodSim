// Camera controls — viewpoint presets, vertical exaggeration slider and
// keyboard shortcuts for camera navigation.
//
// Viewpoint binding works via x_ite's `set_bind` field: setting it to "true"
// on a Viewpoint node makes the browser smoothly transition to that view.
// The ZScale Transform (DEF="ZScale") wraps the entire terrain so we can
// scale Y without touching any geometry data.

/**
 * Bind to one of the named Viewpoint nodes embedded in the scene.
 * @param {string} def  DEF attribute value — "VP_Overview" | "VP_Cenital" | "VP_Lateral"
 */
function _bindViewpoint(def) {
  const vp = document.querySelector(`Viewpoint[DEF="${def}"]`);
  if (vp) vp.setAttribute("set_bind", "true");
}

/**
 * Apply a vertical exaggeration factor to the ZScale Transform.
 * @param {number} factor  1 = real scale, 10 = 10× taller
 */
function _applyZScale(factor) {
  const t = document.querySelector('Transform[DEF="ZScale"]');
  if (t) t.setAttribute("scale", `1 ${factor} 1`);
  const label = document.getElementById("zScaleVal");
  if (label) label.textContent = `${factor}×`;
}

/**
 * Wire up preset buttons, Z-scale slider and keyboard shortcuts.
 * Must be called once after the scene is ready.
 */
export function initCamera() {
  // Preset buttons — each carries data-vp with the target Viewpoint DEF.
  document.querySelectorAll(".cam-btn[data-vp]").forEach(btn => {
    btn.addEventListener("click", () => _bindViewpoint(btn.dataset.vp));
  });

  // Reset button — back to the Overview viewpoint.
  const resetBtn = document.getElementById("camReset");
  if (resetBtn) {
    resetBtn.addEventListener("click", () => _bindViewpoint("VP_Overview"));
  }

  // Z-scale slider.
  const zSlider = document.getElementById("zScale");
  if (zSlider) {
    zSlider.addEventListener("input", () => _applyZScale(parseFloat(zSlider.value)));
  }

  // Keyboard shortcuts (camera only — playback keys live in playback.js).
  document.addEventListener("keydown", e => {
    // Ignore shortcuts when user is typing inside an input/textarea.
    if (e.target.tagName === "INPUT" || e.target.tagName === "TEXTAREA") return;

    switch (e.key.toLowerCase()) {
      case "c": _bindViewpoint("VP_Cenital");  break;
      case "p": _bindViewpoint("VP_Overview"); break;
      case "l": _bindViewpoint("VP_Lateral");  break;
      case "r": _bindViewpoint("VP_Overview"); break;
    }
  });
}
