/** Browser serial permission probe. Never sends bytes to the device. */
export function serialSupported({serial, secureContext}) {
  return secureContext === true && !!serial && typeof serial.requestPort === "function";
}
export async function openReadOnlyConnection({serial, secureContext, baudRate = 115200}) {
  if (!serialSupported({serial, secureContext})) throw new Error("Web Serial requires HTTPS and a compatible desktop browser.");
  if (baudRate !== 115200) throw new Error("Unsupported serial speed.");
  const port = await serial.requestPort(); // Requires explicit user gesture and permission.
  if (!port || typeof port.open !== "function" || typeof port.close !== "function") throw new Error("Invalid serial port.");
  try {
    await port.open({baudRate, bufferSize: 4096});
  } catch (error) {
    // Some implementations may hold a partially opened port.
    try { await port.close(); } catch (_) { /* already closed */ }
    throw error;
  }
  return {
    port, // Exposed only for explicitly invoked read-only ROM diagnostics.
    info: typeof port.getInfo === "function" ? port.getInfo() : {},
    async disconnect() { await port.close(); },
  };
}
