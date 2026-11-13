import {createLoader,isSupportedChip,importOfficialLibrary,safeError} from './esptool-adapter.mjs';
const $ = id => document.getElementById(id);
let session;
let loadingPromise;
const terminal = {
  clean(){},
  writeLine(message){ $('official-status').textContent=String(message).slice(0,300); },
  write(){}
};
function loadOfficialLibrary() {
  if (!loadingPromise) loadingPromise=importOfficialLibrary().finally(()=>{loadingPromise=undefined;});
  return loadingPromise;
}
async function disconnect() {
  const current=session;session=undefined;
  $('official-disconnect').disabled=true;
  if(current) await current.transport.disconnect();
}
$('official-identify').addEventListener('click',async()=>{
  if(session){$('official-status').textContent='Disconnect the existing session first.';return;}
  $('official-identify').disabled=true;
  $('official-status').textContent='Loading official Espressif library…';
  try {
    const library=await loadOfficialLibrary();
    $('official-status').textContent='Select the device serial port…';
    session=await createLoader({library,terminal});
    $('official-disconnect').disabled=false;
    $('official-status').textContent='Connecting to ESP32 ROM (device may reset)…';
    const chip=await session.loader.main();
    if (typeof chip !== 'string' || !chip.trim()) throw new Error('Device identity unavailable; no operation permitted');
    $('official-status').textContent=isSupportedChip(chip)
      ? `Detected ${chip}. Identification complete. Backup and flashing remain disabled.`
      : `Detected ${chip}. Unsupported or unverified hardware. Backup and flashing blocked.`;
  } catch(e) {
    $('official-status').textContent=`Identification failed: ${safeError(e)}`;
    try{await disconnect()}catch{}
  } finally {$('official-identify').disabled=false;}
});
$('official-disconnect').addEventListener('click',async()=>{
  try{await disconnect();$('official-status').textContent='Disconnected.';}
  catch(e){$('official-status').textContent=`Disconnect failed: ${safeError(e)}`;}
});
window.addEventListener('pagehide',()=>{if(session){void disconnect().catch(()=>{});}});
