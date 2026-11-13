import test from 'node:test';
import assert from 'node:assert/strict';
import {isSupportedChip,validateBackupSize,flashWritingEnabled,ESPTOOL_VERSION,validateLibrary,safeError,validateDeviceIdentity,validateReadback} from './esptool-adapter.mjs';
test('original ESP32 only',()=>{assert.equal(isSupportedChip('ESP32'),true);for(const x of ['ESP32-S3','ESP32-C3','ESP32-S2','ESP32-H2','unknown'])assert.equal(isSupportedChip(x),false)});
test('safe capacity validation',()=>{assert.equal(validateBackupSize(4194304),true);for(const n of [0,NaN,123,2**26])assert.equal(validateBackupSize(n),false)});
test('writes stay disabled',()=>assert.equal(flashWritingEnabled(),false));
test('dependency version pinned',()=>assert.equal(ESPTOOL_VERSION,'0.7.0'));

test('library API gate rejects absent or malformed exports',()=>{
  assert.throws(()=>validateLibrary(undefined),/Unsupported/);
  assert.throws(()=>validateLibrary({ESPLoader:()=>{}}),/Unsupported/);
  assert.equal(validateLibrary({ESPLoader:class {},Transport:class {}}),true);
});
test('safe error output is bounded and single line',()=>{
  const output=safeError(new Error('bad\nsecret\t'+'x'.repeat(500)));
  assert.ok(output.length<=240);
  assert.ok(!/[\r\n\t]/.test(output));
});

test('device identity and capacity gate fails closed',()=>{
  assert.deepEqual(validateDeviceIdentity('ESP32',4194304),{chip:'ESP32',flashBytes:4194304,readOnly:true});
  assert.throws(()=>validateDeviceIdentity('ESP32-S3',4194304),/Unsupported/);
  assert.throws(()=>validateDeviceIdentity('ESP32',0),/independently confirmed/);
});
test('full readback requires exact expected byte count',()=>{
  assert.equal(validateReadback(new Uint8Array(1048576),1048576),true);
  assert.throws(()=>validateReadback(new Uint8Array(1024),1048576),/Incomplete/);
  assert.throws(()=>validateReadback('not bytes',1048576),/Uint8Array/);
});

// The browser bundle is an ES module, not a window.esptooljs global.
import {importOfficialLibrary,ESPTOOL_BUNDLE} from './esptool-adapter.mjs';
test('loads named exports from a browser ES module',async()=>{
 const api=await importOfficialLibrary(async url=>{assert.match(url,/\+esm$/);return {ESPLoader:class{},Transport:class{}}});
 assert.equal(typeof api.ESPLoader,'function');
});
test('rejects a module missing Transport',async()=>{
 await assert.rejects(importOfficialLibrary(async()=>({ESPLoader:class{}})),/Unsupported/);
});

test('hardware-reported ESP32-D0WD-V3 is accepted without broadening to other families',()=>{
  for(const chip of ['ESP32-D0WD-V3 (revision 3)', 'ESP32-D0WD-V3 (revision 3.1)', 'ESP32-D0WD', 'ESP32']) {
    assert.equal(isSupportedChip(chip), true, chip);
    assert.equal(validateDeviceIdentity(chip,4194304).chip, 'ESP32');
  }
  for(const chip of ['ESP32-S3', 'ESP32-C3', 'ESP32-C6', 'ESP32-H2', 'ESP32-P4', 'ESP32-S2', 'ESP32-UNKNOWN', 'ESP32-D0WD-V3 garbage']) {
    assert.equal(isSupportedChip(chip), false, chip);
  }
});
