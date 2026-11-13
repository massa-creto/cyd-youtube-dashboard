import test from 'node:test';
import assert from 'node:assert/strict';
import {assessReadiness,makeDiagnosticReport} from './device-readiness.mjs';
test('all unverified by default',()=>{const s=assessReadiness();assert.equal(s.diagnosticsReady,false);assert.equal(s.backupEnabled,false);assert.equal(s.flashEnabled,false)});
test('verified ROM does not unlock flash',()=>{const s=assessReadiness({webSerial:true,connected:true,synced:true,chipVerified:true});assert.equal(s.diagnosticsReady,true);assert.equal(s.backupEnabled,false);assert.equal(s.flashEnabled,false)});
test('manual flash values cannot be trusted as device verification',()=>{const s=assessReadiness({webSerial:true,connected:true,synced:true,chipVerified:true});assert.equal(s.checks[4].ok,false)});
test('report contains no device identifiers or secrets',()=>{const report=makeDiagnosticReport(assessReadiness());assert.equal(JSON.parse(report).schema,'cyd-readiness-v1');assert.equal(report.includes('macAddress'),false)});
