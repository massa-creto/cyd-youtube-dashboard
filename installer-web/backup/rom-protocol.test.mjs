import test from 'node:test';
import assert from 'node:assert/strict';
import {slipEncode,slipDecode,syncPacket,parseRomResponse,isSyncResponse,SlipFrameCollector} from './rom-protocol.mjs';
test('SLIP escape roundtrip',()=>assert.deepEqual([...slipDecode(slipEncode([0xc0,0xdb,0x55]).slice(1,-1))],[0xc0,0xdb,0x55]));
test('SYNC command header and 36-byte payload',()=>{const p=slipDecode(syncPacket().slice(1,-1));assert.equal(p[0],0);assert.equal(p[1],8);assert.equal(p[2],36);assert.equal(p.length,44);});
test('response validation',()=>{const frame=Uint8Array.from([1,8,0,0,0,0,0,0]);assert.equal(isSyncResponse(frame),true);assert.equal(parseRomResponse(frame).opcode,8);assert.equal(isSyncResponse(Uint8Array.from([1,9,0,0,0,0,0,0])),false);});
test('collector supports split frames',()=>{const c=new SlipFrameCollector();assert.equal(c.push([0xc0,1,8]).length,0);assert.deepEqual([...c.push([0,0,0,0,0,0,0xc0])[0]],[1,8,0,0,0,0,0,0]);});
test('rejects malformed frames',()=>{assert.throws(()=>slipDecode([0xdb,0x00]));assert.throws(()=>parseRomResponse([1,8]));});
import {ROM_READ_REG,readRegisterPacket,parseRegisterResponse} from './rom-protocol.mjs';
test('READ_REG request encodes 32-bit address little-endian',()=>{
 const packet=slipDecode(readRegisterPacket(0x3ff5a004).slice(1,-1));
 assert.deepEqual([...packet],[0,ROM_READ_REG,4,0,0,0,0,0,4,0xa0,0xf5,0x3f]);
});
test('READ_REG validates status and opcode',()=>{
 assert.equal(parseRegisterResponse(Uint8Array.from([1,ROM_READ_REG,2,0,0x78,0x56,0x34,0x12,0,0])),0x12345678);
 assert.throws(()=>parseRegisterResponse(Uint8Array.from([1,ROM_READ_REG,2,0,0,0,0,0,1,0])));
 assert.throws(()=>parseRegisterResponse(Uint8Array.from([1,ROM_READ_REG,2,0,0,0,0,0,0])));
});

import {ESP_ROM_MAGIC_REG,ESP32_ROM_MAGIC,classifyRomMagic} from './rom-protocol.mjs';
test('ROM magic register request stays read-only',()=>{const p=slipDecode(readRegisterPacket(ESP_ROM_MAGIC_REG).slice(1,-1));assert.equal(p[1],ROM_READ_REG);});
test('original ESP32 magic is accepted',()=>assert.deepEqual(classifyRomMagic(ESP32_ROM_MAGIC),{chip:'ESP32 (original)',supported:true}));
test('unknown ROM magic is blocked',()=>{for(const v of [0,0xffffffff,0x12345678,-1]) assert.equal(classifyRomMagic(v).supported,false);});
