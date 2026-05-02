#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32WifiAp — Cardputer SoftAP with integrated DNS responder + HTTP
// captive-portal server.
//
// DNS: bind UDP/53, parse the question (just to get the txn ID), reply
// with an A-record pointing at our own IP for ANY query. Phones detect
// the captive portal and pop the login UI.
//
// HTTP: WiFiServer on TCP/80. Any GET → serve the configured HTML; any
// POST → invoke the form callback with the request body.
//
// Background work (DNS poll, HTTP accept) runs from a small task spawned
// via xTaskCreate so the rest of the firmware doesn't have to pump it.
#include "yui/hal/IWifiAp.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdint>
#include <cstring>
#include <string>

namespace yui {

class Esp32WifiAp : public IWifiAp {
public:
  static Esp32WifiAp* instance() { return self_; }

  Esp32WifiAp() { self_ = this; }
  ~Esp32WifiAp() override { stop(); if (self_ == this) self_ = nullptr; }

  bool start_open(const char* ssid, uint8_t channel) override {
    if (active_) stop();
    if (!ssid) return false;
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(ssid, nullptr, channel)) return false;
    our_ip_ = WiFi.softAPIP();
    active_ = true;
    return true;
  }

  void stop() override {
    captive_ = false;
    if (task_) { vTaskDelete(task_); task_ = nullptr; }
    if (udp_)  { udp_->stop(); delete udp_; udp_ = nullptr; }
    if (http_) { http_->stop(); delete http_; http_ = nullptr; }
    if (active_) WiFi.softAPdisconnect(true);
    active_ = false;
    cb_ = nullptr; ctx_ = nullptr;
    html_.clear();
    dns_count_ = 0; http_count_ = 0;
  }

  bool active() const override { return active_; }
  size_t client_count() const override {
    return active_ ? WiFi.softAPgetStationNum() : 0;
  }

  bool start_captive(const char* html, CaptiveFormCb cb, void* ctx) override {
    if (!active_) return false;
    html_ = html ? html : "";
    cb_   = cb;
    ctx_  = ctx;
    udp_  = new WiFiUDP();
    http_ = new WiFiServer(80);
    if (!udp_->begin(53)) { delete udp_; udp_ = nullptr; return false; }
    http_->begin();
    captive_ = true;
    xTaskCreatePinnedToCore(&Esp32WifiAp::task_thunk_, "yui_cp",
                            6 * 1024, this, 1, &task_, 1);
    return true;
  }

  size_t dns_queries() const override { return dns_count_; }
  size_t http_requests() const override { return http_count_; }

private:
  static void task_thunk_(void* arg) {
    static_cast<Esp32WifiAp*>(arg)->loop_();
  }

  void loop_() {
    while (captive_) {
      pump_dns_();
      pump_http_();
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(nullptr);
  }

  void pump_dns_() {
    if (!udp_) return;
    int n = udp_->parsePacket();
    if (n <= 0) return;
    uint8_t buf[256];
    if (n > static_cast<int>(sizeof(buf))) n = sizeof(buf);
    udp_->read(buf, n);
    ++dns_count_;
    // Build a generic A-record response pointing at our IP.
    if (n < 12) return;
    // Mark response, copy ID + question
    uint8_t resp[256];
    int     rlen = 0;
    std::memcpy(resp, buf, n);  rlen = n;
    resp[2] = 0x81; resp[3] = 0x80;   // QR=1 RA=1
    resp[6] = 0; resp[7] = 1;         // ANCOUNT=1
    // Append answer: name pointer (0xC0 0x0C), type A (1), class IN (1),
    // TTL=60, RDLENGTH=4, RDATA=our IP
    if (rlen + 16 > static_cast<int>(sizeof(resp))) return;
    resp[rlen++] = 0xC0; resp[rlen++] = 0x0C;
    resp[rlen++] = 0x00; resp[rlen++] = 0x01;
    resp[rlen++] = 0x00; resp[rlen++] = 0x01;
    resp[rlen++] = 0x00; resp[rlen++] = 0x00; resp[rlen++] = 0x00; resp[rlen++] = 0x3C;
    resp[rlen++] = 0x00; resp[rlen++] = 0x04;
    resp[rlen++] = our_ip_[0]; resp[rlen++] = our_ip_[1];
    resp[rlen++] = our_ip_[2]; resp[rlen++] = our_ip_[3];
    udp_->beginPacket(udp_->remoteIP(), udp_->remotePort());
    udp_->write(resp, rlen);
    udp_->endPacket();
  }

  void pump_http_() {
    if (!http_) return;
    WiFiClient client = http_->available();
    if (!client) return;
    ++http_count_;
    String line;
    String first;
    while (client.connected() && client.available()) {
      char c = client.read();
      if (c == '\n') {
        if (line.length() == 0) break;     // end of headers
        if (first.length() == 0) first = line;
        line = "";
      } else if (c != '\r') {
        line += c;
      }
    }
    String body;
    if (first.startsWith("POST")) {
      body.reserve(256);
      while (client.available() && body.length() < 256) body += static_cast<char>(client.read());
      if (cb_) {
        const String ip_str = client.remoteIP().toString();
        cb_(ctx_, ip_str.c_str(), body.c_str());
      }
    }
    String resp =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html; charset=utf-8\r\n"
      "Connection: close\r\n\r\n";
    resp += html_.c_str();
    client.print(resp);
    delay(2);
    client.stop();
  }

  static Esp32WifiAp*  self_;
  bool                 active_      = false;
  bool                 captive_     = false;
  std::string          html_;
  CaptiveFormCb        cb_          = nullptr;
  void*                ctx_         = nullptr;
  WiFiUDP*             udp_         = nullptr;
  WiFiServer*          http_        = nullptr;
  TaskHandle_t         task_        = nullptr;
  IPAddress            our_ip_;
  size_t               dns_count_   = 0;
  size_t               http_count_  = 0;
};

inline Esp32WifiAp* Esp32WifiAp::self_ = nullptr;

}  // namespace yui
#endif
