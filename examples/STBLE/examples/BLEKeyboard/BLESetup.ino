#if BLE_DEBUG
#include <stdio.h>
#ifndef PRINTF
char sprintbuff[100];
#define PRINTF(...) {sprintf(sprintbuff,__VA_ARGS__);SerialMonitorInterface.print(sprintbuff);}
#endif
#else
#define PRINTF(...)
#endif

volatile uint8_t set_connectable = 1;
uint16_t connection_handle = 0;


#define  ADV_INTERVAL_MIN_MS  50
#define  ADV_INTERVAL_MAX_MS  100


int connected = FALSE;

int BLEsetup()
{
  int ret;

  HCI_Init();
  /* Init SPI interface */
  BNRG_SPI_Init();
  /* Reset BlueNRG/BlueNRG-MS SPI interface */
  BlueNRG_RST();

  uint8_t bdaddr[] = {0x12, 0x34, 0x00, 0xE1, 0x80, 0x02};

  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bdaddr);

  if (ret) {
    PRINTF("Setting BD_ADDR failed.\n");
  }

  ret = aci_gatt_init();

  if (ret) {
    PRINTF("GATT_Init failed.\n");
  }

  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
  ret = aci_gap_init_IDB05A1(GAP_PERIPHERAL_ROLE_IDB05A1, 0, 0x07, &service_handle, &dev_name_char_handle, &appearance_char_handle);

  if (ret) {
    PRINTF("GAP_Init failed.\n");
  }

  const char *name = "BlueKB";

  ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, strlen(name), (uint8_t *)name);

  if (ret) {
    PRINTF("aci_gatt_update_char_value failed.\n");
  } else {
    PRINTF("BLE Stack Initialized.\n");
  }

  ret = Add_UART_Service();

  if (ret == BLE_STATUS_SUCCESS) {
    PRINTF("UART service added successfully.\n");
  } else {
    PRINTF("Error while adding UART service.\n");
  }

  ret = Add_HID_Service();

  if (ret == BLE_STATUS_SUCCESS) {
    PRINTF("HID service added successfully.\n");
  } else {
    PRINTF("Error while adding HID service.\n");
  }

  /* +4 dBm output power */
  ret = aci_hal_set_tx_power_level(1, 3);
  /* Configure security - Required by Android */
  ret = aci_gap_set_io_capability(IO_CAP_DISPLAY_ONLY);
  if(ret != BLE_STATUS_SUCCESS) PRINTF("Error configuring connection security.\n");
  ret = aci_gap_set_auth_requirement(MITM_PROTECTION_REQUIRED,
                                     OOB_AUTH_DATA_ABSENT,
                                     NULL,
                                     MIN_ENCRY_KEY_SIZE,
                                     MAX_ENCRY_KEY_SIZE,
                                     DONOT_USE_FIXED_PIN_FOR_PAIRING,
                                     123456,
                                     BONDING);
  if(ret != BLE_STATUS_SUCCESS) PRINTF("Error configuring connection security.\n");

  loadRandomSeed();
}

void loadRandomSeed()
{
  // initialize Pseudo RNG with seed from Hardware RNG
  uint8_t rand_seq[8];
  hci_le_rand(rand_seq);
  uint16_t rand_xor = *(uint16_t *)&rand_seq[0] ^ *(uint16_t *)&rand_seq[2] ^
                      *(uint16_t *)&rand_seq[4] ^ *(uint16_t *)&rand_seq[6];
  randomSeed(rand_xor);
}

void aci_loop() {
  HCI_Process();
  ble_connection_state = connected;
  if (set_connectable) {
    setConnectable();
    print_bonded_devices();
    set_connectable = 0;
  }
  if (HCI_Queue_Empty()) {
    //Enter_LP_Sleep_Mode();
  }
}

void print_bonded_devices()
{
  tBleStatus ret;
  uint8_t num_devices = 0, device_list[12*7];
  ret = aci_gap_get_bonded_devices(&num_devices, device_list, sizeof(device_list));
  PRINTF("List Bonded Devices:\n");
  for(int i = 0; i < num_devices * 7; i+=7){
    uint8_t addr_type = device_list[i];
    uint8_t *addr = &device_list[i+1];
    PRINTF("Type: %d, Addr: %02X%02X%02X%02X%02X%02X\n",addr_type,addr[5],addr[4],addr[3],addr[2],addr[1],addr[0]);
  }
}

void Read_Request_CB(uint16_t handle)
{
  /*if(handle == UARTTXCharHandle + 1)
    {

    }
    else if(handle == UARTRXCharHandle + 1)
    {


    }*/

  if (connection_handle != 0)
    aci_gatt_allow_read(connection_handle);
}


void setConnectable(void)
{
  tBleStatus ret;

  const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME, 'B', 'l', 'u', 'e', 'K', 'B'};
  uint8_t num_devices = 0, device_list[12*7];

  hci_le_set_scan_resp_data(0, NULL);

  ret = aci_gap_get_bonded_devices(&num_devices, device_list, sizeof(device_list));
  if(ret == BLE_STATUS_SUCCESS && num_devices > 0) {
    PRINTF("Paired device found. Connectable non-advertise.\n");
    ret = aci_gap_set_undirected_connectable(STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE);
  }
  else {
    PRINTF("General Discoverable Mode.\n");

    ret = aci_gap_set_discoverable(ADV_IND,
                                   (ADV_INTERVAL_MIN_MS * 1000) / 625, (ADV_INTERVAL_MAX_MS * 1000) / 625,
                                   STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE,
                                   sizeof(local_name), local_name, 0, NULL, 0, 0);
  }

  if (ret != BLE_STATUS_SUCCESS)
    PRINTF("%d\n", (uint8_t)ret);

}

//void Attribute_Modified_CB(uint16_t handle, uint8_t data_length, uint8_t *att_data)
//{
//  if (handle == UARTTXCharHandle + 1) {
//    int i;
//    for (i = 0; i < data_length; i++) {
//      ble_rx_buffer[i] = att_data[i];
//    }
//    ble_rx_buffer[i] = '\0';
//    ble_rx_buffer_len = data_length;
//  }
//}

void GAP_ConnectionComplete_CB(uint8_t addr[6], uint16_t handle) {

  connected = TRUE;
  connection_handle = handle;

  PRINTF("Connected to device:");
  for (int i = 5; i > 0; i--) {
    PRINTF("%02X-", addr[i]);
  }
  PRINTF("%02X\r\n", addr[0]);

  aci_gap_slave_security_request(connection_handle, BONDING, MITM_PROTECTION_REQUIRED);
}

void GAP_DisconnectionComplete_CB(void) {
  connected = FALSE;
  PRINTF("Disconnected\n");
  /* Make the device connectable again. */
  set_connectable = TRUE;
}



void HCI_Event_CB(void *pckt)
{
  hci_uart_pckt *hci_pckt = (hci_uart_pckt *)pckt;
  hci_event_pckt *event_pckt = (hci_event_pckt*)hci_pckt->data;

  if (hci_pckt->type != HCI_EVENT_PKT)
    return;

  switch (event_pckt->evt) {

    case EVT_DISCONN_COMPLETE:
      {
        //evt_disconn_complete *evt = (void *)event_pckt->data;
        GAP_DisconnectionComplete_CB();
      }
      break;

    case EVT_LE_META_EVENT:
      {
        evt_le_meta_event *evt = (evt_le_meta_event *)event_pckt->data;

        switch (evt->subevent) {
          case EVT_LE_CONN_COMPLETE:
            {
              evt_le_connection_complete *cc = (evt_le_connection_complete *)evt->data;
              GAP_ConnectionComplete_CB(cc->peer_bdaddr, cc->handle);
            }
            break;
        }
      }
      break;

    case EVT_VENDOR:
      {
        evt_blue_aci *blue_evt = (evt_blue_aci *)event_pckt->data;
        switch (blue_evt->ecode) {

          case EVT_BLUE_GATT_READ_PERMIT_REQ:
            {
              evt_gatt_read_permit_req *pr = (evt_gatt_read_permit_req *)blue_evt->data;
              Read_Request_CB(pr->attr_handle);
            }
            break;

          case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
            {
              evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)blue_evt->data;
              Attribute_Modified_CB(evt->attr_handle, evt->data_length, evt->att_data);
            }
            break;
          case EVT_BLUE_GAP_PASS_KEY_REQUEST:
            {
              evt_gap_pass_key_req *pkr = (evt_gap_pass_key_req *)blue_evt->data;
              // generate pseudo random integer, 0-999999 (max 6 digits for PIN)
              long rand_n = random(999999);
              // display PIN on screen
              char rand_txt[7];
              snprintf(rand_txt, sizeof(rand_txt), "%d", rand_n);
              display.clearScreen();
              writeTextCustom(rand_txt, liberationSans_16ptFontInfo, 0xFF, 0xFF, TS_8b_Green, TS_8b_Black);
              // configure security subsystem with PIN
              PRINTF("Generated PIN: %d\n", rand_n);
              uint8_t retval = aci_gap_pass_key_response(pkr->conn_handle, rand_n);
              PRINTF("Return: 0x%02X\n",retval);
              // lock brightness and disable buttons
              display.setBrightness(10);
              disable_buttons = true;
            }
            break;
          case EVT_BLUE_GAP_LIMITED_DISCOVERABLE:
          case EVT_BLUE_GAP_PAIRING_CMPLT:
            {
              disable_buttons = false;
              writeText();
            }
            break;
        }
      }
      break;
  }

}
