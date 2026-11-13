import {verifyFlashEvidence} from '../preflight/flash-id-policy.mjs';
import {importOfficialLibrary,createLoader,isSupportedChip,safeError} from '../backup/esptool-adapter.mjs';
import {FLASH_BYTES,PARTITION_BYTES,PARTITION_OFFSET,checkImageLayout,comparePartitionTable,createFlashFileArray,createFullInstallFileArray} from './safety.mjs';
const $=id=>document.getElementById(id);
let session=null,ready=false,writing=false,firmware=null,backupBytes=null,backupSaved=false,backupHash=null,installMode=null,imagesForInstall=null;
let romLines=[];
const log=s=>{$('log').textContent+=String(s)+'\n';};
const hex=b=>Array.from(new Uint8Array(b)).map(x=>x.toString(16).padStart(2,'0')).join('');
async function close(){if(session){const s=session;session=null;await s.transport.disconnect();}}
async function loadArtifacts(){
  const r=await fetch('../release/release-metadata.json',{cache:'no-store'});
  if(!r.ok)throw Error('Release metadata unavailable');const meta=await r.json();checkImageLayout(meta);
  const result={};
  for(const a of meta.artifacts){
    const response=await fetch('../release/'+a.file,{cache:'no-store'});if(!response.ok)throw Error('Missing '+a.file);
    const data=new Uint8Array(await response.arrayBuffer());
    if(data.length!==a.size_bytes||hex(await crypto.subtle.digest('SHA-256',data))!==a.sha256)throw Error('Checksum mismatch: '+a.file);
    result[a.file]=data;
  }
  return result;
}
function reset(){ready=false;firmware=null;backupBytes=null;backupSaved=false;backupHash=null;installMode=null;imagesForInstall=null;$('flash').disabled=true;$('ack').checked=false;$('progress').value=0;}
$('verify').onclick=async()=>{
  reset();romLines=[];$('log').textContent='';$('verify').disabled=true;$('status').textContent='VERIFYING — NO WRITE PERFORMED';
  try{
    log('Checking SHA-256 of all release images...');const images=await loadArtifacts();log('PASS: release images verified');
    const library=await importOfficialLibrary();
    session=await createLoader({library,terminal:{clean(){},writeLine(s){romLines.push(String(s));log('ROM: '+s)},write(){} }});
    const chip=await session.loader.main();if(!isSupportedChip(chip))throw Error('Unsupported chip: '+chip);
    if(typeof session.loader.flashId!=='function')throw Error('Flash ID method unavailable');
    const flashId=await session.loader.flashId();verifyFlashEvidence(flashId,romLines);
    log('PASS: original ESP32 and 4 MB flash verified');
    const file=$('existingBackup').files?.[0];
    if(file){
      if(file.size!==FLASH_BYTES)throw Error('Optional backup must be exactly 4 MB');
      const backup=new Uint8Array(await file.arrayBuffer());
      try{comparePartitionTable(backup,images['partitions.bin']);}finally{backup.fill(0);}
      installMode='application';log('PASS: supplied backup partition table matches release. Application-only install selected.');
    }else{
      installMode='full';
      log('WARNING: No backup supplied. Existing partition table cannot be checked by this browser build.');
      log('FULL INSTALL selected: bootloader at 0x1000, partition table at 0x8000, application at 0x10000.');
      log('Existing firmware and settings may become inaccessible. No full backup will be created.');
    }
    imagesForInstall=images;firmware=images['firmware.bin'];ready=true;$('flash').disabled=false;
    $('status').textContent=installMode==='full'?'READY — FULL INSTALL (NO BACKUP)':'READY — APPLICATION-ONLY INSTALL';
  }catch(e){log('BLOCKED: '+safeError(e));$('status').textContent='BLOCKED — NO WRITE PERFORMED';try{await close()}catch{} }
  finally{$('verify').disabled=false;}
};
$('flash').onclick=async()=>{
  if(!ready||!firmware||!imagesForInstall||!session||writing||!$('ack').checked){log('BLOCKED: verify device and acknowledge risk');return;}
  const full=installMode==='full';
  if(full){
    if(!confirm('NO BACKUP: This FULL install writes bootloader, partition table, and firmware. Existing settings or data may become inaccessible. Continue?'))return;
    if(!confirm('FINAL CONFIRMATION: Proceed WITHOUT a backup and replace THREE flash regions?'))return;
  }else if(!confirm('FINAL CONFIRMATION: Replace application at 0x10000 only?'))return;
  writing=true;ready=false;$('verify').disabled=true;$('flash').disabled=true;$('existingBackup').disabled=true;
  $('status').textContent='FLASHING — DO NOT DISCONNECT USB';
  try{
    const fileArray=full?createFullInstallFileArray(imagesForInstall):createFlashFileArray(firmware);
    log('Writing '+(full?'bootloader, partitions, application':'application only')+'...');
    await session.loader.writeFlash({fileArray,flashSize:'4MB',flashMode:'keep',flashFreq:'keep',eraseAll:false,compress:true,reportProgress:(_file,written,total)=>{if(total>0)$('progress').value=Math.min(100,Math.round(written/total*100));}});
    $('progress').value=100;log('writeFlash completed. Firmware boot and runtime have NOT been verified.');
    $('status').textContent='WRITE COMPLETED — VERIFY DEVICE BOOT';
  }catch(e){log('FLASH ERROR: '+safeError(e));$('status').textContent='FLASH FAILED — USE RECOVERY GUIDE';}
  finally{writing=false;firmware=null;imagesForInstall=null;try{await close()}catch(e){log('Disconnect warning: '+safeError(e))}$('verify').disabled=false;$('existingBackup').disabled=false;}
};
window.addEventListener('pagehide',()=>{if(!writing)void close().catch(()=>{})});

$('restore').onclick=async()=>{
  if(writing){log('BLOCKED: flash operation active');return;}
  const file=$('restoreFile').files?.[0];
  if(!file||file.size!==FLASH_BYTES){log('RESTORE BLOCKED: choose a complete 4 MB backup');return;}
  log('Restore requested. This experimental build intentionally disables full-device restore until its readback/write procedure has passed hardware validation.');
  $('status').textContent='RESTORE NOT ENABLED — USE DOCUMENTED RECOVERY';
};
