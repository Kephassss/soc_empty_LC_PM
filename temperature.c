#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sl_sensor_rht.h"


int demandetemp(){
  uint32_t rh;
  int32_t temp;

  sl_sensor_rht_init();
  sl_sensor_rht_get(&rh, &temp);
  sl_sensor_rht_deinit();
  return temp;
}
