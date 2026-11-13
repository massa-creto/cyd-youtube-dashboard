import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createBackup, validateFlashSize, sha256, makeMetadata} from './backup-core.mjs';
test('rejects unknown capacity',()=>assert.throws(()=>validateFlashSize(3*1048576)));
test('rejects missing adapter',async()=>await assert.rejects(createBackup({size:2*1048576}),/adapter/));
test('rejects wrong chip',async()=>await assert.rejects(createBackup({size:2*1048576,reader:{identify:async()=>({chip:'ESP8266',flashBytes:2*1048576,readOnly:true}),readFlash:async()=>new Uint8Array(16384)}}),/not verified/));
test('rejects truncated chunk',async()=>await assert.rejects(createBackup({size:2*1048576,reader:{identify:async()=>({chip:'ESP32',flashBytes:2*1048576,readOnly:true}),readFlash:async()=>new Uint8Array(1)}}),/Incomplete/));
test('successful synthetic backup and digest',async()=>{const size=2*1048576;const data=await createBackup({size,reader:{identify:async()=>({chip:'ESP32',flashBytes:size,readOnly:true}),readFlash:async(_,length)=>new Uint8Array(length)}});assert.equal(data.bytes.length,size);const hash=await sha256(data.bytes);assert.equal(hash.length,64);assert.equal(makeMetadata({size,hash,mode:'SIMULATION_ONLY'}).restore_tested,false)});
