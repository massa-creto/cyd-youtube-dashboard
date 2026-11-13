import {assessReadiness,makeDiagnosticReport} from "./device-readiness.mjs";
import {romExchange} from "./rom-transaction.mjs";
import {parseJedecId} from "./flash-capacity.mjs";
import {createBackup,sha256,makeMetadata} from "./backup-core.mjs";
import {openReadOnlyConnection,serialSupported} from "./serial-connect.mjs";
import {syncPacket,SlipFrameCollector,isSyncResponse,readRegisterPacket,parseRegisterResponse,ESP32_EFUSE_MAC_WORD0,ROM_READ_REG,slipDecode,ESP_ROM_MAGIC_REG,classifyRomMagic} from "./rom-protocol.mjs";
const $=id=>document.getElementById(id);
const supported=serialSupported({serial:navigator.serial,secureContext:window.isSecureContext});
$("status").textContent=supported ? "Web Serial is available; actual flash reading remains disabled pending hardware verification." : "Web Serial is unavailable. Use HTTPS and desktop Chrome/Edge for future device backup.";
const update=()=>{$("demo").disabled=!($("ack-privacy").checked && $("ack-risk").checked)};
$("ack-privacy").addEventListener("change",update);$("ack-risk").addEventListener("change",update);
$("reset").addEventListener("click",()=>{$("ack-privacy").checked=false;$("ack-risk").checked=false;$("progress").value=0;$("result").textContent="No simulation started.";update()});
$("demo").addEventListener("click",async()=>{
 $("demo").disabled=true;$("result").textContent="Generating synthetic backup data locally…";
 try {
  // No Web Serial access, no device commands, and no restoration capability.
  const size=2*1048576;
  const reader={identify:async()=>({chip:"ESP32",flashBytes:size,readOnly:true}),readFlash:async(offset,length)=>new Uint8Array(length).fill((offset/16384)%256)};
  const backup=await createBackup({reader,size,onProgress:(done,total)=>{$("progress").value=Math.round(done/total*100)}});
  const hash=await sha256(backup.bytes);const metadata=makeMetadata({size,hash,mode:"SIMULATION_ONLY"});
  $("result").textContent=`Simulation complete: ${metadata.flash_bytes} synthetic bytes, SHA-256 ${metadata.sha256}. No real device was accessed or backed up.`;
 } catch(err){$("result").textContent=`Simulation failed: ${err.message}`}
 update();
});

let connection = null;
let romSynced = false;
let identifiedOriginalEsp32 = false;
const connectButton = $("connect");
const disconnectButton = $("disconnect");
connectButton.disabled = !supported;
connectButton.addEventListener("click", async () => {
  connectButton.disabled = true;
  $("connection-result").textContent = "Requesting serial permission…";
  try {
    connection = await openReadOnlyConnection({serial:navigator.serial,secureContext:window.isSecureContext});
    const {usbVendorId,usbProductId} = connection.info;
    const fmt = value => Number.isInteger(value) ? `0x${value.toString(16).padStart(4,"0")}` : "unavailable";
    $("connection-result").textContent = `Serial port opened (USB VID ${fmt(usbVendorId)}, PID ${fmt(usbProductId)}). No commands sent. ESP32 identity and flash size NOT verified.`;
    romSynced = false; identifiedOriginalEsp32 = false; $("rom-register").disabled = true; $("rom-identify").disabled = true;
    disconnectButton.disabled = false;
    $("rom-sync").disabled = false;
  } catch (error) {
    $("connection-result").textContent = `Connection not established: ${error.message}`;
    connectButton.disabled = !supported;
  }
});
disconnectButton.addEventListener("click", async () => {
  disconnectButton.disabled = true;
  try { await connection?.disconnect(); $("connection-result").textContent = "Serial port closed. No device commands sent."; }
  catch(error) { $("connection-result").textContent = `Could not close port: ${error.message}. Unplug USB if necessary.`; }
  finally { connection = null; romSynced = false; identifiedOriginalEsp32 = false; $("rom-register").disabled = true; $("rom-identify").disabled = true; $("rom-sync").disabled = true; connectButton.disabled = !supported; }
});

// Explicit user-triggered ROM SYNC. This operation sends serial bytes but NEVER flash write/erase.
$("rom-sync").addEventListener("click", async () => {
  const button = $("rom-sync"); button.disabled = true;
  const port = connection?.port;
  if (!port?.readable || !port?.writable) { $("rom-result").textContent = "Serial stream unavailable."; button.disabled = false; return; }
  $("rom-result").textContent = "Sending ROM SYNC. If the board is running normal firmware, enter ROM download mode and reconnect.";
  try {
    const found = await romExchange(port, syncPacket(), frame => {
      try { return isSyncResponse(frame) ? true : null; } catch(_) { return null; }
    });
    romSynced = found === true; identifiedOriginalEsp32 = false;
    $("rom-register").disabled = true; $("rom-identify").disabled = !romSynced;
    $("rom-result").textContent = "ESP32 ROM SYNC acknowledged. No flash read/write/erase performed.";
  } catch(error) {
    romSynced=false; identifiedOriginalEsp32=false;
    $("rom-register").disabled=true; $("rom-identify").disabled=true;
    $("rom-result").textContent = `ROM SYNC failed: ${error.message}. No flash write attempted.`;
  } finally { button.disabled = !connection; }
});

// Explicit read-only register diagnostic. No flash capacity or device compatibility is inferred.
$("rom-register").addEventListener("click", async () => {
  const button = $("rom-register"); button.disabled = true;
  if (!romSynced || !identifiedOriginalEsp32 || !connection?.port?.readable || !connection?.port?.writable) {
    $("register-result").textContent = "ROM SYNC is required before register access."; return;
  }
  try {
    const value=await romExchange(connection.port,readRegisterPacket(ESP32_EFUSE_MAC_WORD0),frame=>parseRomResponseForRegister(frame));
    $("register-result").textContent = `ESP32 register 0x${ESP32_EFUSE_MAC_WORD0.toString(16)} returned 0x${value.toString(16).padStart(8,"0")}. Not a flash backup.`;
  } catch(error) {
    $("register-result").textContent = `Register diagnostic failed: ${error.message}. No flash write/erase performed.`;
  } finally { button.disabled = !romSynced || !identifiedOriginalEsp32 || !connection; }
});
function parseRomResponseForRegister(frame) {
  // Ignore unrelated serial frames; strict validation for READ_REG replies.
  const decoded = slipDecodeForRegister(frame);
  if (decoded.length < 2 || decoded[1] !== ROM_READ_REG) return null;
  return parseRegisterResponse(frame);
}
function slipDecodeForRegister(frame) { return slipDecode(frame); }

// Conservative ROM chip identification; no SPI flash commands or writes.
$("rom-identify").addEventListener("click", async () => {
  const button = $("rom-identify"); button.disabled = true;
  identifiedOriginalEsp32 = false; $("rom-register").disabled = true;
  if (!romSynced || !connection?.port?.readable || !connection?.port?.writable) {
    $("identify-result").textContent = "ROM SYNC and a serial connection are required.";
    return;
  }
  try {
    const magic=await romExchange(connection.port,readRegisterPacket(ESP_ROM_MAGIC_REG),frame=>{
      let decoded;
      try { decoded=slipDecode(frame); } catch(_) { return null; }
      if(decoded.length<2 || decoded[1]!==ROM_READ_REG) return null;
      return parseRegisterResponse(frame);
    });
    const result=classifyRomMagic(magic);
    identifiedOriginalEsp32=result.supported;
    $("rom-register").disabled=!identifiedOriginalEsp32;
    $("identify-result").textContent=`ROM magic 0x${magic.toString(16).padStart(8,"0")}: ${result.chip}. ${result.supported ? "Original ESP32 register diagnostic unlocked; flash access remains disabled." : "Unknown chip: further chip-specific operations blocked."}`;
  } catch(error) {
    $("identify-result").textContent=`Identification failed: ${error.message}. No flash write/erase performed.`;
  } finally { button.disabled = !romSynced || !connection; }
});

// Offline-only JEDEC decoder. Never promotes manually entered values to trusted device facts.
$("decode-jedec").addEventListener("click", () => {
  try {
    const result = parseJedecId($("jedec-id").value);
    $("jedec-result").textContent = result.bytesCapacity === null
      ? "Capacity code not supported. Device flash capacity remains unverified."
      : `Nominal JEDEC capacity: ${result.mib} MiB. MANUAL, UNVERIFIED. No device read was performed. Flashing remains disabled.`;
  } catch(error) { $("jedec-result").textContent = `Invalid ID: ${error.message}`; }
});

// The report is intentionally free of USB IDs, eFuse data, and secrets.
function currentReadiness() {
 return assessReadiness({webSerial:supported,connected:!!connection,synced:romSynced,chipVerified:identifiedOriginalEsp32});
}
function renderReadiness() {
 const state=currentReadiness();
 $("readiness-result").textContent=state.checks.map(c=>`${c.ok?'PASS':'PENDING'}: ${c.name}`).join(' | ')+'. Backup and flashing remain disabled.';
}
$("download-readiness").addEventListener("click",()=>{
 const blob=new Blob([makeDiagnosticReport(currentReadiness())],{type:'application/json'});
 const url=URL.createObjectURL(blob); const a=document.createElement('a');a.href=url;a.download='cyd-readiness.json';a.click();
 setTimeout(()=>URL.revokeObjectURL(url),1000);
});
for(const id of ['connect','disconnect','rom-sync','rom-identify','rom-register']) {
 $(id).addEventListener('click',()=>{renderReadiness();setTimeout(renderReadiness,3500)});
}
renderReadiness();
