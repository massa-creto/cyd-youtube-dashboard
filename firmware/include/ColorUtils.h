#pragma once
#include <Arduino.h>
#include <ctype.h>
#include <stdint.h>
uint32_t parseHex(const String& raw, uint32_t fallback) {
  String v = raw; v.trim(); if(v.startsWith("#")) v.remove(0,1);
  if(v.length()!=6) return fallback;
  for(size_t i=0;i<v.length();++i) if(!isxdigit((unsigned char)v[i])) return fallback;
  return strtoul(v.c_str(),nullptr,16);
}
String hexColor(uint32_t c) { char buf[8]; snprintf(buf,sizeof(buf),"#%06lX",(unsigned long)(c&0xFFFFFF)); return String(buf); }
uint16_t to565(uint32_t c) { return uint16_t(((c>>19)&31)<<11 | ((c>>10)&63)<<5 | ((c>>3)&31)); }
uint32_t from565(uint16_t c) { uint8_t r=((c>>11)&31)*255/31,g=((c>>5)&63)*255/63,b=(c&31)*255/31; return (uint32_t(r)<<16)|(uint32_t(g)<<8)|b; }
