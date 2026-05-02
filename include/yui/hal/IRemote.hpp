#pragma once
// Browser-based remote viewer for the device. Hosts a tiny HTTP page on
// :80 and pushes frames + receives key events over WebSocket on :81.
//
// Native env injects a mock that records start/stop calls; ESP32 env wires
// in Esp32WebRemote which actually runs the listeners.

#include "yui/hal/IKeyboard.hpp"
#include <cstdint>

namespace yui {

class IRemote {
public:
  virtual ~IRemote() = default;

  // Start the HTTP + WS listeners. Returns false if WiFi isn't up or the
  // sockets can't bind. Idempotent.
  virtual bool start() = 0;

  // Stop both listeners and disconnect any client. Idempotent.
  virtual void stop() = 0;

  virtual bool running() const = 0;

  // Local IP as a NUL-terminated string in dotted form. Returns "" when
  // WiFi is down or the server hasn't been started.
  virtual const char* ip() const = 0;

  // Drain one key event from the browser-side queue. Returns false when
  // the queue is empty. Esp32Keyboard calls this before reading I²C.
  virtual bool poll_key(KeyEvent& out) = 0;
};

}  // namespace yui
