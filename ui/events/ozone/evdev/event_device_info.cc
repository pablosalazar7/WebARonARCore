// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/event_device_info.h"

#include <linux/input.h>

#include "base/logging.h"
#include "base/threading/thread_restrictions.h"

#if !defined(EVIOCGMTSLOTS)
#define EVIOCGMTSLOTS(len) _IOC(_IOC_READ, 'E', 0x0a, len)
#endif

namespace ui {

namespace {

bool GetEventBits(int fd, unsigned int type, void* buf, unsigned int size) {
  if (ioctl(fd, EVIOCGBIT(type, size), buf) < 0) {
    PLOG(ERROR) << "EVIOCGBIT(" << type << ", " << size << ") on fd " << fd;
    return false;
  }

  return true;
}

bool GetPropBits(int fd, void* buf, unsigned int size) {
  if (ioctl(fd, EVIOCGPROP(size), buf) < 0) {
    PLOG(ERROR) << "EVIOCGPROP(" << size << ") on fd " << fd;
    return false;
  }

  return true;
}

bool GetAbsInfo(int fd, int code, struct input_absinfo* absinfo) {
  if (ioctl(fd, EVIOCGABS(code), absinfo)) {
    PLOG(ERROR) << "EVIOCGABS(" << code << ") on fd " << fd;
    return false;
  }
  return true;
}

// |request| needs to be the equivalent to:
// struct input_mt_request_layout {
//   uint32_t code;
//   int32_t values[num_slots];
// };
//
// |size| is num_slots + 1 (for code).
bool GetSlotValues(int fd, int32_t* request, unsigned int size) {
  size_t data_size = size * sizeof(*request);

  if (ioctl(fd, EVIOCGMTSLOTS(data_size), request) < 0) {
    PLOG(ERROR) << "EVIOCGMTSLOTS(" << request[0] << ") on fd " << fd;
    return false;
  }

  return true;
}

void AssignBitset(const unsigned long* src,
                  size_t src_len,
                  unsigned long* dst,
                  size_t dst_len) {
  memcpy(dst, src, std::min(src_len, dst_len) * sizeof(unsigned long));
  if (src_len < dst_len)
    memset(&dst[src_len], 0, (dst_len - src_len) * sizeof(unsigned long));
}

}  // namespace

EventDeviceInfo::EventDeviceInfo() {
  memset(ev_bits_, 0, sizeof(ev_bits_));
  memset(key_bits_, 0, sizeof(key_bits_));
  memset(rel_bits_, 0, sizeof(rel_bits_));
  memset(abs_bits_, 0, sizeof(abs_bits_));
  memset(msc_bits_, 0, sizeof(msc_bits_));
  memset(sw_bits_, 0, sizeof(sw_bits_));
  memset(led_bits_, 0, sizeof(led_bits_));
  memset(prop_bits_, 0, sizeof(prop_bits_));
  memset(abs_info_, 0, sizeof(abs_info_));
}

EventDeviceInfo::~EventDeviceInfo() {}

bool EventDeviceInfo::Initialize(int fd) {
  if (!GetEventBits(fd, 0, ev_bits_, sizeof(ev_bits_)))
    return false;

  if (!GetEventBits(fd, EV_KEY, key_bits_, sizeof(key_bits_)))
    return false;

  if (!GetEventBits(fd, EV_REL, rel_bits_, sizeof(rel_bits_)))
    return false;

  if (!GetEventBits(fd, EV_ABS, abs_bits_, sizeof(abs_bits_)))
    return false;

  if (!GetEventBits(fd, EV_MSC, msc_bits_, sizeof(msc_bits_)))
    return false;

  if (!GetEventBits(fd, EV_SW, sw_bits_, sizeof(sw_bits_)))
    return false;

  if (!GetEventBits(fd, EV_LED, led_bits_, sizeof(led_bits_)))
    return false;

  if (!GetPropBits(fd, prop_bits_, sizeof(prop_bits_)))
    return false;

  for (unsigned int i = 0; i < ABS_CNT; ++i)
    if (HasAbsEvent(i))
      if (!GetAbsInfo(fd, i, &abs_info_[i]))
        return false;

  int max_num_slots = GetAbsMtSlotCount();

  // |request| is MT code + slots.
  int32_t request[max_num_slots + 1];
  int32_t* request_code = &request[0];
  int32_t* request_slots = &request[1];
  for (unsigned int i = EVDEV_ABS_MT_FIRST; i <= EVDEV_ABS_MT_LAST; ++i) {
    if (!HasAbsEvent(i))
      continue;

    memset(request, 0, sizeof(request));
    *request_code = i;
    if (!GetSlotValues(fd, request, max_num_slots + 1))
      LOG(WARNING) << "Failed to get multitouch values for code " << i;

    std::vector<int32_t>* slots = &slot_values_[i - EVDEV_ABS_MT_FIRST];
    slots->assign(request_slots, request_slots + max_num_slots);
  }

  return true;
}

void EventDeviceInfo::SetEventTypes(const unsigned long* ev_bits, size_t len) {
  AssignBitset(ev_bits, len, ev_bits_, arraysize(ev_bits_));
}

void EventDeviceInfo::SetKeyEvents(const unsigned long* key_bits, size_t len) {
  AssignBitset(key_bits, len, key_bits_, arraysize(key_bits_));
}

void EventDeviceInfo::SetRelEvents(const unsigned long* rel_bits, size_t len) {
  AssignBitset(rel_bits, len, rel_bits_, arraysize(rel_bits_));
}

void EventDeviceInfo::SetAbsEvents(const unsigned long* abs_bits, size_t len) {
  AssignBitset(abs_bits, len, abs_bits_, arraysize(abs_bits_));
}

void EventDeviceInfo::SetMscEvents(const unsigned long* msc_bits, size_t len) {
  AssignBitset(msc_bits, len, msc_bits_, arraysize(msc_bits_));
}

void EventDeviceInfo::SetSwEvents(const unsigned long* sw_bits, size_t len) {
  AssignBitset(sw_bits, len, sw_bits_, arraysize(sw_bits_));
}

void EventDeviceInfo::SetLedEvents(const unsigned long* led_bits, size_t len) {
  AssignBitset(led_bits, len, led_bits_, arraysize(led_bits_));
}

void EventDeviceInfo::SetProps(const unsigned long* prop_bits, size_t len) {
  AssignBitset(prop_bits, len, prop_bits_, arraysize(prop_bits_));
}

void EventDeviceInfo::SetAbsInfo(unsigned int code,
                                 const input_absinfo& abs_info) {
  if (code > ABS_MAX)
    return;

  memcpy(&abs_info_[code], &abs_info, sizeof(abs_info));
}

void EventDeviceInfo::SetAbsMtSlots(unsigned int code,
                                    const std::vector<int32_t>& values) {
  DCHECK_EQ(GetAbsMtSlotCount(), values.size());
  int index = code - EVDEV_ABS_MT_FIRST;
  if (index < 0 || index >= EVDEV_ABS_MT_COUNT)
    return;
  slot_values_[index] = values;
}

void EventDeviceInfo::SetAbsMtSlot(unsigned int code,
                                   unsigned int slot,
                                   uint32_t value) {
  int index = code - EVDEV_ABS_MT_FIRST;
  if (index < 0 || index >= EVDEV_ABS_MT_COUNT)
    return;
  slot_values_[index][slot] = value;
}

bool EventDeviceInfo::HasEventType(unsigned int type) const {
  if (type > EV_MAX)
    return false;
  return EvdevBitIsSet(ev_bits_, type);
}

bool EventDeviceInfo::HasKeyEvent(unsigned int code) const {
  if (code > KEY_MAX)
    return false;
  return EvdevBitIsSet(key_bits_, code);
}

bool EventDeviceInfo::HasRelEvent(unsigned int code) const {
  if (code > REL_MAX)
    return false;
  return EvdevBitIsSet(rel_bits_, code);
}

bool EventDeviceInfo::HasAbsEvent(unsigned int code) const {
  if (code > ABS_MAX)
    return false;
  return EvdevBitIsSet(abs_bits_, code);
}

bool EventDeviceInfo::HasMscEvent(unsigned int code) const {
  if (code > MSC_MAX)
    return false;
  return EvdevBitIsSet(msc_bits_, code);
}

bool EventDeviceInfo::HasSwEvent(unsigned int code) const {
  if (code > SW_MAX)
    return false;
  return EvdevBitIsSet(sw_bits_, code);
}

bool EventDeviceInfo::HasLedEvent(unsigned int code) const {
  if (code > LED_MAX)
    return false;
  return EvdevBitIsSet(led_bits_, code);
}

bool EventDeviceInfo::HasProp(unsigned int code) const {
  if (code > INPUT_PROP_MAX)
    return false;
  return EvdevBitIsSet(prop_bits_, code);
}

int32_t EventDeviceInfo::GetAbsMinimum(unsigned int code) const {
  return abs_info_[code].minimum;
}

int32_t EventDeviceInfo::GetAbsMaximum(unsigned int code) const {
  return abs_info_[code].maximum;
}

int32_t EventDeviceInfo::GetAbsValue(unsigned int code) const {
  return abs_info_[code].value;
}

uint32_t EventDeviceInfo::GetAbsMtSlotCount() const {
  if (!HasAbsEvent(ABS_MT_SLOT))
    return 0;
  return GetAbsMaximum(ABS_MT_SLOT) + 1;
}

int32_t EventDeviceInfo::GetAbsMtSlotValue(unsigned int code,
                                           unsigned int slot) const {
  unsigned int index = code - EVDEV_ABS_MT_FIRST;
  DCHECK(index < EVDEV_ABS_MT_COUNT);
  return slot_values_[index][slot];
}

int32_t EventDeviceInfo::GetAbsMtSlotValueWithDefault(
    unsigned int code,
    unsigned int slot,
    int32_t default_value) const {
  if (!HasAbsEvent(code))
    return default_value;
  return GetAbsMtSlotValue(code, slot);
}

bool EventDeviceInfo::HasAbsXY() const {
  return HasAbsEvent(ABS_X) && HasAbsEvent(ABS_Y);
}

bool EventDeviceInfo::HasMTAbsXY() const {
  return HasAbsEvent(ABS_MT_POSITION_X) && HasAbsEvent(ABS_MT_POSITION_Y);
}

bool EventDeviceInfo::HasRelXY() const {
  return HasRelEvent(REL_X) && HasRelEvent(REL_Y);
}

bool EventDeviceInfo::HasMultitouch() const {
  return HasAbsEvent(ABS_MT_SLOT);
}

bool EventDeviceInfo::HasDirect() const {
  bool has_direct = HasProp(INPUT_PROP_DIRECT);
  bool has_pointer = HasProp(INPUT_PROP_POINTER);
  if (has_direct || has_pointer)
    return has_direct;

  switch (ProbeLegacyAbsoluteDevice()) {
    case LegacyAbsoluteDeviceType::LADT_TOUCHSCREEN:
      return true;

    case LegacyAbsoluteDeviceType::LADT_TABLET:
    case LegacyAbsoluteDeviceType::LADT_TOUCHPAD:
    case LegacyAbsoluteDeviceType::LADT_NONE:
      return false;
  }

  NOTREACHED();
  return false;
}

bool EventDeviceInfo::HasPointer() const {
  bool has_direct = HasProp(INPUT_PROP_DIRECT);
  bool has_pointer = HasProp(INPUT_PROP_POINTER);
  if (has_direct || has_pointer)
    return has_pointer;

  switch (ProbeLegacyAbsoluteDevice()) {
    case LegacyAbsoluteDeviceType::LADT_TOUCHPAD:
    case LegacyAbsoluteDeviceType::LADT_TABLET:
      return true;

    case LegacyAbsoluteDeviceType::LADT_TOUCHSCREEN:
    case LegacyAbsoluteDeviceType::LADT_NONE:
      return false;
  }

  NOTREACHED();
  return false;
}

bool EventDeviceInfo::HasStylus() const {
  return HasKeyEvent(BTN_TOOL_PEN) || HasKeyEvent(BTN_STYLUS) ||
         HasKeyEvent(BTN_STYLUS2);
}

bool EventDeviceInfo::HasKeyboard() const {
  if (!HasEventType(EV_KEY))
    return false;

  // Check first 31 keys: If we have all of them, consider it a full
  // keyboard. This is exactly what udev does for ID_INPUT_KEYBOARD.
  for (int key = KEY_ESC; key <= KEY_D; ++key)
    if (!HasKeyEvent(key))
      return false;

  return true;
}

bool EventDeviceInfo::HasMouse() const {
  return HasRelXY();
}

bool EventDeviceInfo::HasTouchpad() const {
  return HasAbsXY() && HasPointer() && !HasStylus();
}

bool EventDeviceInfo::HasTablet() const {
  return HasAbsXY() && HasPointer() && HasStylus();
}

bool EventDeviceInfo::HasTouchscreen() const {
  return HasAbsXY() && HasDirect();
}

EventDeviceInfo::LegacyAbsoluteDeviceType
EventDeviceInfo::ProbeLegacyAbsoluteDevice() const {
  if (!HasAbsXY())
    return LegacyAbsoluteDeviceType::LADT_NONE;

  if (HasStylus())
    return LegacyAbsoluteDeviceType::LADT_TABLET;

  if (HasKeyEvent(BTN_TOOL_FINGER) && HasKeyEvent(BTN_TOUCH))
    return LegacyAbsoluteDeviceType::LADT_TOUCHPAD;

  if (HasKeyEvent(BTN_TOUCH))
    return LegacyAbsoluteDeviceType::LADT_TOUCHSCREEN;

  return LegacyAbsoluteDeviceType::LADT_NONE;
}

}  // namespace ui
