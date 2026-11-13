export const FLASH_BYTES=0x400000;
export const PARTITION_OFFSET=0x8000;
export const PARTITION_BYTES=0xc00;
export const APP_OFFSET=0x10000;
export function comparePartitionTable(backup, proposed){
  if(!(backup instanceof Uint8Array)||backup.length!==FLASH_BYTES) throw Error('Select the complete 4 MB backup');
  if(!(proposed instanceof Uint8Array)||proposed.length!==PARTITION_BYTES) throw Error('Unexpected release partition table');
  for(let i=0;i<PARTITION_BYTES;i++)if(backup[PARTITION_OFFSET+i]!==proposed[i])throw Error('Current partition table differs from release. Flashing blocked.');
  return true;
}
export function checkImageLayout(meta){
  if(meta.write_enabled!==false||meta.flash_size_bytes!==FLASH_BYTES)throw Error('Unexpected release metadata');
  const files=['bootloader.bin','partitions.bin','firmware.bin'];
  const offsets=[0x1000,0x8000,APP_OFFSET];
  if(meta.artifacts?.length!==3)throw Error('Unexpected artifact count');
  for(let i=0;i<3;i++){
    const a=meta.artifacts.find(x=>x.file===files[i]);
    if(!a||Number.parseInt(a.offset,16)!==offsets[i]||!Number.isSafeInteger(a.size_bytes)||a.size_bytes<=0||a.size_bytes+offsets[i]>FLASH_BYTES||!/^[a-f0-9]{64}$/i.test(a.sha256))throw Error('Invalid release image '+files[i]);
  }
  const app=meta.partitions?.find(p=>p.label==='app0');
  if(!app||app.offset!==APP_OFFSET||meta.artifacts.find(a=>a.file==='firmware.bin').size_bytes>app.size)throw Error('Firmware does not fit app0');
  return true;
}
export function bytesToBinaryString(bytes){
  if(!(bytes instanceof Uint8Array))throw Error('Expected byte array');
  let out='';for(let i=0;i<bytes.length;i+=8192)out+=String.fromCharCode(...bytes.subarray(i,i+8192));return out;
}

// esptool-js 0.7.0 writeFlash requires Uint8Array, not a binary string.
export function createFlashFileArray(firmware){
  if(!(firmware instanceof Uint8Array)||firmware.byteLength===0)throw Error('Firmware must be a nonempty Uint8Array');
  return [{data:new Uint8Array(firmware),address:APP_OFFSET}];
}

// Full install is a separate, explicit mode when no verified current partition map exists.
export function createFullInstallFileArray(images){
  const specs=[['bootloader.bin',0x1000,0x8000],['partitions.bin',0x8000,0x9000],['firmware.bin',APP_OFFSET,0x400000]];
  return specs.map(([name,address,end])=>{
    const data=images?.[name];
    if(!(data instanceof Uint8Array)||data.length===0||address+data.length>end)throw Error('Invalid full-install image: '+name);
    return {data:new Uint8Array(data),address};
  });
}
