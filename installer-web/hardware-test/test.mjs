import {createLoader,importOfficialLibrary,isSupportedChip,safeError} from '../backup/esptool-adapter.mjs';
const el=id=>document.getElementById(id);let active=null;let chip='';const events=[];
const log=(s)=>{const message=String(s).slice(0,500);events.push(message);el('log').textContent=events.join('\n');};
const terminal={clean(){},writeLine(s){log('ROM: '+s)},write(){}};
async function loadBundle(){
  try { return await importOfficialLibrary(); }
  catch(error){throw new Error('ES module load failed: '+safeError(error));}
}
async function close(){if(active){const s=active;active=null;try{await s.transport.disconnect()}finally{el('disconnect').disabled=true}}}
el('test').onclick=async()=>{el('test').disabled=true;events.length=0;chip='';try{if(!isSecureContext||!navigator.serial)throw new Error('Web Serial unavailable: use Chrome/Edge and localhost/HTTPS');log('Loading pinned esptool-js library...');const library=await loadBundle();log('Library API found. Select CYD serial port.');active=await createLoader({library,terminal});el('disconnect').disabled=false;log('Connecting to ROM. Device may reset.');chip=await active.loader.main();log('Detected: '+String(chip));log(isSupportedChip(chip)?'Original ESP32 identified; no flash access performed.':'Unsupported/unverified chip. No further operations.')}catch(e){log('ERROR: '+safeError(e));try{await close()}catch(e2){log('Disconnect error: '+safeError(e2))}}finally{el('test').disabled=false}};
el('disconnect').onclick=async()=>{try{await close();log('Disconnected')}catch(e){log('Disconnect error: '+safeError(e))}};
el('export').onclick=()=>{const report={test:'CYD read-only hardware identification',timestamp:new Date().toISOString(),chip:isSupportedChip(chip)?'ESP32':'unknown_or_unsupported',success:isSupportedChip(chip),events:events.filter(s=>!s.startsWith('ROM:'))};const blob=new Blob([JSON.stringify(report,null,2)],{type:'application/json'});const url=URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download='cyd-hardware-test.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000)};
window.addEventListener('pagehide',()=>{void close().catch(()=>{})});