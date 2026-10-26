#!/usr/bin/env python3
"""Exercise the actual BMP280 driver using a deterministic I2C fixture."""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="homehub-environment-") as directory:
    folder = pathlib.Path(directory)
    (folder / "Arduino.h").write_text("""
#pragma once
#include <stdint.h>
#include <stddef.h>
constexpr int D4=4, D5=5;
extern uint32_t ticks;
inline uint32_t millis() { return ticks; }
""")
    (folder / "Wire.h").write_text("""
#pragma once
#include <stdint.h>
#include <stddef.h>
struct FakeWire {
  uint8_t registers[256]{};
  bool connected=true;
  int reg=0, cursor=0, remaining=0, writes=0, timeout=0, reads=0;
  void begin(int sda,int scl) {}
  void setTimeOut(int value) { timeout=value; }
  void beginTransmission(int address) { writes=0; }
  void write(uint8_t value) {
    if(writes++ == 0) reg=value; else registers[reg++]=value;
  }
  int endTransmission(bool stop=true) { return connected?0:4; }
  size_t requestFrom(int address,int count) {
    ++reads; cursor=reg; remaining=connected?count:0; return remaining;
  }
  int available() { return remaining; }
  int read() { --remaining; return registers[cursor++]; }
};
extern FakeWire Wire;
""")
    (folder / "test.cpp").write_text("""
#include "HomeHubEnvironment.h"
#include "Wire.h"
#include <cassert>
#include <cmath>
FakeWire Wire;
uint32_t ticks=0;
void raw(int reg,int value) {
  Wire.registers[reg]=value>>12;
  Wire.registers[reg+1]=value>>4;
  Wire.registers[reg+2]=(value&15)<<4;
}
int main() {
  Wire.registers[0xD0]=0x58;
  const int calibration[]={27504,26435,-1000,36477,-10685,3024,2855,140,-7,15500,-14600,6000};
  for(int i=0;i<12;++i) {
    const uint16_t word=calibration[i];
    Wire.registers[0x88+2*i]=word&255;
    Wire.registers[0x89+2*i]=word>>8;
  }
  raw(0xF7,415148); raw(0xFA,519888);
  HomeHubEnvironment::begin();
  assert(Wire.timeout==50 && Wire.registers[0xF4]==0x27);
  assert(!HomeHubEnvironment::reading().ready);
  ticks=1999; HomeHubEnvironment::poll();
  assert(!HomeHubEnvironment::reading().ready);
  ticks=2000; HomeHubEnvironment::poll();
  assert(HomeHubEnvironment::reading().ready);
  assert(fabs(HomeHubEnvironment::reading().temperatureC-25.08)<0.01);
  assert(fabs(HomeHubEnvironment::reading().pressureHpa-1006.53)<0.02);
  Wire.connected=false;
  ticks=4000; HomeHubEnvironment::poll();
  assert(!HomeHubEnvironment::reading().ready);
  const int attempts=Wire.reads;
  ticks=5000; HomeHubEnvironment::poll();
  assert(Wire.reads==attempts);
  Wire.connected=true;
  ticks=14000; HomeHubEnvironment::poll();
  assert(!HomeHubEnvironment::reading().ready);
  ticks=16000; HomeHubEnvironment::poll();
  assert(HomeHubEnvironment::reading().ready);
  raw(0xFA,0x80000);
  ticks=18000; HomeHubEnvironment::poll();
  assert(!HomeHubEnvironment::reading().ready);
  Wire.registers[0xD0]=0x60; // Do not treat a BME280 as the verified BMP280.
  HomeHubEnvironment::begin();
  ticks=30000; HomeHubEnvironment::poll();
  assert(!HomeHubEnvironment::reading().ready);
}
""")
    executable = folder / "test"
    subprocess.run(["c++", "-std=c++17", "-fsanitize=undefined", "-fno-sanitize-recover=all",
                    "-I", str(folder), "-I", str(ROOT / "firmware/HomeHub/src"),
                    str(folder / "test.cpp"), str(ROOT / "firmware/HomeHub/src/HomeHubEnvironment.cpp"),
                    "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("PASS: compensation, polling interval, unavailable/recovery, invalid sample and chip rejection")
