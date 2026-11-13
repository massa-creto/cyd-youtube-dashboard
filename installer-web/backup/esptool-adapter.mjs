// Official Espressif esptool-js adapter. No custom ROM commands.
// Import is intentionally lazy: the existing page works without a CDN connection.
export const ESPTOOL_VERSION = '0.7.0';
export const ESPTOOL_BUNDLE = `https://cdn.jsdelivr.net/npm/esptool-js@${ESPTOOL_VERSION}/+esm`;
export async function importOfficialLibrary(importer = url => import(url)) {
  const module = await importer(ESPTOOL_BUNDLE);
  const library = module?.default?.ESPLoader && module?.default?.Transport ? module.default : module;
  validateLibrary(library);
  return library;
}
// Match only verified original ESP32 identifiers, not newer ESP32 families.
// esptool-js reports ESP32-D0WD-V3 for the tested original ESP32 silicon.
export function isSupportedChip(name) {
  const chip = String(name || '').trim();
  return /^ESP32(?:\s*\(.*\))?$/i.test(chip) ||
    /^ESP32-D0WD(?:-V3)?(?:\s*\(revision\s+\d+(?:\.\d+)?\))?$/i.test(chip);
}
export function validateBackupSize(bytes) { return Number.isSafeInteger(bytes) && bytes >= 1048576 && bytes <= 33554432 && (bytes & (bytes-1)) === 0; }
export function flashWritingEnabled() { return false; }
export function validateDeviceIdentity(chip, flashBytes) {
  if (!isSupportedChip(chip)) throw new Error('Unsupported chip; only original ESP32 is allowed');
  if (!validateBackupSize(flashBytes)) throw new Error('Flash size must be independently confirmed');
  return Object.freeze({chip: 'ESP32', flashBytes, readOnly: true});
}
export function validateReadback(data, expectedBytes) {
  if (!(data instanceof Uint8Array)) throw new Error('Flash readback must be Uint8Array');
  if (!validateBackupSize(expectedBytes) || data.byteLength !== expectedBytes) throw new Error('Incomplete flash readback');
  return true;
}
export function validateLibrary(library) {
  if (!library || typeof library.ESPLoader !== 'function' || typeof library.Transport !== 'function') {
    throw new Error('Unsupported esptool-js browser bundle: ESPLoader or Transport missing');
  }
  return true;
}
export function safeError(error) {
  const message = error instanceof Error ? error.message : String(error);
  return message.replace(/[\r\n\t]+/g, ' ').slice(0, 240);
}

export async function createLoader({serial=navigator.serial, library, terminal}) {
  if (!serial || !globalThis.isSecureContext) throw new Error('Requires HTTPS and Web Serial capable browser');
  validateLibrary(library);
  const port = await serial.requestPort();
  const transport = new library.Transport(port, true);
  const loader = new library.ESPLoader({transport, baudrate:115200, terminal, debugLogging:false});
  return {loader, transport};
}
