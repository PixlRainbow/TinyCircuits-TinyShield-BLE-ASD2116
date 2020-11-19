#if BLE_DEBUG
#include <stdio.h>
#ifndef PRINTF
char sprintbuff[100];
#define PRINTF(...) {sprintf(sprintbuff,__VA_ARGS__);SerialMonitorInterface.print(sprintbuff);}
#endif
#else
#define PRINTF(...)
#endif

#include "BLEHID.h"

uint16_t HIDServHandle, HIDInfoCharHandle, ReportMapCharHandle, ProtoModeCharHandle, HIDCtrlPtCharHandle;
uint16_t InReportCharHandle, InReportDescHandle, OutReportCharHandle, OutReportDescHandle, FeatReportCharHandle, FeatReportDescHandle;

// "keys pressed" report
static uint8_t inputReportData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
// "keys released" report
static const uint8_t emptyInputReportData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
// LEDs report
static uint8_t outputReportData[] = { 0 };

uint8_t Add_HID_Service(void)
{
  tBleStatus ret;
  // uint8_t uuid[16];
  const uint8_t uuid[]                = HID_SERVICE_UUID;
  const uint8_t info_uuid[]           = HID_INFO_CHAR_UUID;
  const uint8_t report_map_uuid[]     = HID_REPORT_MAP_CHAR_UUID;
  const uint8_t protocol_mode_uuid[]  = HID_PROTO_MODE_CHAR_UUID;
  const uint8_t control_point_uuid[]  = HID_CTRL_PT_CHAR_UUID;
  const uint8_t report_uuid[]         = HID_REPORT_CHAR_UUID;
  const uint8_t report_desc_uuid[]    = HID_REPORT_DESC_UUID;
  const report_reference_t inputDesc  = { 0x00, INPUT_REPORT };
  const report_reference_t outputDesc = { 0x00, OUTPUT_REPORT };
  const report_reference_t featureDesc= { 0x00, FEATURE_REPORT };
  const static HID_information_t info = { HID_VERSION_1_11, 0x00, 0x03 };
  
  uint8_t serviceMaxAttributeRecord = 0;
  const ProtocolMode mode = REPORT_PROTOCOL;

  /* serviceMaxAttributeRecord = 1 for hid service itself +
   *                             2 for protocol mode characteristic +
   *                             4 for each input report characteristic( 2 for charac,1 for report ref desc, 1 for desc) +
   *                             3 for each output report characteristic +
   *                             3 for each feature report characteristic +
   *                             2 for report map characteristic +
   *                             1 for each external report reference descriptor +
   *                             3 for boot keyboard i/p report char, if supported +
   *                             2 for boot keyboard o/p report char, if supported +
   *                             3 for boot mouse i/p report char, if supported +
   *                             2 for hid information characteristic +
   *                             2 for hid control point characteristic
   * The maximum number of records required is dependent on the number of reports
   * the HID device has to expose. The number of HID services is also not limited
   * but multiple HID services should be used when the report descriptor is longer
   * than 512 octets as maybe the case in case of composite devices
   */
  serviceMaxAttributeRecord = 1
                            + 2
                            + 4
                            + 3
                            + 3
                            + 2
                            /* TODO: ext report reference descriptor, boot keyboard reports, etc*/
                            + 2
                            + 2;
  
  // COPY_HID_SERVICE_UUID(uuid);
  /* Add HID Services */
  ret = aci_gatt_add_serv(UUID_TYPE_16, uuid, PRIMARY_SERVICE, serviceMaxAttributeRecord, &HIDServHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add from Protocol mode characteristic */
  ret = aci_gatt_add_char(HIDServHandle, UUID_TYPE_16, protocol_mode_uuid, 1,
                           (CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RESP), ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                           10, CHAR_VALUE_LEN_CONSTANT, &ProtoModeCharHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Set value for Protocol mode characteristic */
  ret = aci_gatt_update_char_value(HIDServHandle, ProtoModeCharHandle, 0, 1, &mode);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add Input Report Characteristic */
  ret = aci_gatt_add_char(HIDServHandle, UUID_TYPE_16, report_uuid, sizeof(inputReportData),
                           (CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY), ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                           10, CHAR_VALUE_LEN_VARIABLE, &InReportCharHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add Input Report Descriptor */
  ret = aci_gatt_add_char_desc(HIDServHandle, InReportCharHandle, UUID_TYPE_16, report_desc_uuid,
                              sizeof(report_reference_t), sizeof(report_reference_t), &inputDesc,
                              ATTR_PERMISSION_NONE, ATTR_ACCESS_READ_ONLY, GATT_DONT_NOTIFY_EVENTS,
                              10, CHAR_VALUE_LEN_CONSTANT, &InReportDescHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add Output Report Characteristic */
  ret = aci_gatt_add_char(HIDServHandle, UUID_TYPE_16, report_uuid, sizeof(outputReportData),
                           (CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP), ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                           10, CHAR_VALUE_LEN_VARIABLE, &OutReportCharHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add Output Report Descriptor */
  ret = aci_gatt_add_char_desc(HIDServHandle, OutReportCharHandle, UUID_TYPE_16, report_desc_uuid,
                              sizeof(report_reference_t), sizeof(report_reference_t), &outputDesc,
                              ATTR_PERMISSION_NONE, ATTR_ACCESS_READ_ONLY, GATT_DONT_NOTIFY_EVENTS,
                              10, CHAR_VALUE_LEN_CONSTANT, &OutReportDescHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add Feature Report Characteristic */
  ret = aci_gatt_add_char(HIDServHandle, UUID_TYPE_16, report_uuid, 0,
                           (CHAR_PROP_READ | CHAR_PROP_WRITE), ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                           10, CHAR_VALUE_LEN_VARIABLE, &FeatReportCharHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add Feature Report Descriptor */
  ret = aci_gatt_add_char_desc(HIDServHandle, FeatReportCharHandle, UUID_TYPE_16, report_desc_uuid,
                              sizeof(report_reference_t), sizeof(report_reference_t), &featureDesc,
                              ATTR_PERMISSION_NONE, ATTR_ACCESS_READ_ONLY, GATT_DONT_NOTIFY_EVENTS,
                              10, CHAR_VALUE_LEN_CONSTANT, &FeatReportDescHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add Report Map Characteristic */
  ret = aci_gatt_add_char(HIDServHandle, UUID_TYPE_16, report_map_uuid, sizeof(KEYBOARD_REPORT_MAP),
                           (CHAR_PROP_READ), ATTR_PERMISSION_NONE, GATT_DONT_NOTIFY_EVENTS,
                           10, CHAR_VALUE_LEN_VARIABLE, &ReportMapCharHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Set value for Report Map characteristic */
  ret = aci_gatt_update_char_value(HIDServHandle, ReportMapCharHandle, 0, sizeof(KEYBOARD_REPORT_MAP), KEYBOARD_REPORT_MAP);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add HID Information Characteristic */
  ret = aci_gatt_add_char(HIDServHandle, UUID_TYPE_16, info_uuid, sizeof(HID_information_t),
                           (CHAR_PROP_READ), ATTR_PERMISSION_NONE, GATT_DONT_NOTIFY_EVENTS,
                           10, CHAR_VALUE_LEN_VARIABLE, &HIDInfoCharHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Set value for HID Information characteristic */
  ret = aci_gatt_update_char_value(HIDServHandle, HIDInfoCharHandle, 0, sizeof(HID_information_t), &info);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* Add HID Control Point Characteristic */
  ret = aci_gatt_add_char(HIDServHandle, UUID_TYPE_16, control_point_uuid, 1,
                           (CHAR_PROP_WRITE_WITHOUT_RESP), ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                           10, CHAR_VALUE_LEN_CONSTANT, &HIDCtrlPtCharHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  return BLE_STATUS_SUCCESS;

  fail:
    PRINTF("Error while adding HID service.\n");
    return BLE_STATUS_ERROR ;
}
