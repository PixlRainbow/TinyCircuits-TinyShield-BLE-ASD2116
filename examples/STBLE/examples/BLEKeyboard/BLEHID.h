#include "USBHID_Types.h"
#include "Keyboard_types.h"
#define HID_SERVICE_UUID          { 0x12, 0x18 }
#define HID_INFO_CHAR_UUID        { 0x4A, 0x2A }
#define HID_REPORT_MAP_CHAR_UUID  { 0x4B, 0x2A }
#define HID_PROTO_MODE_CHAR_UUID  { 0x4E, 0x2A }
#define HID_CTRL_PT_CHAR_UUID     { 0x4C, 0x2A }
#define HID_REPORT_CHAR_UUID      { 0x4D, 0x2A }
#define HID_REPORT_DESC_UUID      { 0x08, 0x29 }
#define HID_REPORT_MAP_CHAR_UUID  { 0x4B, 0x2A }

typedef struct {
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  flags;
} HID_information_t;

enum ReportType {
    INPUT_REPORT    = 0x1,
    OUTPUT_REPORT   = 0x2,
    FEATURE_REPORT  = 0x3,
};

enum ProtocolMode {
    BOOT_PROTOCOL   = 0x0,
    REPORT_PROTOCOL = 0x1,
};

typedef struct {
    uint8_t ID;
    uint8_t type;
} report_reference_t;
