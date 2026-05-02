#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Browser-based remote viewer.
//
//   HTTP :80
//     GET /            → tiny single-page viewer (canvas + WS client)
//     GET /api/info    → {"ip":"...","fw":"...","w":240,"h":135}
//
//   WebSocket :81
//     server → client  : binary RGB565 frames (240×135×2 = 64,800 bytes
//                        prefixed by a 4-byte header [FRAME, w_lo, w_hi,
//                        seq])
//     client → server  : 4-byte key messages [KEY, key_code, modifiers,
//                        ascii] — 'down'-only for now (browsers fire
//                        synthetic press+release together for chord keys
//                        anyway)
//
// Mixed-content note: the GitHub Pages flasher is HTTPS, so it can only
// *link* to the device viewer (target=_blank into http://<ip>/). The viewer
// itself is hosted from this server; no cross-origin gymnastics.

#include "yui/hal/IRemote.hpp"
#include "yui/hal/IKeyboard.hpp"
#include "yui/types.hpp"
#include "yui/remote/viewer_html.hpp"

#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include <deque>
#include <cstdio>
#include <cstring>

namespace yui {

class Esp32WebRemote : public IRemote {
public:
  static constexpr uint16_t kHttpPort = 80;
  static constexpr uint16_t kWsPort   = 81;
  static constexpr uint32_t kFrameIntervalMs = 100;  // 10 fps cap
  static constexpr std::size_t kKeyInboxMax = 32;

  // Wire-format opcodes (byte 0 of each binary message).
  static constexpr uint8_t kMsgFrame = 0x01;
  static constexpr uint8_t kMsgKey   = 0x10;

  Esp32WebRemote() = default;

  // Pull the firmware version string in via setter so we don't bake the
  // global into the header.
  void set_version(const char* v) { fw_version_ = v ? v : ""; }
  void set_canvas(M5Canvas* c)    { canvas_ = c; }

  bool start() override {
    if (running_) return true;
    if (!WiFi.isConnected()) return false;

    http_.on("/",         [this]() { handle_root_(); });
    http_.on("/api/info", [this]() { handle_info_(); });
    http_.onNotFound      ([this]() { http_.send(404, "text/plain", "404"); });
    http_.begin(kHttpPort);

    ws_.begin();
    ws_.onEvent([this](uint8_t client, WStype_t type,
                       uint8_t* payload, size_t len) {
      ws_event_(client, type, payload, len);
    });

    snprintf(ip_buf_, sizeof(ip_buf_), "%s",
             WiFi.localIP().toString().c_str());
    running_ = true;
    Serial.printf("[remote] up: http://%s/  ws://%s:%u/\n",
                  ip_buf_, ip_buf_, kWsPort);
    return true;
  }

  void stop() override {
    if (!running_) return;
    ws_.disconnect();
    ws_.close();
    http_.stop();
    running_ = false;
    ip_buf_[0] = 0;
    last_client_ = 0xFF;
    key_inbox_.clear();
    Serial.println("[remote] down");
  }

  bool running() const override { return running_; }
  const char* ip() const override { return ip_buf_; }

  bool poll_key(KeyEvent& out) override {
    if (key_inbox_.empty()) return false;
    out = key_inbox_.front();
    key_inbox_.pop_front();
    return true;
  }

  // Service HTTP + WS in the main loop. Called every tick when running.
  void tick() {
    if (!running_) return;
    http_.handleClient();
    ws_.loop();
  }

  // Push the current sprite to the connected viewer. Throttled so we don't
  // saturate WiFi with 30 fps × 65 KB even when nobody is watching.
  void push_frame() {
    if (!running_ || !canvas_ || last_client_ == 0xFF) return;
    const uint32_t now = millis();
    if (now - last_frame_ms_ < kFrameIntervalMs) return;
    last_frame_ms_ = now;

    const int w = canvas_->width();
    const int h = canvas_->height();
    const std::size_t pixels = static_cast<std::size_t>(w) * h;
    const std::size_t frame_bytes = pixels * 2;

    // Header: [op, seq_lo, seq_hi, reserved] then RGB565 LE pixel data.
    const std::size_t total = 4 + frame_bytes;
    if (total > sizeof(scratch_)) return;  // belt-and-braces; sized for 240x135
    scratch_[0] = kMsgFrame;
    scratch_[1] = static_cast<uint8_t>(seq_ & 0xFF);
    scratch_[2] = static_cast<uint8_t>((seq_ >> 8) & 0xFF);
    scratch_[3] = 0;
    ++seq_;

    // Lovyan stores the sprite in 16-bit big-endian by default; ask for
    // a contiguous RGB565 LE copy so JS can ImageData-decode trivially.
    canvas_->readRect(0, 0, w, h,
                      reinterpret_cast<uint16_t*>(&scratch_[4]));

    ws_.sendBIN(last_client_, scratch_, total);
  }

private:
  void handle_root_() {
    http_.sendHeader("Cache-Control", "no-store");
    http_.send_P(200, "text/html", remote_viewer_html);
  }

  void handle_info_() {
    char buf[160];
    int n = snprintf(buf, sizeof(buf),
                     "{\"ip\":\"%s\",\"fw\":\"%s\",\"w\":%d,\"h\":%d,"
                     "\"ws_port\":%u}",
                     ip_buf_, fw_version_,
                     M5.Display.width(), M5.Display.height(), kWsPort);
    http_.sendHeader("Cache-Control", "no-store");
    http_.send(200, "application/json",
               n > 0 ? String(buf) : String("{}"));
  }

  void ws_event_(uint8_t client, WStype_t type,
                 uint8_t* payload, size_t len) {
    switch (type) {
      case WStype_CONNECTED:
        last_client_ = client;
        Serial.printf("[remote] client %u connected\n", client);
        break;
      case WStype_DISCONNECTED:
        if (client == last_client_) last_client_ = 0xFF;
        Serial.printf("[remote] client %u gone\n", client);
        break;
      case WStype_BIN:
        if (len >= 4 && payload[0] == kMsgKey) decode_key_(payload, len);
        break;
      default:
        break;
    }
  }

  // [op, key_code, modifiers, ascii]. modifiers bits: 0x01 shift, 0x02 ctrl,
  // 0x04 alt, 0x08 fn, 0x80 release-bit (else press).
  void decode_key_(const uint8_t* p, std::size_t len) {
    if (len < 4) return;
    KeyEvent ev{};
    ev.key   = static_cast<Key>(p[1]);
    const uint8_t mods = p[2];
    ev.shift = (mods & 0x01) != 0;
    ev.ctrl  = (mods & 0x02) != 0;
    ev.alt   = (mods & 0x04) != 0;
    ev.fn    = (mods & 0x08) != 0;
    ev.down  = (mods & 0x80) == 0;
    ev.ch    = static_cast<char>(p[3]);
    if (key_inbox_.size() >= kKeyInboxMax) key_inbox_.pop_front();
    key_inbox_.push_back(ev);
  }

  WebServer        http_{kHttpPort};
  WebSocketsServer ws_{kWsPort};
  M5Canvas*        canvas_   = nullptr;
  const char*      fw_version_ = "";
  bool             running_  = false;
  uint8_t          last_client_ = 0xFF;
  uint16_t         seq_ = 0;
  uint32_t         last_frame_ms_ = 0;
  char             ip_buf_[20] = {0};
  std::deque<KeyEvent> key_inbox_;

  // 240×135×2 + 4-byte header. Static so we don't malloc per frame.
  uint8_t          scratch_[4 + 240 * 135 * 2] = {0};
};

}  // namespace yui
#endif
