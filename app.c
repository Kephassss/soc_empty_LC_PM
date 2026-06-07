#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sl_sensor_rht.h"
#include "temperature.h"
#include "stdio.h"
#include "gatt_db.h"
#include "sl_sleeptimer.h"
#include "sl_simple_led_instances.h"

#define IDENTIFIANT_SIGNAL_TIMER_TEMPERATURE 1

static uint8_t advertising_set_handle = 0xff;
static uint8_t identifiant_connexion_active = 0xff;

int32_t temp;
sl_sleeptimer_timer_handle_t gestionnaire_timer_temperature;

void app_process_action(void)
{
}

SL_WEAK void app_init(void)
{
  sl_simple_led_init_instances();
}

void fonction_rappel_timer_temperature(sl_sleeptimer_timer_handle_t *pointeur_timer, void *donnees_utilisateur)
{
    (void)pointeur_timer;
    (void)donnees_utilisateur;
    sl_bt_external_signal(IDENTIFIANT_SIGNAL_TIMER_TEMPERATURE);
}

void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {

    case sl_bt_evt_system_boot_id:
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle, sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);
      sc = sl_bt_advertiser_set_timing(advertising_set_handle, 160, 160, 0, 0);
      app_assert_status(sc);
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_connection_opened_id:
      identifiant_connexion_active = evt->data.evt_connection_opened.connection;
      break;

    case sl_bt_evt_connection_closed_id:
      sl_sleeptimer_stop_timer(&gestionnaire_timer_temperature);
      identifiant_connexion_active = 0xff;
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_temperature) {
        temp = demandetemp();
        int32_t temp_ble = temp / 10;
        uint16_t sent_len = 0;
        sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                       gattdb_temperature, 0, sizeof(temp_ble),
                                                       (uint8_t *)&temp_ble, &sent_len);
      }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature) {
        if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_notification) {
          sl_sleeptimer_start_periodic_timer_ms(&gestionnaire_timer_temperature, 1000, fonction_rappel_timer_temperature, NULL, 0, 0);
        } else {
          sl_sleeptimer_stop_timer(&gestionnaire_timer_temperature);
        }
      }
      break;

    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals == IDENTIFIANT_SIGNAL_TIMER_TEMPERATURE) {
        if (identifiant_connexion_active != 0xff) {
          temp = demandetemp();
          int16_t t_ble = (int16_t)(temp / 10);
          sl_bt_gatt_server_send_notification(identifiant_connexion_active, gattdb_temperature, sizeof(t_ble), (const uint8_t *)&t_ble);
          // Suppression du compteur_tics_timer car il n'est plus déclaré
          app_log_info("Temperature notifiée : %li degC\n", temp / 1000);
        }
      }
      break;

    case sl_bt_evt_gatt_server_user_write_request_id:
      if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_digital) {
        uint8_t val = evt->data.evt_gatt_server_user_write_request.value.data[0];
        for (uint8_t i = 0; i < SL_SIMPLE_LED_COUNT; i++) {
          val ? sl_led_turn_on(sl_simple_led_array[i]) : sl_led_turn_off(sl_simple_led_array[i]);
        }
        if (evt->data.evt_gatt_server_user_write_request.att_opcode == 0x12) {
          sl_bt_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection, gattdb_digital, 0);
        }
      }
      break;

    default:
      break;
  }
}
