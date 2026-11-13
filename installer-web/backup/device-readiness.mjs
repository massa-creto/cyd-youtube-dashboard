/** Conservative readiness classification. Never authorizes flash access. */
export function assessReadiness({webSerial=false,connected=false,synced=false,chipVerified=false,flashBytes=null}={}) {
  const checks = [
    {name:'Web Serial',ok:webSerial},
    {name:'Serial port connected',ok:connected},
    {name:'ESP32 ROM handshake',ok:synced},
    {name:'Original ESP32 identified',ok:chipVerified},
    {name:'Device flash capacity verified',ok:Number.isSafeInteger(flashBytes) && flashBytes>=1048576 && flashBytes<=33554432}
  ];
  return {checks,diagnosticsReady:checks.slice(0,4).every(x=>x.ok),backupEnabled:false,flashEnabled:false,
    message:'No flash read, backup, restore or installation is enabled in this checkpoint.'};
}
export function makeDiagnosticReport(readiness) {
  return JSON.stringify({schema:'cyd-readiness-v1',generatedAt:new Date().toISOString(),checks:readiness.checks,
    diagnosticsReady:readiness.diagnosticsReady,backupEnabled:false,flashEnabled:false,
    note:'No device identifiers, register values, credentials, or serial logs included.'},null,2)+'\n';
}
