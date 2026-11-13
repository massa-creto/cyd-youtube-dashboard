/** Single-flight ROM exchange. The timeout actively cancels a pending read;
 * unlike Promise.race alone, no orphan reader is left holding the port lock.
 * Only callers choose which ROM command is sent. This module has no flash write/erase opcodes.
 */
import {SlipFrameCollector} from './rom-protocol.mjs';
export async function romExchange(port, packet, match, {timeoutMs=2500, maxFrame=4096}={}) {
  if (!port?.readable || !port?.writable) throw Error('Serial streams unavailable');
  if (!Number.isInteger(timeoutMs) || timeoutMs<100 || timeoutMs>30000) throw Error('Invalid timeout');
  let reader, writer, timer, expired=false;
  try {
    reader=port.readable.getReader(); writer=port.writable.getWriter();
    await writer.write(packet);
    const collector=new SlipFrameCollector(maxFrame);
    // Cancellation resolves a blocked reader.read() before we release its lock.
    timer=setTimeout(()=>{expired=true; void reader.cancel('ROM response timeout').catch(()=>{});},timeoutMs);
    while(true) {
      const {value,done}=await reader.read();
      if(done) throw Error(expired?'ROM response timeout':'Serial stream closed');
      for(const frame of collector.push(value)) {
        const result=match(frame);
        if(result!==null && result!==undefined && result!==false) return result;
      }
    }
  } finally {
    if(timer!==undefined) clearTimeout(timer);
    if(reader) {try {await reader.cancel();} catch(_){} try{reader.releaseLock()}catch(_){}}
    if(writer) {try{writer.releaseLock()}catch(_){}}
  }
}
