#pragma once
#if defined(YUI_TARGET_CARDPUTER_ADV)

// Esp32Gnss — GPS feed from the TH-D75's onboard GNSS over BT-SPP.
//
// STUB: returns invalid fix until v0.2 Track A wires the NMEA parser.
// The eventual impl will share the IRadioLink with Esp32RadioLink and
// peel off NMEA sentences (the radio sends them when GP is enabled).
#include "yui/hal/IGnss.hpp"

namespace yui {

class Esp32Gnss : public IGnss {
public:
  GnssFix latest() override { return {}; }
  bool any_data_seen() const override { return false; }
};

}  // namespace yui
#endif
