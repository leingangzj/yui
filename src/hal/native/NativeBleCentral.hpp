#pragma once
#include "yui/hal/IBleCentral.hpp"
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace yui {

class FakeBleCentral : public IBleCentral {
public:
  // Test fixture: register a service tree. Tests call register_service
  // before exercising the app.
  struct CharSpec {
    std::string uuid;
    uint8_t     properties = 0;
    std::vector<uint8_t> value;
  };
  void register_service(const std::string& mac,
                        const std::string& service_uuid,
                        std::vector<CharSpec> chars) {
    db_[mac][service_uuid] = std::move(chars);
  }

  bool connect(const char* mac) override {
    if (!mac) return false;
    auto it = db_.find(mac);
    if (it == db_.end()) return false;
    last_mac_   = mac;
    connected_  = true;
    return true;
  }
  void disconnect() override { connected_ = false; }
  bool connected() const override { return connected_; }

  size_t enumerate_services(GattService* out, size_t cap) override {
    if (!connected_) return 0;
    auto it = db_.find(last_mac_);
    if (it == db_.end()) return 0;
    size_t n = 0;
    for (auto& kv : it->second) {
      if (n >= cap) break;
      std::strncpy(out[n].uuid, kv.first.c_str(), sizeof(out[n].uuid) - 1);
      out[n].uuid[sizeof(out[n].uuid) - 1] = '\0';
      ++n;
    }
    return n;
  }

  size_t enumerate_characteristics(const char* service_uuid,
                                   GattCharacteristic* out,
                                   size_t cap) override {
    if (!connected_ || !service_uuid) return 0;
    auto it = db_.find(last_mac_);
    if (it == db_.end()) return 0;
    auto sit = it->second.find(service_uuid);
    if (sit == it->second.end()) return 0;
    size_t n = 0;
    for (const auto& cs : sit->second) {
      if (n >= cap) break;
      std::strncpy(out[n].uuid, cs.uuid.c_str(), sizeof(out[n].uuid) - 1);
      out[n].uuid[sizeof(out[n].uuid) - 1] = '\0';
      out[n].properties = cs.properties;
      ++n;
    }
    return n;
  }

  int read_characteristic(const char* service_uuid,
                          const char* char_uuid,
                          uint8_t* out, size_t cap) override {
    if (!connected_) return -1;
    auto it = db_.find(last_mac_);
    if (it == db_.end()) return -1;
    auto sit = it->second.find(service_uuid);
    if (sit == it->second.end()) return -1;
    for (const auto& cs : sit->second) {
      if (cs.uuid == char_uuid) {
        const size_t n = (cs.value.size() < cap) ? cs.value.size() : cap;
        std::memcpy(out, cs.value.data(), n);
        return static_cast<int>(n);
      }
    }
    return -1;
  }

  const std::string& last_mac() const { return last_mac_; }

private:
  std::map<std::string,
    std::map<std::string, std::vector<CharSpec>>> db_;
  std::string last_mac_;
  bool        connected_ = false;
};

}  // namespace yui
