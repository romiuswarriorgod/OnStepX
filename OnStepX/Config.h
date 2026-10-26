

/* ---------------------------------------------------------------------------------------------------------------------------------
 * Configuration for OnStepX - МАКСИМАЛЬНО ПОЛНЫЙ МОНОЛИТНЫЙ ФАЙЛ ДЛЯ FYSETC E4
 * =================================================================================================================================
 * ВСЕ НАСТРОЙКИ ПИНОВ ДЛЯ ДАТЧИКОВ, ДЖОЙСТИКА И GPS ВШИТЫ СЮДА ДЛЯ ИСКЛЮЧЕНИЯ КОНФЛИКТОВ
 */

// =================================================================================================================================
// CONTROLLER & GENERAL CONFIG =====================================================================================================

#define THERMISTOR1_TNOM 25
#define THERMISTOR1_RNOM 10000
#define THERMISTOR1_BETA 3950
#define THERMISTOR1_RSERIES 4700
#define SERIAL_B_ESP32_BT ON
#define MNT_NAME "OnStep"

#define LIMIT_SENSE_PIN AUTO
#define LIMIT_SENSE HIGH
#define SERIAL_TMC_RX 34 // Перенос RX драйверов на Y-MIN для диагностики status_on

#define SERIAL_DEBUG_BAUD 115200
#define DISPLAY_WEATHER OFF
#define DISPLAY_INTERNAL_TEMPERATURE ON

// Указываем базовый пинмап платы
#define PINMAP FYSETC_E4

// Каналы связи
#define SERIAL_A_BAUD_DEFAULT 115200
#define SERIAL_B_BAUD_DEFAULT OFF
#define SERIAL_B_ESP_FLASHING OFF
#define SERIAL_C_BAUD_DEFAULT OFF
#define SERIAL_D_BAUD_DEFAULT OFF
#define SERIAL_E_BAUD_DEFAULT OFF
#define SERIAL_RADIO WIFI_ACCESS_POINT // Включение встроенного Wi-Fi ESP32

#define STATUS_LED ON
#define RETICLE_LED_DEFAULT OFF
#define RETICLE_LED_MEMORY OFF
#define RETICLE_LED_INVERT OFF
#define WEATHER OFF
#define STEP_WAVE_FORM PULSE
#define NV_DRIVER NV_DEFAULT

// =================================================================================================================================
// MOUNT & MOTOR AXES CONFIGURATION ================================================================================================

#define MOUNT_TYPE ALTAZM // Режим Добсона (Азимут / Высота)

// --- AXIS1: ОСЬ АЗИМУТА (ПОВОРОТ ВЛЕВО / ВПРАВО) ---------------------------------------------------------------------------------
#define AXIS1_DRIVER_MODEL TMC2209
#define AXIS1_STEPS_PER_DEGREE 25600.00000 // Нуждается в индивидуальном расчете под ваши редукторы!
#define AXIS1_REVERSE OFF
#define AXIS1_LIMIT_MIN -170 // Программный лимит вращения влево (градусы)
#define AXIS1_LIMIT_MAX 170  // Программный лимит вращения вправо (градусы)

#define AXIS1_DRIVER_MICROSTEPS 64
#define AXIS1_DRIVER_MICROSTEPS_GOTO 4
#define AXIS1_DRIVER_IHOLD 800
#define AXIS1_DRIVER_IRUN 1100
#define AXIS1_DRIVER_IGOTO OFF
#define AXIS1_DRIVER_STATUS ON
#define AXIS1_DRIVER_DECAY OFF
#define AXIS1_DRIVER_DECAY_GOTO OFF
#define AXIS1_POWER_DOWN OFF

// Жесткое принудительное назначение оптического датчика Home Азимута на разъем Y-MIN

#define AXIS1_SENSE_HOME HIGH
#define AXIS1_SENSE_HOME_PIN 15
#define AXIS1_SENSE_LIMIT_MIN OFF
#define AXIS1_SENSE_LIMIT_MAX OFF

// --- AXIS2: ОСЬ ВЫСОТЫ (НАКЛОН ВВЕРХ / ВНИЗ) -------------------------------------------------------------------------------------
#define AXIS2_DRIVER_MODEL TMC2209
#define AXIS2_STEPS_PER_DEGREE 256000.00000 // Нуждается в индивидуальном расчете под ваши редукторы!
#define AXIS2_REVERSE OFF
#define AXIS2_LIMIT_MIN 0  // Программный лимит горизонта (0 градусов)
#define AXIS2_LIMIT_MAX 80 // Программный лимит зенита (85 градусов)

#define AXIS2_DRIVER_MICROSTEPS 64
#define AXIS2_DRIVER_MICROSTEPS_GOTO 4
#define AXIS2_DRIVER_IHOLD 800
#define AXIS2_DRIVER_IRUN 1100
#define AXIS2_DRIVER_IGOTO OFF
#define AXIS2_DRIVER_STATUS ON
#define AXIS2_DRIVER_DECAY OFF
#define AXIS2_DRIVER_DECAY_GOTO OFF
#define AXIS2_POWER_DOWN OFF

// Жесткое принудительное назначение оптического датчика Home Высоты на разъем E-MIN (FAN0)

#define AXIS2_SENSE_HOME HIGH
#define AXIS2_SENSE_HOME_PIN 35
#define AXIS2_SENSE_LIMIT_MIN OFF
#define AXIS2_SENSE_LIMIT_MAX OFF

#define TIME_LOCATION_SOURCE GPS
#define SERIAL_GPS Serial2
#define SERIAL_GPS_RX 22       // Переносим на чистый пин 22 (SCL разъема I2C)
#define SERIAL_GPS_TX 21       // Пин SDA (даже если не подключен, OnStep инициализирует порт на этой паре)
#define SERIAL_GPS_BAUD 115200 // Родная скорость вашего чипа BE-220
#define TIME_LOCATION_PPS_SENSE OFF

#define DISPLAY_TYPE OFF // Екран вимкнено

// Отключаем внутренний опрос термодатчиков, чтобы они не читали движения джойстика как температуру
#define FOCUSER_TEMPERATURE 0

// Полностью выключаем все грелки (Dew Heaters), чтобы освободить GPIO2 под кнопку джойстика
#define FEATURE1_PURPOSE OFF
#define FEATURE1_PIN OFF
#define FEATURE2_PURPOSE OFF
#define FEATURE2_PIN OFF
#define FEATURE3_PURPOSE OFF
#define FEATURE3_PIN OFF
#define FEATURE4_PURPOSE OFF
#define FEATURE4_PIN OFF
#define FEATURE5_PURPOSE OFF
#define FEATURE5_PIN OFF
#define FEATURE6_PURPOSE OFF
#define FEATURE6_PIN OFF
#define FEATURE7_PURPOSE OFF
#define FEATURE7_PIN OFF
#define FEATURE8_PURPOSE OFF
#define FEATURE8_PIN OFF

// =================================================================================================================================
// MOUNT BEHAVIOR & LIMITS =========================================================================================================

#define MOUNT_COORDS TOPOCENTRIC
#define MOUNT_COORDS_MEMORY OFF
#define MOUNT_ENABLE_IN_STANDBY OFF

#define STATUS_MOUNT_LED OFF
#define STATUS_BUZZER OFF
#define STATUS_BUZZER_DEFAULT OFF
#define STATUS_BUZZER_MEMORY ON

#define ST4_INTERFACE OFF
#define ST4_HAND_CONTROL OFF
#define ST4_HAND_CONTROL_FOCUSER OFF

#define GUIDE_TIME_LIMIT 20
#define GUIDE_DISABLE_BACKLASH OFF
#define LIMIT_STRICT OFF

#define PARK_SENSE OFF
#define PARK_SIGNAL OFF
#define PARK_STATUS OFF
#define PARK_STRICT OFF

#define PEC_STEPS_PER_WORM_ROTATION 12800
#define PEC_SENSE OFF
#define PEC_BUFFER_SIZE_LIMIT 720

#define TRACK_BACKLASH_RATE 20
#define TRACK_AUTOSTART OFF
#define TRACK_COMPENSATION_DEFAULT OFF
#define TRACK_COMPENSATION_MEMORY OFF

#define SLEW_RATE_BASE_DESIRED 2.5
#define SLEW_RATE_MEMORY ON
#define SLEW_ACCELERATION_DIST 5.0
#define SLEW_RAPID_STOP_DIST 2.0
#define GOTO_FEATURE ON
#define GOTO_OFFSET 0.25
#define GOTO_OFFSET_ALIGN OFF

#define MFLIP_SKIP_HOME OFF
#define MFLIP_AUTOMATIC_DEFAULT OFF
#define MFLIP_AUTOMATIC_MEMORY OFF
#define MFLIP_PAUSE_HOME_DEFAULT OFF
#define MFLIP_PAUSE_HOME_MEMORY OFF

#define PIER_SIDE_SYNC_CHANGE_SIDES OFF
#define PIER_SIDE_PREFERRED_DEFAULT EAST
#define PIER_SIDE_PREFERRED_MEMORY OFF

#define ALIGN_AUTO_HOME OFF
#define ALIGN_MODEL_MEMORY OFF
#define ALIGN_MAX_STARS AUTO

// Дополнительное оборудование (выключено)
#define AXIS3_DRIVER_MODEL OFF
#define AXIS4_DRIVER_MODEL OFF
#define AXIS5_DRIVER_MODEL OFF

#define FEATURE2_TEMP OFF
#define FEATURE2_SENSE_PIN OFF
#define FEATURE2_SENSE OFF

#define DEBUG_STICK OFF

// ---------------------------------------------------------------------------------------------------------------------------------
#define FileVersionConfig 6
#include "Extended.config.h" // Подключение финальных математических макросов ядра
