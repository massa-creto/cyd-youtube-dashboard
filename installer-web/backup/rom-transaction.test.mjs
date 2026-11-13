import test from 'node:test';
import assert from 'node:assert/strict';
import {romExchange} from './rom-transaction.mjs';
const packet=new Uint8Array([0xc0,0xc0]);
function mockPort({chunks=[],pending=false}={}) {
  const state={cancelled:false,released:false,written:0};
  let wake;
  const reader={async read(){if(chunks.length)return {value:chunks.shift(),done:false};if(!pending)return {done:true};return new Promise(resolve=>{wake=resolve})},async cancel(){state.cancelled=true;wake?.({done:true})},releaseLock(){state.released=true}};
  const writer={async write(){state.written++},releaseLock(){}};
  return {port:{readable:{getReader:()=>reader},writable:{getWriter:()=>writer}},state};
}
test('returns matched SLIP frame and releases lock',async()=>{
  const {port,state}=mockPort({chunks:[new Uint8Array([0xc0,1,2,0xc0])]});
  assert.equal(await romExchange(port,packet,frame=>frame[0]===1?'ok':null),'ok');
  assert.equal(state.cancelled,true);assert.equal(state.released,true);assert.equal(state.written,1);
});
test('timeout cancels pending read and releases lock',async()=>{
  const {port,state}=mockPort({pending:true});
  await assert.rejects(romExchange(port,packet,()=>null,{timeoutMs:100}),/timeout/);
  assert.equal(state.cancelled,true);assert.equal(state.released,true);
});
test('closed stream releases locks',async()=>{
  const {port,state}=mockPort();
  await assert.rejects(romExchange(port,packet,()=>null),/closed/);
  assert.equal(state.released,true);
});
test('rejects invalid timeout without opening streams',async()=>{
  await assert.rejects(romExchange({},packet,()=>null,{timeoutMs:0}),/unavailable|timeout/);
});
