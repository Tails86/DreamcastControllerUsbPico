#include "usb_execution.h"

#include "UsbControllerInterface.hpp"
#include "UsbGamepad.h"
#include "configuration.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "device/dcd.h"
#include "usb_descriptors.h"
#include "class/hid/hid_device.h"

bool usbEnabled = false;

UsbControllerInterface** pAllUsbDevices = nullptr;

uint8_t numUsbDevices = 0;

bool usbDisconnecting = false;
absolute_time_t usbDisconnectTime;

void set_usb_devices(UsbControllerInterface** devices, uint8_t n)
{
  pAllUsbDevices = devices;
  numUsbDevices = n;
}

bool gIsConnected = false;

void led_task()
{
  static bool ledOn = false;
  static uint32_t startMs = 0;
  if (usbDisconnecting)
  {
    // Currently counting down to disconnect; flash LED
    static const uint32_t BLINK_TIME_MS = 500;
    uint32_t t = board_millis() - startMs;
    if (t >= BLINK_TIME_MS)
    {
      startMs += BLINK_TIME_MS;
      ledOn = !ledOn;
    }
  }
  else
  {
    bool keyPressed = false;
    UsbControllerInterface** pdevs = pAllUsbDevices;
    for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
    {
      if ((*pdevs)->isButtonPressed())
      {
        keyPressed = true;
      }
    }
    if (gIsConnected)
    {
      // When connected, LED is ON only when no key is pressed
      ledOn = !keyPressed;
    }
    else
    {
      // When not connected, LED blinks when key is pressed
      static const uint32_t BLINK_TIME_MS = 100;
      uint32_t t = board_millis() - startMs;
      if (t >= BLINK_TIME_MS)
      {
        startMs += BLINK_TIME_MS;
        ledOn = !ledOn;
      }
      ledOn = ledOn && keyPressed;
    }
  }
  board_led_write(ledOn);
}

void usb_init()
{
  tusb_init();
}

void usb_task()
{
  tud_task(); // tinyusb device task
  led_task();
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  UsbControllerInterface** pdevs = pAllUsbDevices;
  for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
  {
    (*pdevs)->updateUsbConnected(true);
  }
  gIsConnected = true;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  UsbControllerInterface** pdevs = pAllUsbDevices;
  for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
  {
    (*pdevs)->updateUsbConnected(false);
  }
  gIsConnected = false;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  UsbControllerInterface** pdevs = pAllUsbDevices;
  for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
  {
    (*pdevs)->updateUsbConnected(false);
  }
  gIsConnected = false;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  UsbControllerInterface** pdevs = pAllUsbDevices;
  for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
  {
    (*pdevs)->updateUsbConnected(true);
  }
  gIsConnected = true;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  (void) report_id;
  (void) report_type;
  if (instance < numUsbDevices)
  {
    return 0;
  }
  else
  {
    // Build the report for the given report ID
    pAllUsbDevices[instance]->getReport(buffer, reqlen);
    // Return the size of the report
    return sizeof(hid_keyboard_report_t);
  }
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize)
{
  (void) instance;
  (void) report_type;

  // echo back anything we received from host
  tud_hid_report(report_id, buffer, bufsize);
}