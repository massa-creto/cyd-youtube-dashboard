import test from 'node:test';
import assert from 'node:assert/strict';
import {serialSupported,openReadOnlyConnection} from './serial-connect.mjs';
test('requires secure context and Web Serial',()=>{
  assert.equal(serialSupported({serial:{requestPort(){}},secureContext:true}),true);
  assert.equal(serialSupported({serial:{requestPort(){}},secureContext:false}),false);
  assert.equal(serialSupported({serial:null,secureContext:true}),false);
});
test('opens and closes without writing or erasing',async()=>{
  const calls=[];
  const port={open:async options=>calls.push(['open',options]),close:async()=>calls.push(['close']),getInfo:()=>({usbVendorId:0x10c4})};
  const connection=await openReadOnlyConnection({serial:{requestPort:async()=>port},secureContext:true});
  assert.equal(connection.info.usbVendorId,0x10c4);
  await connection.disconnect();
  assert.deepEqual(calls,[['open',{baudRate:115200,bufferSize:4096}],['close']]);
});
test('rejects unsupported speed and missing permission',async()=>{
  await assert.rejects(openReadOnlyConnection({serial:{requestPort:async()=>({})},secureContext:false}));
  await assert.rejects(openReadOnlyConnection({serial:{requestPort:async()=>({})},secureContext:true,baudRate:9600}));
});
test('closes after failure to open',async()=>{
  let closed=false;
  await assert.rejects(openReadOnlyConnection({serial:{requestPort:async()=>({open:async()=>{throw Error('busy')},close:async()=>{closed=true}})},secureContext:true}),/busy/);
  assert.equal(closed,true);
});
