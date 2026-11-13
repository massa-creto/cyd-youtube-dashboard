/** Backup workflow independent of serial protocol implementation. No write operations. */
export const MAX_FLASH_BYTES = 16 * 1024 * 1024;
export function validateFlashSize(size) {
  if (!Number.isSafeInteger(size) || ![2,4,8,16].includes(size / 1048576)) {
    throw new Error("Unsupported or unverified flash capacity; backup blocked.");
  }
  return size;
}
export async function createBackup({reader, size, onProgress = () => {}}) {
  validateFlashSize(size);
  if (!reader || typeof reader.readFlash !== "function" || typeof reader.identify !== "function") throw new Error("No verified read-only flash adapter.");
  const identity = await reader.identify();
  if (identity.chip !== "ESP32" || identity.flashBytes !== size || identity.readOnly !== true) throw new Error("Chip identity or flash size not verified.");
  const buffer = new Uint8Array(size);
  const blockSize = 16384;
  for (let offset = 0; offset < size; offset += blockSize) {
    const length = Math.min(blockSize, size - offset);
    const chunk = await reader.readFlash(offset, length);
    if (!(chunk instanceof Uint8Array) || chunk.length !== length) throw new Error("Incomplete flash read; backup rejected.");
    buffer.set(chunk, offset);
    onProgress(offset + length, size);
  }
  return {bytes: buffer, chip: identity.chip, flashBytes: size};
}
export async function sha256(bytes) {
  if (!globalThis.crypto?.subtle) throw new Error("Secure context required for SHA-256.");
  const digest = await crypto.subtle.digest("SHA-256", bytes);
  return Array.from(new Uint8Array(digest), x => x.toString(16).padStart(2, "0")).join("");
}
export function makeMetadata({size, hash, mode}) {
  if (!/^[a-f0-9]{64}$/.test(hash)) throw new Error("Invalid SHA-256 digest");
  return {format:"cyd-full-flash-backup-v1",mode,flash_bytes:validateFlashSize(size),sha256:hash,contains_sensitive_data:true,verified_device_readback:false,restore_tested:false};
}
