import test from 'node:test';import assert from 'node:assert/strict';
import {comparePartitionTable,checkImageLayout,bytesToBinaryString,createFlashFileArray,FLASH_BYTES,PARTITION_OFFSET,PARTITION_BYTES} from './safety.mjs';
import {readFileSync} from 'node:fs';
const meta=JSON.parse(readFileSync(new URL('../release/release-metadata.json',import.meta.url)));
test('release image layout passes',()=>assert.equal(checkImageLayout(meta),true));
test('reject altered layout',()=>assert.throws(()=>checkImageLayout({...meta,artifacts:meta.artifacts.map(a=>a.file==='firmware.bin'?{...a,offset:'0x20000'}:a)})));
test('reject enabled metadata',()=>assert.throws(()=>checkImageLayout({...meta,write_enabled:true})));
test('partition table match',()=>{const backup=new Uint8Array(FLASH_BYTES),partition=new Uint8Array(PARTITION_BYTES);partition.fill(4);backup.set(partition,PARTITION_OFFSET);assert.equal(comparePartitionTable(backup,partition),true);});
test('partition mismatch blocks',()=>assert.throws(()=>comparePartitionTable(new Uint8Array(FLASH_BYTES),new Uint8Array(PARTITION_BYTES).fill(1))));
test('backup size mismatch blocks',()=>assert.throws(()=>comparePartitionTable(new Uint8Array(100),new Uint8Array(PARTITION_BYTES))));
test('binary conversion preserves bytes',()=>assert.deepEqual(Array.from(bytesToBinaryString(Uint8Array.of(0,127,128,255))).map(x=>x.charCodeAt(0)),[0,127,128,255]));

test('writeFlash payload uses Uint8Array and correct application offset',()=>{const original=Uint8Array.of(0,127,128,255);const files=createFlashFileArray(original);assert.equal(files.length,1);assert.equal(files[0].address,0x10000);assert.ok(files[0].data instanceof Uint8Array);assert.deepEqual(files[0].data,original);assert.notEqual(files[0].data,original);});
test('writeFlash payload rejects string and empty input',()=>{assert.throws(()=>createFlashFileArray('\x00'));assert.throws(()=>createFlashFileArray(new Uint8Array()));});
