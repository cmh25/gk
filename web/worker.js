// gk runs here, off the main thread. A runaway evaluation (e.g. while[1;1], now a
// genuine infinite loop) freezes only THIS worker — the page stays responsive and
// can terminate+restart us to stop it. gk.js auto-detects the worker environment
// (synchronous XHR to fetch gk.wasm), so the same build runs here unchanged.
importScripts('gk.js');

let Module = null;
let ready = false;

function post(type, s) { postMessage({ type, s }); }

onmessage = e => {
  const msg = e.data;
  if (msg.cmd === 'boot') {
    // showBanner: print the startup banner on the very first boot, but not when the
    // page restarts us after a stop (output before `ready` is the banner).
    const showBanner = msg.showBanner;
    GK({
      print:    s => { if (ready || showBanner) post('out', s); },
      printErr: s => { if (ready || showBanner) post('err', s); },
    }).then(m => {
      Module = m;
      Module.ccall('gk_init', 'void', [], []);   // prints the banner (when not suppressed)
      ready = true;
      post('ready');
    });
  } else if (msg.cmd === 'eval') {
    if (!ready) return;
    try {
      Module.ccall('gk_eval', 'void', ['string'], [msg.src]);
      post('done');
    } catch (err) {
      // gk hit a fatal exit() — e.g. `wsfull` (the OOM message printed just before).
      // The wasm runtime is now dead, so it can't keep going; tell the page to discard
      // this worker and boot a fresh one. (The 'wsfull' line already reached the page
      // via the print sink before exit unwound.)
      ready = false;
      post('exited');
    }
  }
};
