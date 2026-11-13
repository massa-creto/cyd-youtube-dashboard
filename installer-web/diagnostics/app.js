"use strict";
// Read-only USB metadata viewer. NEVER open the serial port or transmit bytes.
const $ = id => document.getElementById(id);
const hex = value => Number.isInteger(value) ? "0x" + value.toString(16).toUpperCase().padStart(4,"0") : "Unknown";
const hints = new Map([["10C4:EA60","Silicon Labs CP210x USB-UART bridge"],["1A86:7523","WCH CH340 USB-UART bridge"],["1A86:55D4","WCH USB-UART bridge"],["0403:6001","FTDI USB-UART bridge"]]);
let selected = null, mode = "Idle", metadata = null;
const isSupported = () => window.isSecureContext && !!navigator.serial && typeof navigator.serial.requestPort === "function";
function render(){
  $("mode").textContent = mode;
  $("vid").textContent = metadata ? hex(metadata.usbVendorId) : "—";
  $("pid").textContent = metadata ? hex(metadata.usbProductId) : "—";
  const key = metadata && Number.isInteger(metadata.usbVendorId) && Number.isInteger(metadata.usbProductId) ? hex(metadata.usbVendorId).slice(2)+":"+hex(metadata.usbProductId).slice(2) : "";
  $("adapter").textContent = hints.get(key) || (metadata ? "Unrecognized or non-USB serial adapter" : "—");
  $("chip").textContent = "Not verified — USB metadata is insufficient";
  $("flash").textContent = "Not verified — no flash access";
  $("install").textContent = "Unavailable — no flashing code is included";
  $("disconnect").disabled = !selected && mode !== "Simulation";
}
function reset(){selected=null;metadata=null;mode="Idle";$("message").textContent="No port selected.";render()}
$("support").textContent = isSupported() ? "Web Serial available. Port selection requires your permission." : "Web Serial unavailable. Use recent desktop Chrome/Edge over HTTPS or localhost; simulation works without hardware.";
$("connect").disabled = !isSupported();
$("connect").addEventListener("click", async () => {
  if (!isSupported()) return;
  try {
    const port = await navigator.serial.requestPort(); // chooser only; no serial connection is opened
    const info = port.getInfo();
    selected = port;
    metadata = {usbVendorId:info.usbVendorId,usbProductId:info.usbProductId};
    mode = "USB metadata only";
    $("message").textContent = "Port selected. No serial connection was opened and no bytes were sent.";
    render();
  } catch(err) {
    $("message").textContent = err && err.name === "NotFoundError" ? "Port selection cancelled." : "Could not select port. Check browser permissions and try again.";
  }
});
$("simulate").addEventListener("click",()=>{selected=null;metadata={usbVendorId:0x10C4,usbProductId:0xEA60};mode="Simulation";$("message").textContent="SIMULATED metadata only. No device was contacted.";render()});
$("disconnect").addEventListener("click",reset);
$("reset").addEventListener("click",reset);
$("copy").addEventListener("click",async()=>{
  const report = ["CYD USB Diagnostics v0.1", "Mode: "+mode, "USB VID: "+$("vid").textContent,"USB PID: "+$("pid").textContent,"Adapter hint: "+$("adapter").textContent,"Chip: unverified","Flash: unverified","Install: disabled"].join("\n");
  try {await navigator.clipboard.writeText(report);$("message").textContent="Redacted report copied."} catch {$("message").textContent="Clipboard unavailable. No report was transmitted."}
});
if(navigator.serial && typeof navigator.serial.addEventListener === "function") navigator.serial.addEventListener("disconnect",e=>{if(selected && e.port === selected){reset();$("message").textContent="Selected USB device disconnected."}});
render();
