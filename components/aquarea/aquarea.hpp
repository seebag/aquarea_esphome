#pragma once

#include <array>
#include <functional>
#include <vector>
#include <cstdint>
#include <queue>
#include "esphome/core/hal.h"
#include "esphome/core/gpio.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace aquarea {

enum Registers
{
  Defrost = 17, // -> 0x02 booster
  OutsideTemp = 18,
  OutletTemp = 19,
  ErrorCode =  20,
  InletTemp = 21,
  TankTemp  =  22,
  Compressor = 23,
  LastErrorCode = 24,
  Mode = 27,
  HeatLSB = 29,
  HeatMSB = 30,
  CoolLSB = 31,
  CoolMSB = 32,
  TankLSB = 33,
  TankMSB = 34,
  PumpStage = 35,
  Force = 128, //-> 0x40
  ResetError = 129,
  HeatOutsideTempLow = 130,
  HeatOutsideTempHigh = 131,
  HeatWaterTempLow = 132,
  HeatWaterTempHigh = 133,
  HeatOffOutsideTemp = 134,
  HeaterOnOutsideTemp = 135,
  CoolSetpointTemp = 136,
  TankSetpointTemp = 137,
  ShiftSetpoint = 138,
  ServiceMode = 142, // 0x02 - pump speed
  ReqMode = 144,
  PumpSpeed = 147,
  // Installer-menu registers. The heat pump never broadcasts them: it only
  // echoes one back after it has been written. Verified on a WH-MDC05F3E5:
  // writing 195 or 198 is acknowledged by a 0x55 frame carrying the value,
  // and clearing bit 0x04 of 198 does disable the tank booster heater.
  Settings = 192,      // -> 0x1 room thermo, 0x80 antifreeze, 0x04 solar prio, 0x40 cool prio, 0x08 heat prio, 0x10 steril
  HeatInterval = 193,  // -> 1 == 30min, 20 == 10h
  TankInterval = 194,  // in min
  BoosterDelay = 195,  // in min
  SterilTemp = 196,
  SterilTime = 197,    // in min
  SettingsCont = 198,  // -> 0x04 booster, 0x08 bpan
  AutoToCoolTemp = 199,
  AutoToHeatTemp = 200,
  // UnknownCommand 255; -- out of range, see the bound in heatpump_update
  Num
};

std::array<std::function<void(uint8_t value)>, (size_t)Registers::Num> registers;

class AquareaComponent : public Component, public uart::UARTDevice
{
public:
    AquareaComponent() = delete;
    AquareaComponent(InternalGPIOPin *rts_pin_)
    {
      rx_packet.reserve(4);
      rts_pin = rts_pin_;
    }

protected:
    SUB_BINARY_SENSOR(defrost);
    SUB_BINARY_SENSOR(booster);
    SUB_SENSOR(outside_temperature);
    SUB_SENSOR(outlet_temperature);
    SUB_SENSOR(inlet_temperature);
    SUB_SENSOR(tank_temperature);
    SUB_SENSOR(compressor_value);
    SUB_SENSOR(heat_power);
    SUB_SENSOR(cool_power);
    SUB_SENSOR(tank_power);
    SUB_SENSOR(pump_speed);
    SUB_TEXT_SENSOR(error_code);
    SUB_TEXT_SENSOR(last_error_code);
    SUB_TEXT_SENSOR(current_mode);
    SUB_BUTTON(reset_error);
    SUB_SWITCH(force);
    SUB_SWITCH(booster_heater_enable);   // bit 0x04 of 198
    SUB_SWITCH(sterilization_enable);    // bit 0x10 of 192
    SUB_NUMBER(heat_out_temp_low);
    SUB_NUMBER(heat_out_temp_high);
    SUB_NUMBER(heat_water_temp_low);
    SUB_NUMBER(heat_water_temp_high);
    SUB_NUMBER(heat_off_out_temp);
    SUB_NUMBER(heater_on_out_temp);
    SUB_NUMBER(cool_setpoint_temp);
    SUB_NUMBER(tank_setpoint_temp);
    SUB_NUMBER(shift_setpoint_temp);
    SUB_NUMBER(pump_speed);
    SUB_NUMBER(settings_raw);    // raw byte 192
    SUB_NUMBER(heat_interval);   // 193
    SUB_NUMBER(tank_interval);   // 194
    SUB_NUMBER(booster_delay);   // 195
    SUB_NUMBER(steril_temp);     // 196
    SUB_NUMBER(steril_time);     // 197
    SUB_NUMBER(settings_cont);   // raw byte 198
    SUB_SELECT(request_mode);

  std::queue<std::pair<uint8_t /*register*/, uint8_t /* value */>> write_queue;

  uint8_t settings_octet = 0, settings_cont_octet = 0;
  bool settings_vu = false, settings_cont_vu = false;

  void ecrire_bit(const Registers& reg, uint8_t masque, bool pose)
  {
    uint8_t* octet = (reg == Registers::Settings) ? &settings_octet : &settings_cont_octet;
    const bool vu = (reg == Registers::Settings) ? settings_vu : settings_cont_vu;
    if(!vu)
    {
      ESP_LOGW("aquarea", "register %d never seen, write the raw byte once first", (int)reg);
      return;
    }
    write_queue.push(std::make_pair(reg, pose ? (*octet | masque) : (*octet & (uint8_t)~masque)));
  }

private:
  struct Power
  {
    void write_lb(long timestamp, uint8_t value)
    {
      valid = false;
      lb_timestamp = timestamp;
      power=value;
    }

    void write_mb(long timestamp, uint8_t value)
    {
      power |= ((uint16_t)value)<<8;
      if((timestamp - lb_timestamp) < 200) valid = true;
    }

    bool is_valid() const
    {
      return valid;
    }

    uint16_t get_power() const
    {
      return power;
    }

  private:
    long lb_timestamp = 0;
    uint16_t power=0u;
    bool valid=false;
  };

  std::string get_current_mode(uint8_t value)
  {
    std::string mode(value & 1 ? "on" : "off");
    if (value & 2) mode += " heat";
    if (value & 4) mode += " cool";
    if (value & 16) mode += " tank";
    if (value & 32) mode += " auto";
    if (value & 64) mode += " quiet";
    if (value & 128) mode += " heater";

    return mode;
  }

  void register_select_if_present(const Registers& reg, select::Select* select, std::function<std::string(uint8_t value)> modifier = {})
  {
    if(select)
      registers[reg] = std::move([select = select, modifier = std::move(modifier)](uint8_t value)
        { 
          select->publish_state(modifier ? modifier(value) : std::to_string((uint32_t)value));
        });
  }

  void register_text_sensor_if_present(const Registers& reg, text_sensor::TextSensor* text_sensor, std::function<std::string(uint8_t value)> modifier = {})
  {
    if(text_sensor)
      registers[reg] = std::move([text_sensor = text_sensor, modifier = std::move(modifier)](uint8_t value)
        { 
          text_sensor->publish_state(modifier ? modifier(value) : std::to_string((uint32_t)value));
        });
  }

  void register_binary_sensor_if_present(const Registers& reg, const std::vector<std::pair<binary_sensor::BinarySensor* /*sensor*/, uint8_t /*mask*/>>& sensors)
  {
    if(std::any_of(sensors.begin(), sensors.end(), [](auto& sensor){return sensor.first != nullptr;}))
      registers[reg] = std::move([&](uint8_t value)
        { 
          for(auto& sensor : sensors)
          {
            if(sensor.first)
              sensor.first->publish_state(value & sensor.second);
          }
        });
  }

  void register_sensor_if_present(const Registers& reg, sensor::Sensor* sensor, std::function<uint8_t(uint8_t value)>&& modifier = {})
  {
    if(sensor)
      registers[reg] = std::move([sensor = sensor, modifier = std::move(modifier)](uint8_t value)
        { 
          if(modifier)
            value = modifier(value);
          sensor->publish_state(*((int8_t*)&value));
        });
  }

  void register_power_sensor_if_present(const Registers& reg_lsb, const Registers& reg_msb, sensor::Sensor* sensor, Power& power)
  {
    if(sensor)
    {
      registers[reg_lsb] = std::move([&power](uint8_t value)
        { 
          power.write_lb(millis(), value);
        });
      registers[reg_msb] = std::move([&power, sensor = sensor](uint8_t value)
        { 
          power.write_mb(millis(), value);
          if(power.is_valid())
            sensor->publish_state(power.get_power());
        });
    }
  }

  void register_switch_if_present(const Registers& reg, switch_::Switch* sw, std::function<uint8_t(uint8_t value)> modifier = {})
  {
    if(sw)
      registers[reg] = std::move([sw = sw, modifier = std::move(modifier)](uint8_t value)
        { 
          sw->publish_state(modifier ? modifier(value) : value);
        });
  }

  void register_number_if_present(const Registers& reg, number::Number* number, std::function<uint8_t(uint8_t value)> modifier = {})
  {
    if(number)
      registers[reg] = std::move([number = number, modifier = std::move(modifier)](uint8_t value)
        { 
          if(modifier) 
            value = modifier(value);
          number->publish_state(*((int8_t*)&value));
        });
  }

  // 192 and 198 are bit fields and a write carries the whole byte, so a bit
  // can only be set once the byte is known. The heat pump never broadcasts
  // them, it only echoes a write, so until something has been written the
  // switches refuse to act rather than guess the other bits.
  // Like register_number_if_present, without the signed cast: a bit field
  // would read negative as soon as bit 0x80 is set.
  void register_raw_number_if_present(const Registers& reg, number::Number* number)
  {
    if(number)
      registers[reg] = std::move([number = number](uint8_t value)
        {
          number->publish_state(value);
        });
  }

  void setup_registers_cbk()
  {
    static const std::vector<std::pair<binary_sensor::BinarySensor*, uint8_t>> defrost_reg_sensors({{defrost_binary_sensor_, 64}, {booster_binary_sensor_, 2}});
    register_binary_sensor_if_present(Registers::Defrost, defrost_reg_sensors);
    register_sensor_if_present(Registers::OutsideTemp, outside_temperature_sensor_);
    register_sensor_if_present(Registers::OutletTemp, outlet_temperature_sensor_);
    register_sensor_if_present(Registers::InletTemp, inlet_temperature_sensor_);
    register_sensor_if_present(Registers::TankTemp, tank_temperature_sensor_);
    register_sensor_if_present(Registers::Compressor, compressor_value_sensor_);
    register_power_sensor_if_present(Registers::HeatLSB, Registers::HeatMSB, heat_power_sensor_, heat_power);
    register_power_sensor_if_present(Registers::CoolLSB, Registers::CoolMSB, cool_power_sensor_, cool_power);
    register_power_sensor_if_present(Registers::TankLSB, Registers::TankMSB, tank_power_sensor_, tank_power);
    register_sensor_if_present(Registers::PumpStage, pump_speed_sensor_, [](uint8_t value){ return value/16; });
    register_text_sensor_if_present(Registers::ErrorCode, error_code_text_sensor_);
    register_text_sensor_if_present(Registers::LastErrorCode, last_error_code_text_sensor_);
    register_text_sensor_if_present(Registers::Mode, current_mode_text_sensor_, std::bind(&AquareaComponent::get_current_mode, this, std::placeholders::_1));
    register_switch_if_present(Registers::Force, force_switch_, [](uint8_t value){ return value & 0x40; });
    register_number_if_present(Registers::HeatOutsideTempLow, heat_out_temp_low_number_);
    register_number_if_present(Registers::HeatOutsideTempHigh, heat_out_temp_high_number_);
    register_number_if_present(Registers::HeatWaterTempLow, heat_water_temp_low_number_);
    register_number_if_present(Registers::HeatWaterTempHigh, heat_water_temp_high_number_);
    register_number_if_present(Registers::HeatOffOutsideTemp, heat_off_out_temp_number_);
    register_number_if_present(Registers::HeaterOnOutsideTemp, heater_on_out_temp_number_);
    register_number_if_present(Registers::CoolSetpointTemp, cool_setpoint_temp_number_);
    register_number_if_present(Registers::TankSetpointTemp, tank_setpoint_temp_number_);
    register_number_if_present(Registers::ShiftSetpoint, shift_setpoint_temp_number_);
    register_number_if_present(Registers::PumpSpeed, pump_speed_number_, [](uint8_t value){ return value*16; });
    // Installer menu
    register_raw_number_if_present(Registers::HeatInterval, heat_interval_number_);
    register_raw_number_if_present(Registers::TankInterval, tank_interval_number_);
    register_raw_number_if_present(Registers::BoosterDelay, booster_delay_number_);
    register_raw_number_if_present(Registers::SterilTemp, steril_temp_number_);
    register_raw_number_if_present(Registers::SterilTime, steril_time_number_);
    registers[Registers::Settings] = std::move([this](uint8_t value)
      {
        settings_octet = value; settings_vu = true;
        if(settings_raw_number_) settings_raw_number_->publish_state(value);
        if(sterilization_enable_switch_) sterilization_enable_switch_->publish_state(value & 0x10);
      });
    registers[Registers::SettingsCont] = std::move([this](uint8_t value)
      {
        settings_cont_octet = value; settings_cont_vu = true;
        if(settings_cont_number_) settings_cont_number_->publish_state(value);
        if(booster_heater_enable_switch_) booster_heater_enable_switch_->publish_state(value & 0x04);
      });
    register_select_if_present(Registers::ReqMode, request_mode_select_, std::bind(&AquareaComponent::get_current_mode, this, std::placeholders::_1));
  }

  float get_setup_priority() const override
  {
    return setup_priority::BUS;
  }

  void setup() override
  {
    ESP_LOGD("aquarea", "%s", __func__);
    setup_registers_cbk();
    rts_pin->pin_mode(gpio::FLAG_OUTPUT);
    rts_pin->digital_write(false);
  }

  void loop() override
  {
    bool wait = false;
    long ts = millis();
    do
    {
      if(heatpump_read())
      {
        if(wait)
        {
          rts_pin->digital_write(false);
          ESP_LOGD("aquarea", "rts low, done after %lu", millis() - ts);
          wait = false;
        }

        if((rx_packet[0] == 0x55))
        {
          ESP_LOGV("aquarea", "data %x %x %x %x", rx_packet[0], rx_packet[1], rx_packet[2], rx_packet[3]);
          heatpump_update(rx_packet[1], rx_packet[2]);
          const bool gotMagick = rx_packet[1] == Registers::Defrost;
          if(gotMagick && !write_queue.empty())
          {
            wait = true;
            ESP_LOGD("aquarea", "rts high");
            rts_pin->digital_write(true);
            delay(1);
            heatpump_write();
          }
        }
        else
        {
          // Remote -> heat pump frames, otherwise read and dropped silently.
          ESP_LOGV("aquarea", "cmd %x %x %x %x", rx_packet[0], rx_packet[1], rx_packet[2], rx_packet[3]);
        }
        rx_packet.clear();
      }

      if((millis() - ts) > 300u)
      {
          rts_pin->digital_write(false);
          ESP_LOGD("aquarea", "rts low, timeout");
          break;
      }
    }while(wait);
  }

  uint8_t heatpump_checksum(const std::vector<uint8_t>& packet)
  {
    return (packet[0] + packet[1] + packet[2]) & 0xff;
  }

  bool heatpump_read(bool log = false)
  {
    if(this->available())
    {
      rx_packet.push_back(this->read());
      ESP_LOGVV("aquarea", "data %x", rx_packet[0]);
      if((rx_packet[0] == 0xAA || rx_packet[0] == 0x55))
      {
        if(rx_packet.size() == 4)
        {
          uint8_t rx_chsum = heatpump_checksum(rx_packet);
          ESP_LOGVV("aquarea", "data %x %x %x %x", rx_packet[0], rx_packet[1], rx_packet[2], rx_packet[3]);
          if(rx_chsum == rx_packet[3])
          {
              return true;
          }
          else
          {
            ESP_LOGW("aquarea", "Checksum mismatch %x %x", rx_chsum, rx_packet[3]);
            rx_packet.clear();
          }
        }
      }
      else
      {
        rx_packet.clear();
      }
    }
    return false;
  }

  void heatpump_write()
  {
    std::vector<uint8_t> packet({0xAA, write_queue.front().first, write_queue.front().second});
    packet.push_back(heatpump_checksum(packet));

    ESP_LOGD("aquarea", "send: %x %x", packet[1], packet[2]);
    this->write_array(packet);
    this->flush();
    write_queue.pop();
  }

  void heatpump_update(uint8_t reg, uint8_t value)
  {
    // reg comes from the frame and can be up to 255 while the table only has
    // Registers::Num entries -- without this bound, an unmapped register the
    // pump emits (255 is sent at every start-up) reads past the end.
    if(reg < Registers::Num && registers[reg])
      registers[reg](value);
  }

  Power heat_power;
  Power tank_power;
  Power cool_power;
  GPIOPin* rts_pin;
  std::vector<uint8_t> rx_packet;
};

class ResetErrorButton : public button::Button, public Parented<AquareaComponent>
{
 protected:
  void press_action() override
  {
    ESP_LOGD("aquarea", "ResetErrorButton");
    parent_->write_queue.push(std::make_pair(Registers::ResetError, 1));
  }
};

class ForceSwitch : public switch_::Switch, public Parented<AquareaComponent>
{
protected:
  void write_state(bool state) override
  {
    ESP_LOGD("aquarea", "ForceSwitch");
    parent_->write_queue.push(std::make_pair(Registers::Force, state ? 0x40 : 0x00));
  }
};

class HeatOutsideTempLowNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "HeatOutsideTempLowNumber");
    parent_->write_queue.push(std::make_pair(Registers::HeatOutsideTempLow, value));
  }
};

class HeatOutsideTempHighNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "HeatOutsideTempHighNumber");
    parent_->write_queue.push(std::make_pair(Registers::HeatOutsideTempHigh, value));
  }
};

class HeatWaterTempLowNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "HeatWaterTempLowNumber");
    parent_->write_queue.push(std::make_pair(Registers::HeatWaterTempLow, value));
  }
};

class HeatWaterTempHighNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "HeatWaterTempHighNumber");
    parent_->write_queue.push(std::make_pair(Registers::HeatWaterTempHigh, value));
  }
};

class HeatOffOutsideTempNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "HeatOffOutsideTempNumber");
    parent_->write_queue.push(std::make_pair(Registers::HeatOffOutsideTemp, value));
  }
};

class HeaterOnOutsideTempNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "HeaterOnOutsideTempNumber");
    parent_->write_queue.push(std::make_pair(Registers::HeaterOnOutsideTemp, value));
  }
};

class CoolSetpointTempNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "CoolSetpointTempNumber");
    parent_->write_queue.push(std::make_pair(Registers::CoolSetpointTemp, value));
  }
};

class TankSetpointTempNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "TankSetpointTempNumber");
    parent_->write_queue.push(std::make_pair(Registers::TankSetpointTemp, value));
  }
};

class ShiftSetpointTempNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "ShiftSetpointTempNumber");
    parent_->write_queue.push(std::make_pair(Registers::ShiftSetpoint, value));
  }
};

// Installer-menu switches. They set one bit of an already known byte; if the
// byte has never been seen, ecrire_bit refuses and logs a warning.
class BoosterHeaterEnableSwitch : public switch_::Switch, public Parented<AquareaComponent>
{
protected:
  void write_state(bool state) override
  {
    ESP_LOGD("aquarea", "BoosterHeaterEnableSwitch %d", state);
    parent_->ecrire_bit(Registers::SettingsCont, 0x04, state);
  }
};

class SterilizationEnableSwitch : public switch_::Switch, public Parented<AquareaComponent>
{
protected:
  void write_state(bool state) override
  {
    ESP_LOGD("aquarea", "SterilizationEnableSwitch %d", state);
    parent_->ecrire_bit(Registers::Settings, 0x10, state);
  }
};

// Installer-menu numbers, raw bytes. Give them no initial_value: they must
// never write at boot, since the current value cannot be read back.
class BoosterDelayNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "BoosterDelayNumber");
    parent_->write_queue.push(std::make_pair(Registers::BoosterDelay, value));
  }
};

class SettingsContNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "SettingsContNumber");
    parent_->write_queue.push(std::make_pair(Registers::SettingsCont, value));
  }
};

class SettingsRawNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "SettingsRawNumber");
    parent_->write_queue.push(std::make_pair(Registers::Settings, value));
  }
};

class HeatIntervalNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "HeatIntervalNumber");
    parent_->write_queue.push(std::make_pair(Registers::HeatInterval, value));
  }
};

class TankIntervalNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "TankIntervalNumber");
    parent_->write_queue.push(std::make_pair(Registers::TankInterval, value));
  }
};

class SterilTempNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "SterilTempNumber");
    parent_->write_queue.push(std::make_pair(Registers::SterilTemp, value));
  }
};

class SterilTimeNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "SterilTimeNumber");
    parent_->write_queue.push(std::make_pair(Registers::SterilTime, value));
  }
};

class PumpSpeedNumber : public number::Number, public Parented<AquareaComponent>
{
protected:
  void control(float value) override
  {
    ESP_LOGD("aquarea", "PumpSpeedNumber");
    parent_->write_queue.push(std::make_pair(Registers::PumpSpeed, value));
  }
};

class ReqModeSelect : public select::Select, public Parented<AquareaComponent>
{
protected:
  void control(const std::string& value) override
  {
    ESP_LOGD("aquarea", "ReqModeSelect: %s", value.c_str());
    uint8_t mode = 0u;
    if(std::string::npos != value.find("on")) mode += 1;
    if(std::string::npos != value.find("heat")) mode += 2;
    if(std::string::npos != value.find("cool")) mode += 4;
    if(std::string::npos != value.find("tank")) mode += 16;
    if(std::string::npos != value.find("auto")) mode += 32;
    if(std::string::npos != value.find("quiet"))  mode += 64;
    if(std::string::npos != value.find("heater"))  mode += 128;

    parent_->write_queue.push(std::make_pair(Registers::ReqMode, mode));
  }
};

} // namespace aquarea
} // namespace esphome
