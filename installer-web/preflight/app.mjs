import {parseFlashId,verifyFlashEvidence} from './flash-id-policy.mjs';
import {importOfficialLibrary,createLoader,isSupportedChip,safeError} from '../backup/esptool-adapter.mjs';
const $=id=>document.getElementById(id);
const EXPECTED={ 'bootloader.bin':{offset:0x1000},'partitions.bin':{offset:0x8000},'firmware.bin':{offset:0x10000}};
let session=null,passed=false;
let romLines=[];
const log=s=>{$('log').textContent+=String(s)+'\n';};
const hex=buffer=>Array.from(new Uint8Array(buffer)).map(v=>v.toString(16).padStart(2,'0')).join('');
export async function verifyArtifacts(meta,fetcher=fetch,hasher=b=>crypto.subtle.digest('SHA-256',b)){
  if(meta.write_enabled!==false||meta.flash_size_bytes!==4194304||meta.artifacts?.length!==3)throw Error('Release metadata is not an approved read-only candidate');
  const verified=[];
  for(const [name,config] of Object.entries(EXPECTED)){
    const a=meta.artifacts.find(x=>x.file===name);
    if(!a||Number.parseInt(a.offset,16)!==config.offset||!Number.isSafeInteger(a.size_bytes)||a.size_bytes<=0||config.offset+a.size_bytes>4194304)throw Error('Unexpected image layout: '+name);
    const response=await fetcher('../release/'+name,{cache:'no-store'});
    if(!response.ok)throw Error('Missing '+name);
    const data=await response.arrayBuffer();
    if(data.byteLength!==a.size_bytes||hex(await hasher(data))!==a.sha256)throw Error('Checksum mismatch: '+name);
    verified.push(name);
  }
  const partition=meta.partitions?.find(p=>p.label==='app0');
  const fw=meta.artifacts.find(a=>a.file==='firmware.bin');
  if(!partition||partition.offset!==0x10000||fw.size_bytes>partition.size)throw Error('Firmware does not fit app0');
  return verified;
}
async function disconnect(){if(session){const s=session;session=null;await s.transport.disconnect();}}
$('run').onclick=async()=>{
  $('run').disabled=true;passed=false;romLines=[];$('log').textContent='';$('result').textContent='NOT APPROVED FOR FLASHING';
  try{
    log('Checking local release artifacts...');
    const response=await fetch('../release/release-metadata.json',{cache:'no-store'});
    if(!response.ok)throw Error('Missing metadata');
    const names=await verifyArtifacts(await response.json());
    log('PASS: '+names.join(', '));
    log('Select USB serial device for identification.');
    const library=await importOfficialLibrary();
    session=await createLoader({library,terminal:{clean(){},writeLine(s){romLines.push(String(s));log('ROM: '+s)},write(){} }});
    const chip=await session.loader.main();
    if(!isSupportedChip(chip))throw Error('Unsupported chip: '+chip);
    log('PASS: original ESP32 detected ('+chip+')');
    // Flash ID is only read for capacity verification; no readback, write, or erase.
    if(typeof session.loader.flashId!=='function')throw Error('Flash ID method unavailable in pinned library');
    const flashId=await session.loader.flashId();
    const evidence=verifyFlashEvidence(flashId,romLines);
    log('PASS: 4 MB flash capacity confirmed by ROM JEDEC evidence (0x'+evidence.id.toString(16)+')');
    passed=true;
    $('result').textContent='PREFLIGHT PASSED — NO FLASH WRITE PERFORMED';
    log('All preflight checks passed. Installation is intentionally disabled in this build.');
  }catch(e){log('FAIL: '+safeError(e));$('result').textContent='PREFLIGHT FAILED — NO FLASH WRITE PERFORMED';}
  finally{try{await disconnect()}catch(e){log('Disconnect warning: '+safeError(e))}$('run').disabled=false;}
};
window.addEventListener('pagehide',()=>{void disconnect().catch(()=>{})});
