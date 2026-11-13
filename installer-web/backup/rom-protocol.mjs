/** ESP32 ROM SLIP framing. Only SYNC (0x08) is permitted in this diagnostic module.
 * Sending serial bytes is not the same as writing flash memory.
 */
export const ROM_SYNC = 0x08;
export function slipEncode(bytes) {
  const out = [0xc0];
  for (const b of bytes) {
    if (b === 0xc0) out.push(0xdb, 0xdc);
    else if (b === 0xdb) out.push(0xdb, 0xdd);
    else out.push(b);
  }
  return Uint8Array.from([...out, 0xc0]);
}
export function slipDecode(frame) {
  const bytes = []; let escaping = false;
  for (const b of frame) {
    if (escaping) {
      if (b !== 0xdc && b !== 0xdd) throw Error('Invalid SLIP escape');
      bytes.push(b === 0xdc ? 0xc0 : 0xdb); escaping = false;
    } else if (b === 0xdb) escaping = true;
    else if (b === 0xc0) throw Error('Unexpected SLIP delimiter');
    else bytes.push(b);
  }
  if (escaping) throw Error('Truncated SLIP escape');
  return Uint8Array.from(bytes);
}
export function syncPacket() {
  const payload = Uint8Array.from([0x07,0x07,0x12,0x20,...Array(32).fill(0x55)]);
  const header = Uint8Array.from([0x00,ROM_SYNC,payload.length,0,0,0,0,0]);
  return slipEncode(Uint8Array.from([...header,...payload]));
}
export function parseRomResponse(frame) {
  const data = slipDecode(frame);
  if (data.length < 8) throw Error('ROM response too short');
  if (data[0] !== 1) throw Error('Not an ESP ROM response');
  return {opcode:data[1], length:data[2] | (data[3]<<8), value:(data[4] | (data[5]<<8) | (data[6]<<16) | (data[7]<<24))>>>0, data:data.slice(8)};
}
export function isSyncResponse(frame) {
  const r = parseRomResponse(frame);
  return r.opcode === ROM_SYNC && r.length <= r.data.length;
}
/** Byte framing independent of Web Serial. Drops unframed boot logs. */
export class SlipFrameCollector {
  constructor(maxFrame = 4096) { this.maxFrame=maxFrame; this.frame=[]; this.started=false; }
  push(chunk) {
    const frames=[];
    for(const b of chunk) {
      if(b===0xc0) {
        if(this.started && this.frame.length) frames.push(Uint8Array.from(this.frame));
        this.frame=[];this.started=true;
      } else if(this.started) {
        this.frame.push(b);
        if(this.frame.length>this.maxFrame) {this.frame=[];this.started=false;}
      }
    }
    return frames;
  }
}

/** ESP32 ROM READ_REG (0x0a). Reads a 32-bit memory-mapped register; no flash erase/write. */
export const ROM_READ_REG = 0x0a;
export const ESP32_EFUSE_MAC_WORD0 = 0x3ff5a004;
export function readRegisterPacket(address) {
  if (!Number.isInteger(address) || address < 0 || address > 0xffffffff) throw Error('Invalid register address');
  const payload = [address & 255,(address>>>8)&255,(address>>>16)&255,(address>>>24)&255];
  return slipEncode(Uint8Array.from([0,ROM_READ_REG,4,0,0,0,0,0,...payload]));
}
export function parseRegisterResponse(frame) {
  const response = parseRomResponse(frame);
  if (response.opcode !== ROM_READ_REG) throw Error('Unexpected ROM opcode');
  if (response.length > response.data.length) throw Error('Truncated ROM register response');
  // ROM status is typically two bytes, but do not infer success from a missing status.
  if (response.length < 2) throw Error('Missing ROM status bytes');
  if (response.data[0] !== 0) throw Error(`ROM read failed: status ${response.data[0]}`);
  return response.value;
}

/** ROM magic register is used for conservative chip-family identification.
 * Unknown magic values are deliberately NOT accepted as original ESP32.
 * This is not a flash-size probe or a guarantee of board compatibility.
 */
export const ESP_ROM_MAGIC_REG = 0x40001000;
export const ESP32_ROM_MAGIC = 0x00f01d83;
export function classifyRomMagic(value) {
  if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) return {chip:'Unknown',supported:false};
  return value === ESP32_ROM_MAGIC
    ? {chip:'ESP32 (original)',supported:true}
    : {chip:'Unknown / other ESP variant',supported:false};
}
