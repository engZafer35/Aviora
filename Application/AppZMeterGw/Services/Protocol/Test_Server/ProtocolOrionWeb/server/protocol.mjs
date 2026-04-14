/**
 * Orion TLV protocol helpers — parity with ProtocolOrionTLV_TestServer.py
 */
export const PKT_START = 0x24;
export const PKT_END = 0x23;

export const TAG_FLAG = 0x0001;
export const TAG_SERIAL_NUMBER = 0x0002;
export const TAG_FUNCTION = 0x0003;
export const TAG_TRANS_NUMBER = 0x00ff;

export const TAG_REGISTERED = 0x0101;
export const TAG_DEVICE_BRAND = 0x0102;
export const TAG_DEVICE_MODEL = 0x0103;
export const TAG_DEVICE_DATE = 0x0104;
export const TAG_PULL_IP = 0x0105;
export const TAG_PULL_PORT = 0x0106;
export const TAG_REGISTER = 0x0107;

export const TAG_PACKET_NUM = 0x0201;
export const TAG_PACKET_STREAM = 0x0202;
export const TAG_ACK_STATUS = 0x0301;
export const TAG_LOG_DATA = 0x0401;

export const TAG_METER_OPERATION = 0x0501;
export const TAG_METER_PROTOCOL = 0x0502;
export const TAG_METER_TYPE = 0x0503;
export const TAG_METER_BRAND = 0x0504;
export const TAG_METER_SERIAL_NUM = 0x0505;
export const TAG_METER_SERIAL_PORT = 0x0506;
export const TAG_METER_INIT_BAUD = 0x0507;
export const TAG_METER_FIX_BAUD = 0x0508;
export const TAG_METER_FRAME = 0x0509;
export const TAG_METER_CUSTOMER_NUM = 0x050a;
export const TAG_METER_INDEX = 0x050b;

export const TAG_SERVER_IP = 0x0601;
export const TAG_SERVER_PORT = 0x0602;

export const TAG_METER_ID = 0x0701;
export const TAG_READOUT_DATA = 0x0702;
export const TAG_DIRECTIVE_NAME = 0x0703;
export const TAG_START_DATE = 0x0704;
export const TAG_END_DATE = 0x0705;

export const TAG_DIRECTIVE_ID = 0x0801;
export const TAG_DIRECTIVE_DATA = 0x0802;

export const TAG_FW_ADDRESS = 0x0901;
export const TAG_ERROR_CODE = 0x0a01;

export const FUNC_IDENT = 0x01;
export const FUNC_ALIVE = 0x02;
export const FUNC_ACK = 0x03;
export const FUNC_NACK = 0x04;
export const FUNC_LOG = 0x05;
export const FUNC_SETTING = 0x06;
export const FUNC_FW_UPDATE = 0x07;
export const FUNC_READOUT = 0x08;
export const FUNC_LOADPROFILE = 0x09;
export const FUNC_DIRECTIVE_LIST = 0x0a;
export const FUNC_DIRECTIVE_ADD = 0x0b;
export const FUNC_DIRECTIVE_DEL = 0x0c;

export const TAG_NAMES = new Map([
  [TAG_FLAG, "FLAG"],
  [TAG_SERIAL_NUMBER, "SERIAL_NUMBER"],
  [TAG_FUNCTION, "FUNCTION"],
  [TAG_TRANS_NUMBER, "TRANS_NUMBER"],
  [TAG_REGISTERED, "REGISTERED"],
  [TAG_DEVICE_BRAND, "DEVICE_BRAND"],
  [TAG_DEVICE_MODEL, "DEVICE_MODEL"],
  [TAG_DEVICE_DATE, "DEVICE_DATE"],
  [TAG_PULL_IP, "PULL_IP"],
  [TAG_PULL_PORT, "PULL_PORT"],
  [TAG_REGISTER, "REGISTER"],
  [TAG_PACKET_NUM, "PACKET_NUM"],
  [TAG_PACKET_STREAM, "PACKET_STREAM"],
  [TAG_ACK_STATUS, "ACK_STATUS"],
  [TAG_LOG_DATA, "LOG_DATA"],
  [TAG_METER_OPERATION, "METER_OPERATION"],
  [TAG_METER_PROTOCOL, "METER_PROTOCOL"],
  [TAG_METER_TYPE, "METER_TYPE"],
  [TAG_METER_BRAND, "METER_BRAND"],
  [TAG_METER_SERIAL_NUM, "METER_SERIAL_NUM"],
  [TAG_METER_SERIAL_PORT, "METER_SERIAL_PORT"],
  [TAG_METER_INIT_BAUD, "METER_INIT_BAUD"],
  [TAG_METER_FIX_BAUD, "METER_FIX_BAUD"],
  [TAG_METER_FRAME, "METER_FRAME"],
  [TAG_METER_CUSTOMER_NUM, "METER_CUSTOMER_NUM"],
  [TAG_METER_INDEX, "METER_INDEX"],
  [TAG_SERVER_IP, "SERVER_IP"],
  [TAG_SERVER_PORT, "SERVER_PORT"],
  [TAG_METER_ID, "METER_ID"],
  [TAG_READOUT_DATA, "READOUT_DATA"],
  [TAG_DIRECTIVE_NAME, "DIRECTIVE_NAME"],
  [TAG_START_DATE, "START_DATE"],
  [TAG_END_DATE, "END_DATE"],
  [TAG_DIRECTIVE_ID, "DIRECTIVE_ID"],
  [TAG_DIRECTIVE_DATA, "DIRECTIVE_DATA"],
  [TAG_FW_ADDRESS, "FW_ADDRESS"],
  [TAG_ERROR_CODE, "ERROR_CODE"],
]);

export const FUNC_NAMES = new Map([
  [FUNC_IDENT, "IDENT"],
  [FUNC_ALIVE, "ALIVE"],
  [FUNC_ACK, "ACK"],
  [FUNC_NACK, "NACK"],
  [FUNC_LOG, "LOG"],
  [FUNC_SETTING, "SETTING"],
  [FUNC_FW_UPDATE, "FW_UPDATE"],
  [FUNC_READOUT, "READOUT"],
  [FUNC_LOADPROFILE, "LOADPROFILE"],
  [FUNC_DIRECTIVE_LIST, "DIRECTIVE_LIST"],
  [FUNC_DIRECTIVE_ADD, "DIRECTIVE_ADD"],
  [FUNC_DIRECTIVE_DEL, "DIRECTIVE_DEL"],
]);

export class PacketBuilder {
  constructor() {
    this._chunks = [Buffer.from([PKT_START])];
  }

  addString(tag, val) {
    const data = val ? Buffer.from(String(val), "ascii") : Buffer.alloc(0);
    this._addTlv(tag, data);
  }

  addBool(tag, val) {
    this._addTlv(tag, Buffer.from([val ? 1 : 0]));
  }

  addUint8(tag, val) {
    this._addTlv(tag, Buffer.from([val & 0xff]));
  }

  addUint16(tag, val) {
    const b = Buffer.alloc(2);
    b.writeUInt16BE(val & 0xffff, 0);
    this._addTlv(tag, b);
  }

  addUint32(tag, val) {
    const b = Buffer.alloc(4);
    b.writeUInt32BE(val >>> 0, 0);
    this._addTlv(tag, b);
  }

  addInt16(tag, val) {
    const b = Buffer.alloc(2);
    b.writeInt16BE(val, 0);
    this._addTlv(tag, b);
  }

  addRaw(tag, data) {
    this._addTlv(tag, Buffer.isBuffer(data) ? data : Buffer.from(data));
  }

  addDeviceHeader(flag, serial) {
    this.addString(TAG_FLAG, flag);
    this.addString(TAG_SERIAL_NUMBER, serial);
  }

  finish() {
    this._chunks.push(Buffer.from([PKT_END]));
    return Buffer.concat(this._chunks);
  }

  _addTlv(tag, data) {
    const head = Buffer.alloc(4);
    head.writeUInt16BE(tag & 0xffff, 0);
    head.writeUInt16BE(data.length, 2);
    this._chunks.push(head, data);
  }
}

export function parsePacket(raw) {
  if (!raw || raw.length < 2 || raw[0] !== PKT_START || raw[raw.length - 1] !== PKT_END) {
    return null;
  }
  const payload = raw.subarray(1, -1);
  const fields = [];
  let pos = 0;
  while (pos + 4 <= payload.length) {
    const tag = payload.readUInt16BE(pos);
    const len = payload.readUInt16BE(pos + 2);
    pos += 4;
    if (pos + len > payload.length) break;
    fields.push([tag, payload.subarray(pos, pos + len)]);
    pos += len;
  }
  return fields;
}

export function findTag(fields, tag) {
  for (const [t, v] of fields) {
    if (t === tag) return v;
  }
  return null;
}

export function getString(fields, tag, defaultVal = "") {
  const v = findTag(fields, tag);
  if (!v) return defaultVal;
  return v.toString("ascii");
}

export function getUint8(fields, tag, defaultVal = 0) {
  const v = findTag(fields, tag);
  return v && v.length >= 1 ? v[0] : defaultVal;
}

export function getUint16(fields, tag, defaultVal = 0) {
  const v = findTag(fields, tag);
  return v && v.length >= 2 ? v.readUInt16BE(0) : defaultVal;
}

export function getUint32(fields, tag, defaultVal = 0) {
  const v = findTag(fields, tag);
  return v && v.length >= 4 ? v.readUInt32BE(0) : defaultVal;
}

export function getBool(fields, tag, defaultVal = false) {
  const v = findTag(fields, tag);
  return v && v.length >= 1 ? v[0] !== 0 : defaultVal;
}

export function getFunction(fields) {
  return getUint8(fields, TAG_FUNCTION, 0);
}

export function formatJsonForLogDisplay(text) {
  try {
    const obj = JSON.parse(text.trim());
    return JSON.stringify(obj, null, 2);
  } catch {
    return null;
  }
}

export function formatTlvFields(fields) {
  const lines = [];
  for (const [tag, val] of fields) {
    const tagName = TAG_NAMES.get(tag) ?? `0x${tag.toString(16).padStart(4, "0").toUpperCase()}`;

    if (tag === TAG_FUNCTION) {
      const fname =
        val.length === 1
          ? FUNC_NAMES.get(val[0]) ?? `0x${val[0].toString(16).padStart(2, "0")}`
          : val.toString("hex");
      lines.push(`  ${tagName.padEnd(22)}  = ${fname}`);
    } else if (
      [TAG_REGISTERED, TAG_REGISTER, TAG_PACKET_STREAM, TAG_ACK_STATUS, TAG_METER_FIX_BAUD].includes(
        tag
      )
    ) {
      const bval = val.length ? val[0] !== 0 : false;
      lines.push(`  ${tagName.padEnd(22)}  = ${bval}`);
    } else if (
      [TAG_TRANS_NUMBER, TAG_PULL_PORT, TAG_SERVER_PORT, TAG_PACKET_NUM].includes(tag)
    ) {
      const num = val.length >= 2 ? val.readUInt16BE(0) : Number(val.toString("hex") || 0);
      lines.push(`  ${tagName.padEnd(22)}  = ${num}`);
    } else if (tag === TAG_METER_INIT_BAUD) {
      const num = val.length >= 4 ? val.readUInt32BE(0) : 0;
      lines.push(`  ${tagName.padEnd(22)}  = ${num}`);
    } else if (tag === TAG_METER_INDEX) {
      lines.push(`  ${tagName.padEnd(22)}  = ${val.length ? val[0] : "?"}`);
    } else if (tag === TAG_ERROR_CODE) {
      const num = val.length >= 2 ? val.readInt16BE(0) : 0;
      lines.push(`  ${tagName.padEnd(22)}  = ${num}`);
    } else if (tag === TAG_DIRECTIVE_DATA) {
      const s = val.toString("utf8");
      const pj = formatJsonForLogDisplay(s);
      if (pj) {
        lines.push(`  ${tagName.padEnd(22)}  =`);
        for (const pl of pj.split("\n")) lines.push(`      ${pl}`);
      } else {
        const disp = s.length <= 400 ? s : `${s.slice(0, 397)}...`;
        lines.push(`  ${tagName.padEnd(22)}  = ${JSON.stringify(disp)}`);
      }
    } else {
      try {
        const s = val.toString("utf8");
        const jsonLike =
          tag === TAG_LOG_DATA ||
          tag === TAG_READOUT_DATA ||
          (s.length > 2 && ["{", "["].includes(s.trim().slice(0, 1)));
        if (jsonLike) {
          const pj = formatJsonForLogDisplay(s);
          if (pj) {
            lines.push(`  ${tagName.padEnd(22)}  =`);
            for (const pl of pj.split("\n")) lines.push(`      ${pl}`);
            continue;
          }
        }
        lines.push(`  ${tagName.padEnd(22)}  = "${s}"`);
      } catch {
        lines.push(`  ${tagName.padEnd(22)}  = [${val.toString("hex")}]`);
      }
    }
  }
  return lines.join("\n");
}

export function extractPackets(buf) {
  const packets = [];
  let b = buf;
  while (true) {
    const start = b.indexOf(PKT_START);
    if (start < 0) {
      return { packets, rest: Buffer.alloc(0) };
    }
    const end = b.indexOf(PKT_END, start + 1);
    if (end < 0) {
      return { packets, rest: b.subarray(start) };
    }
    packets.push(b.subarray(start, end + 1));
    b = b.subarray(end + 1);
  }
}

export function minifyJsonForDevice(text) {
  const obj = JSON.parse(text.trim());
  return JSON.stringify(obj);
}
