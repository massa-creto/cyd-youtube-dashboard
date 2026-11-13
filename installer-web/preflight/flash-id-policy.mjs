// Validate JEDEC flash identity from esptool-js return value or its terminal output.
// Some esptool-js versions print the decoded identity but return no flashId value.
export function parseFlashId(value) {
  let id;
  if (typeof value === 'number' && Number.isSafeInteger(value)) id = value;
  else if (typeof value === 'string' && /^(?:0x)?[0-9a-f]{6}$/i.test(value.trim())) id = Number.parseInt(value.trim().replace(/^0x/i, ''), 16);
  else throw Error('Unexpected flash ID format');
  if (id < 0 || id > 0xffffff || ((id >>> 16) & 255) !== 0x16) throw Error('Flash ID does not indicate 4 MB capacity');
  return id;
}

export function verifyFlashEvidence(value, lines) {
  const messages = lines.map(s => String(s));
  const idLines = messages.map(s => s.match(/\bFlash ID:\s*(?:0x)?([0-9a-f]{6})\b/i)).filter(Boolean);
  const maker = messages.map(s => s.match(/\bManufacturer:\s*(?:0x)?([0-9a-f]{2})\b/i)).find(Boolean);
  const device = messages.map(s => s.match(/\bDevice:\s*(?:0x)?([0-9a-f]{4})\b/i)).find(Boolean);
  const sizes = messages.map(s => s.match(/\bDetected flash size:\s*(\d+)\s*MB\b/i)).filter(Boolean);
  if (!idLines.length || !maker || !device || !sizes.length) throw Error('Incomplete flash identification evidence');
  const ids = idLines.map(m => parseFlashId(m[1]));
  if (!ids.every(id => id === ids[0])) throw Error('Conflicting flash IDs');
  if (!sizes.every(m => Number(m[1]) === 4)) throw Error('Flash size mismatch');
  const id = ids[0];
  const expectedMaker = id & 255, expectedDevice = (((id >>> 8) & 255) << 8) | ((id >>> 16) & 255);
  if (Number.parseInt(maker[1], 16) !== expectedMaker || Number.parseInt(device[1], 16) !== expectedDevice) throw Error('Flash ID and manufacturer/device mismatch');
  if (value !== undefined && value !== null) {
    if (parseFlashId(value) !== id) throw Error('Flash ID return value conflicts with ROM log');
  }
  return Object.freeze({id, bytes: 4194304});
}
