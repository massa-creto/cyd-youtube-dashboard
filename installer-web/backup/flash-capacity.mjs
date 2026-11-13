/** Offline JEDEC SPI flash ID interpretation. No device access or serial commands.
 * A reported capacity is informational until read back and validated on hardware.
 */
export function parseJedecId(input) {
  if (typeof input !== 'string') throw Error('JEDEC ID must be text');
  const cleaned = input.trim().replace(/^0x/i, '').replace(/[\s:_-]/g, '');
  if (!/^[0-9a-fA-F]{6}$/.test(cleaned)) throw Error('Enter exactly 3 hexadecimal bytes (e.g. EF4016)');
  const bytes = [0,2,4].map(i => parseInt(cleaned.slice(i,i+2),16));
  if (bytes.every(x=>x===0) || bytes.every(x=>x===255)) throw Error('Invalid or disconnected flash ID');
  const [manufacturer, memoryType, capacityCode] = bytes;
  // JEDEC capacity exponent: 2^N bytes for conventional SPI NOR parts.
  // Limit to common ESP32-compatible capacities; unknowns remain untrusted.
  const recognized = capacityCode >= 0x14 && capacityCode <= 0x19;
  const bytesCapacity = recognized ? 2 ** capacityCode : null;
  return {manufacturer, memoryType, capacityCode, bytesCapacity,
    mib: bytesCapacity === null ? null : bytesCapacity / (1024*1024),
    status: recognized ? 'unverified-manual-entry' : 'unsupported-capacity-code'};
}
