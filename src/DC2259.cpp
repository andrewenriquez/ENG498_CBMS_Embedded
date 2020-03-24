#include <LTC6811.h>

/*! Analog Devices DC2259A Demonstration Board. 
* LTC6811: Multicell Battery Monitors
*
*@verbatim
*NOTES
* Setup:
*   Set the terminal baud rate to 115200 and select the newline terminator.
*   Ensure all jumpers on the demo board are installed in their default positions from the factory.
*   Refer to Demo Manual.
*
*USER INPUT DATA FORMAT:
* decimal : 1024
* hex     : 0x400
* octal   : 02000  (leading 0)
* binary  : B10000000000
* float   : 1024.0
*@endverbatim
*
* https://www.analog.com/en/products/ltc6811-1.html
* https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/dc2259a.html
*
********************************************************************************
* Copyright 2019(c) Analog Devices, Inc.
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*  - Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  - Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided with the
*    distribution.
*  - Neither the name of Analog Devices, Inc. nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*  - The use of this software may or may not infringe the patent rights
*    of one or more patent holders.  This license does not release you
*    from the requirement that you obtain separate licenses from these
*    patent holders to use this software.
*  - Use of the software either in source or binary form, must be run
*    on or directly connected to an Analog Devices Inc. component.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/*! @file
    @ingroup LTC6811-1
*/

/************************************* Read me *******************************************
In this sketch book:
  -All Global Variables are in Upper casing
  -All Local Variables are in lower casing
  -The Function wakeup_sleep(TOTAL_IC) : is used to wake the LTC681x from sleep state.
   It is defined in LTC681x.cpp
  -The Function wakeup_idle(TOTAL_IC) : is used to wake the ICs connected in daisy chain 
   via the LTC6820 by initiating a dummy SPI communication. It is defined in LTC681x.cpp  
*******************************************************************************************/

/************************* Includes ***************************/
#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include "Linduino.h"
#include "LT_SPI.h"
#include "LT_I2C.h"          
#include "QuikEval_EEPROM.h"
#include "UserInterface.h"   
#include "LTC681x.h"
#include "LTC6811.h"
#include "LTC2944.h"
#include <Wire.h>

#include "RTClib.h"

/************************* Defines *****************************/
#define ENABLED 1
#define DISABLED 0
#define DATALOG_ENABLED 1
#define DATALOG_DISABLED 0
#define SPI_CLOCK_DIV16 0x01



/**************** Local Function Declaration *******************/
void measurement_loop(uint8_t datalog_en);
void measurement_loop2(uint8_t datalog_en, int8_t mAh_or_Coulombs , int8_t celcius_or_kelvin ,uint16_t prescalar_mode,uint16_t prescalarValue, uint16_t alcc_mode);
void print_menu(void);
void print_wrconfig(void);
void print_rxconfig(void);
void print_cells(uint8_t datalog_en);
void BLE_cells(uint8_t datalog_en); //added by AE
void print_aux(uint8_t datalog_en);
void print_stat(void);
void print_sumofcells(void);
void check_mux_fail(void);
void print_selftest_errors(uint8_t adc_reg ,int8_t error);
void print_overlap_results(int8_t error);
void print_digital_redundancy_errors(uint8_t adc_reg ,int8_t error);
void print_open_wires(void);
void print_pec_error_count(void);
int8_t select_s_pin(void);
void print_wrpwm(void);
void print_rxpwm(void);
void print_wrsctrl(void);
void print_rxsctrl(void); 
void print_wrcomm(void);
void print_rxcomm(void);
void print_conv_time(uint32_t conv_time);
void check_error(int error);
void serial_print_text(char data[]);
void serial_print_hex(uint8_t data);
char read_hex(void);   
char get_char(void);
void run_command(uint32_t cmd);

// Function Declaration
void print_title();                 // Print the title block
void print_prompt();                // Print the Prompt
void store_alert_settings();        // Store the alert settings to the EEPROM
int8_t restore_alert_settings();    // Read the alert settings from EEPROM
int8_t menu_1_automatic_mode(int8_t mAh_or_Coulombs, int8_t celcius_or_kelvin ,uint16_t prescalar_mode, uint16_t prescalarValue, uint16_t alcc_mode);
int8_t menu_2_scan_mode(int8_t mAh_or_Coulombs , int8_t celcius_or_kelvin ,uint16_t prescalar_mode,uint16_t prescalarValue, uint16_t alcc_mode);
int8_t menu_3_manual_mode(int8_t mAh_or_Coulombs ,int8_t celcius_or_kelvin ,uint16_t prescalar_mode, uint16_t prescalarValue, uint16_t alcc_mode);
int8_t menu_4_sleep_mode(int8_t mAh_or_Coulombs ,uint16_t prescalar_mode, uint16_t prescalarValue, uint16_t alcc_mode);
int8_t menu_5_shutdown_mode();

int8_t menu_6_settings(uint8_t *mAh_or_Coulombs, uint8_t *celcius_or_kelvin, uint16_t *prescalar_mode, uint16_t *prescalarValue, uint16_t *alcc_mode);
void checkAlerts(uint8_t status_code);

int8_t menu_6_settings_menu_1_set_alert_thresholds();
int8_t menu_6_settings_menu_2_set_prescalar_values(uint16_t *prescalar_mode, uint16_t *prescalarValue);
uint8_t menu_6_alert_menu_3_set_allcc_state(uint16_t *alcc_mode);
uint8_t menu_6_alert_menu_4_set_units(uint8_t *mAh_or_Coulombs, uint8_t *celcius_or_kelvin);
int8_t menu_6_alert_menu_1_set_charge_thresholds();
int8_t menu_6_alert_menu_2_set_voltage_thresholds();
int8_t menu_6_alert_menu_3_set_current_thresholds();
int8_t menu_6_alert_menu_4_set_temperature_thresholds();

#define AUTOMATIC_MODE_DISPLAY_DELAY 1000                  //!< The delay between readings in automatic mode
#define SCAN_MODE_DISPLAY_DELAY 5000                      //!< The delay between readings in scan mode
const float resistor = .100;                               //!< resistor value on demo board

// Error string
const char ack_error[] = "Error: No Acknowledge. Check I2C Address."; //!< Error message



// Global variables
static int8_t demo_board_connected;        //!< Set to 1 if the board is connected
static uint8_t alert_code = 0;             //!< Value stored or read from ALERT register.  Shared between loop() and restore_alert_settings()


/*******************************************************************
  Setup Variables
  The following variables can be modified to configure the software.
********************************************************************/
const uint8_t TOTAL_IC = 1;//!< Number of ICs in the daisy chain



//ADC Command Configurations. See LTC681x.h for options.
const uint8_t ADC_OPT = ADC_OPT_DISABLED; //!< ADC Mode option bit
const uint8_t ADC_CONVERSION_MODE = MD_7KHZ_3KHZ; //!< ADC Mode
const uint8_t ADC_DCP = DCP_ENABLED; //!< Discharge Permitted 
const uint8_t CELL_CH_TO_CONVERT = CELL_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t AUX_CH_TO_CONVERT = AUX_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t STAT_CH_TO_CONVERT = STAT_CH_ALL; //!< Channel Selection for ADC conversion
const uint8_t SEL_ALL_REG = REG_ALL; //!< Register Selection 
const uint8_t SEL_REG_A = REG_1; //!< Register Selection 
const uint8_t SEL_REG_B = REG_2; //!< Register Selection 

const uint16_t MEASUREMENT_LOOP_TIME = 900; //!< Loop Time in milliseconds(ms)

//Under Voltage and Over Voltage Thresholds
const uint16_t OV_THRESHOLD = 44000; //!< Over voltage threshold ADC Code. LSB = 0.0001 ---(4.4V)
const uint16_t UV_THRESHOLD = 25000; //!< Under voltage threshold ADC Code. LSB = 0.0001 ---(2.5V)

//Loop Measurement Setup. These Variables are ENABLED or DISABLED. Remember ALL CAPS
const uint8_t WRITE_CONFIG = DISABLED;  //!< This is to ENABLED or DISABLED writing into to configuration registers in a continuous loop
const uint8_t READ_CONFIG = DISABLED; //!< This is to ENABLED or DISABLED reading the configuration registers in a continuous loop
const uint8_t MEASURE_CELL = ENABLED; //!< This is to ENABLED or DISABLED measuring the cell voltages in a continuous loop
const uint8_t MEASURE_AUX = DISABLED; //!< This is to ENABLED or DISABLED reading the auxiliary registers in a continuous loop
const uint8_t MEASURE_STAT = DISABLED; //!< This is to ENABLED or DISABLED reading the status registers in a continuous loop
const uint8_t PRINT_PEC = DISABLED; //!< This is to ENABLED or DISABLED printing the PEC Error Count in a continuous loop

RTC_DS3231 rtc; //Real time clock object



/************************************
  END SETUP
*************************************/

/******************************************************
 Global Battery Variables received from 681x commands.
 These variables store the results from the LTC6811
 register reads and the array lengths must be based
 on the number of ICs on the stack
 ******************************************************/
cell_asic BMS_IC[TOTAL_IC]; //!< Global Battery Variable

/*********************************************************
 Set the configuration bits. 
 Refer to the Configuration Register Group from data sheet. 
**********************************************************/
bool REFON = true; //!< Reference Powered Up Bit
bool ADCOPT = false; //!< ADC Mode option bit
bool GPIOBITS_A[5] = {false,false,true,true,true}; //!< GPIO Pin Control // Gpio 1,2,3,4,5
uint16_t UV=UV_THRESHOLD; //!< Under-voltage Comparison Voltage
uint16_t OV=OV_THRESHOLD; //!< Over-voltage Comparison Voltage
bool DCCBITS_A[12] = {false,false,false,false,false,false,false,false,false,false,false,false}; //!< Discharge cell switch //Dcc 1,2,3,4,5,6,7,8,9,10,11,12
bool DCTOBITS[4] = {true, false, true, false}; //!< Discharge time value // Dcto 0,1,2,3 // Programed for 4 min 
/*Ensure that Dcto bits are set according to the required discharge time. Refer to the data sheet */

/*!**********************************************************************
 \brief  Initializes hardware and variables
 @return void
 ***********************************************************************/
void setup()
{

  char demo_name[] = "DC1812";      //! Demo Board Name stored in QuikEval EEPROM

  //quikeval_I2C_init();              //! Configure the EEPROM I2C port for 100kHz
  //quikeval_I2C_connect();          //! Connects to main I2C port
  Wire.begin();
  Serial.begin(115200);             //! Initialize the serial port to the PC
  //print_title();
  //demo_board_connected = discover_demo_board(demo_name);
  demo_board_connected = true;
  if (demo_board_connected)
  {
    
    //print_prompt();
  }
  else
  {
    demo_board_connected = true;
    Serial.println("Did not read ID String, attempting to proceed anyway...\nPlease ensure I2C lines of Linduino are connected to the LTC device");
    //print_prompt();
  }


  rtc.begin();
  Serial.begin(115200);
  Serial1.begin(9600); //Default Comm for BLE.
  //quikeval_SPI_connect();
  spi_enable(SPI_CLOCK_DIV16); // This will set the Linduino to have a 1MHz Clock
  LTC6811_init_cfg(TOTAL_IC, BMS_IC);
  for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
  {
    LTC6811_set_cfgr(current_ic,BMS_IC,REFON,ADCOPT,GPIOBITS_A,DCCBITS_A, DCTOBITS, UV, OV);
  }
  LTC6811_reset_crc_count(TOTAL_IC,BMS_IC);
  LTC6811_init_reg_limits(TOTAL_IC,BMS_IC);
  print_menu();
}

/*!*********************************************************************
 \brief Main loop
 @return void
***********************************************************************/
void loop()
{

  if (Serial.available())           // Check for user input
  {
    uint32_t user_command;
    user_command = read_int();      // Read the user command
    if(user_command=='m')
    { 
      print_menu();
    }
    else
    { 
      Serial.println(user_command);
      run_command(user_command); 
    }   
  }
}

/*!*****************************************
 \brief Executes the user command
 @return void
*******************************************/
void run_command(uint32_t cmd)
{
  uint8_t streg=0;
  int8_t error = 0;
  uint32_t conv_time = 0;
  int8_t s_pin_read=0;
  
    int8_t ack = 0;                               //! I2C acknowledge indicator
                 //! The user input command
  static uint8_t mAh_or_Coulombs = 0;
  static uint8_t celcius_or_kelvin = 0;
  static uint16_t prescalar_mode = LTC2944_PRESCALAR_M_4096;
  static uint16_t prescalarValue = 4096;
  static uint16_t alcc_mode = LTC2944_ALERT_MODE;
  switch (cmd)
  {
    case 1: // Write and Read Configuration Register
      wakeup_sleep(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,BMS_IC); // Write into Configuration Register
      print_wrconfig();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,BMS_IC); // Read Configuration Register
      check_error(error);
      print_rxconfig();
      break;

    case 2: // Read Configuration Register
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,BMS_IC);
      check_error(error);
      print_rxconfig();
      break;

    case 3: // Start Cell ADC Measurement
      wakeup_sleep(TOTAL_IC);
      LTC6811_adcv(ADC_CONVERSION_MODE,ADC_DCP,CELL_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      print_conv_time(conv_time);
      break;

    case 4: // Read Cell Voltage Registers
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdcv(SEL_ALL_REG, TOTAL_IC,BMS_IC); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);
      break;

    case 5: // Start GPIO ADC Measurement
      wakeup_sleep(TOTAL_IC);
      LTC6811_adax(ADC_CONVERSION_MODE, AUX_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      print_conv_time(conv_time); 
      break;

    case 6: // Read AUX Voltage Registers
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdaux(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Set to read back all aux registers
      check_error(error);
      print_aux(DATALOG_DISABLED);
      break;

    case 7: // Start Status ADC Measurement
      wakeup_sleep(TOTAL_IC);
      LTC6811_adstat(ADC_CONVERSION_MODE, STAT_CH_TO_CONVERT);
      conv_time=LTC6811_pollAdc();
      print_conv_time(conv_time);
      break;

    case 8: // Read Status registers
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdstat(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Set to read back all stat registers
      check_error(error);
      print_stat();
      break;

    case 9:// Start Combined Cell Voltage and GPIO1, GPIO2 Conversion and Poll Status
      wakeup_sleep(TOTAL_IC);
      LTC6811_adcvax(ADC_CONVERSION_MODE,ADC_DCP);
      conv_time = LTC6811_pollAdc();
      print_conv_time(conv_time);
      wakeup_idle(TOTAL_IC);
      error =LTC6811_rdcv(SEL_ALL_REG, TOTAL_IC,BMS_IC); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);     
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdaux(SEL_REG_A,TOTAL_IC,BMS_IC); // Set to read back aux registers A
      check_error(error);
      print_aux(DATALOG_DISABLED);
      break;
      
    case 10: //Start Combined Cell Voltage and Sum of cells
      wakeup_sleep(TOTAL_IC);
      LTC6811_adcvsc(ADC_CONVERSION_MODE,ADC_DCP);
      conv_time = LTC6811_pollAdc();
      print_conv_time(conv_time);
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcv(SEL_ALL_REG, TOTAL_IC,BMS_IC); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdstat(SEL_REG_A,TOTAL_IC,BMS_IC); // Set to read stat registers A
      check_error(error);
      print_sumofcells();
      break;
      
    case 11: // Loop Measurements of configuration register or cell voltages or auxiliary register or status register without data-log output
      wakeup_sleep(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,BMS_IC);
      measurement_loop(DATALOG_DISABLED);
      print_menu();
      break;

    case 12: //Data-log print option Loop Measurements of configuration register or cell voltages or auxiliary register or status register
      wakeup_sleep(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,BMS_IC);
      measurement_loop(DATALOG_ENABLED);
      print_menu();
      break;

    case 13: // Clear all ADC measurement registers
      wakeup_sleep(TOTAL_IC);
      LTC6811_clrcell();
      LTC6811_clraux();
      LTC6811_clrstat();
      wakeup_idle(TOTAL_IC);    
      LTC6811_rdcv(SEL_ALL_REG, TOTAL_IC,BMS_IC); // Read back all cell voltage registers
      print_cells(DATALOG_DISABLED);

      LTC6811_rdaux(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Read back all aux registers
      print_aux(DATALOG_DISABLED);

      LTC6811_rdstat(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Read back all stat 
      print_stat();           
      break;
        
    case 14: //Read CV,AUX and ADSTAT Voltages 
      wakeup_sleep(TOTAL_IC);
      LTC6811_adcv(ADC_CONVERSION_MODE,ADC_DCP,CELL_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      print_conv_time(conv_time);
      wakeup_idle(TOTAL_IC);      
      error = LTC6811_rdcv(SEL_ALL_REG, TOTAL_IC,BMS_IC); // Set to read back all cell voltage registers
      check_error(error);
      print_cells(DATALOG_DISABLED);

      wakeup_sleep(TOTAL_IC);
      LTC6811_adax(ADC_CONVERSION_MODE , AUX_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      print_conv_time(conv_time);
      wakeup_idle(TOTAL_IC); 
      error = LTC6811_rdaux(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Set to read back all aux registers
      check_error(error);
      print_aux(DATALOG_DISABLED);   

      wakeup_sleep(TOTAL_IC);
      LTC6811_adstat(ADC_CONVERSION_MODE, STAT_CH_TO_CONVERT);
      conv_time = LTC6811_pollAdc();
      print_conv_time(conv_time);
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdstat(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Set to read back all status registers 
      check_error(error);
      print_stat();    
      break;

    case 15: // Run the Mux Decoder Self Test
      wakeup_sleep(TOTAL_IC);
      LTC6811_diagn();
      conv_time = LTC6811_pollAdc();
      print_conv_time(conv_time); 
      error = LTC6811_rdstat(SEL_REG_B,TOTAL_IC,BMS_IC); // Set to read back status register B
      check_error(error);
      check_mux_fail();
      break;

    case 16:  // Run the ADC/Memory Self Test
      error =0;
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_cell_adc_st(CELL,TOTAL_IC,BMS_IC, ADC_CONVERSION_MODE, ADCOPT);
      print_selftest_errors(CELL, error);
      
      error =0;
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_cell_adc_st(AUX,TOTAL_IC, BMS_IC, ADC_CONVERSION_MODE, ADCOPT);
      print_selftest_errors(AUX, error);

      error =0;
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_cell_adc_st(STAT,TOTAL_IC, BMS_IC, ADC_CONVERSION_MODE, ADCOPT);
      print_selftest_errors(STAT, error);
      print_menu();
      break;
      
    case 17: // Run ADC Overlap self test
      error =0;
      wakeup_sleep(TOTAL_IC);
      error = (int8_t)LTC6811_run_adc_overlap(TOTAL_IC,BMS_IC);
      print_overlap_results(error);
      break;

    case 18: // Run ADC Digital Redundancy self test
      error =0;
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_adc_redundancy_st(ADC_CONVERSION_MODE,AUX,TOTAL_IC, BMS_IC);
      print_digital_redundancy_errors(AUX, error);

      error =0;
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_run_adc_redundancy_st(ADC_CONVERSION_MODE,STAT,TOTAL_IC, BMS_IC);
      print_digital_redundancy_errors(STAT, error);
      break;

    case 19: // Open Wire test for single cell detection
      wakeup_sleep(TOTAL_IC);         
      LTC6811_run_openwire_single(TOTAL_IC, BMS_IC);
      print_open_wires();  
      break;

    case 20: // Open Wire test for multiple cell and two consecutive cells detection
      wakeup_sleep(TOTAL_IC);         
      LTC6811_run_openwire_multi(TOTAL_IC, BMS_IC);  
      break;

    case 21:// PEC Errors Detected
      print_pec_error_count();    
      break;

    case 22: // Reset PEC Counter
      LTC6811_reset_crc_count(TOTAL_IC,BMS_IC);
      print_pec_error_count();
      break;
      
    case 23: // Enable a discharge transistor
      s_pin_read = select_s_pin();
      wakeup_sleep(TOTAL_IC);
      LTC6811_set_discharge(s_pin_read,TOTAL_IC,BMS_IC);
      LTC6811_wrcfg(TOTAL_IC,BMS_IC);   
      print_wrconfig();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,BMS_IC);
      check_error(error);
      print_rxconfig();
      break;
      
    case 24: // Clear all discharge transistors
      wakeup_sleep(TOTAL_IC);
      LTC6811_clear_discharge(TOTAL_IC,BMS_IC);
      LTC6811_wrcfg(TOTAL_IC,BMS_IC);    
      print_wrconfig();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,BMS_IC);
      check_error(error);
      print_rxconfig();
      break;

    case 25:// Write read pwm configuration     
      /*****************************************************
         PWM configuration data.
         1)Set the corresponding DCC bit to one for pwm operation. 
         2)Set the DCTO bits to the required discharge time.
         3)Choose the value to be configured depending on the
          required duty cycle. 
         Refer to the data sheet. 
      *******************************************************/ 
      wakeup_sleep(TOTAL_IC);
      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      {
        BMS_IC[current_ic].pwm.tx_data[0]= 0x88; // Duty cycle for S pin 2 and 1
        BMS_IC[current_ic].pwm.tx_data[1]= 0x88; // Duty cycle for S pin 4 and 3
        BMS_IC[current_ic].pwm.tx_data[2]= 0x88; // Duty cycle for S pin 6 and 5
        BMS_IC[current_ic].pwm.tx_data[3]= 0x88; // Duty cycle for S pin 8 and 7
        BMS_IC[current_ic].pwm.tx_data[4]= 0x88; // Duty cycle for S pin 10 and 9
        BMS_IC[current_ic].pwm.tx_data[5]= 0x88; // Duty cycle for S pin 12 and 11
      }          
      LTC6811_wrpwm(TOTAL_IC,0,BMS_IC);
      print_wrpwm(); 

      wakeup_idle(TOTAL_IC);
      LTC6811_rdpwm(TOTAL_IC,0,BMS_IC);       
      print_rxpwm();                              
      break;

    case 26: // Write and read S Control Register Group
      wakeup_sleep(TOTAL_IC);
      /**************************************************************************************
         S pin control. 
         1)Ensure that the pwm is set according to the requirement using the previous case.
         2)Choose the value depending on the required number of pulses on S pin. 
         Refer to the data sheet. 
      ***************************************************************************************/
      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      {
        BMS_IC[current_ic].sctrl.tx_data[0]=0xFF; // No. of high pulses on S pin 2 and 1
        BMS_IC[current_ic].sctrl.tx_data[1]=0xFF; // No. of high pulses on S pin 4 and 3
        BMS_IC[current_ic].sctrl.tx_data[2]=0xFF; // No. of high pulses on S pin 6 and 5
        BMS_IC[current_ic].sctrl.tx_data[3]=0xFF; // No. of high pulses on S pin 8 and 7
        BMS_IC[current_ic].sctrl.tx_data[4]=0xFF; // No. of high pulses on S pin 10 and 9
        BMS_IC[current_ic].sctrl.tx_data[5]=0xFF; // No. of high pulses on S pin 12 and 11
      }
      LTC6811_wrsctrl(TOTAL_IC,streg,BMS_IC);
      print_wrsctrl();

      // Start S Control pulsing
      wakeup_idle(TOTAL_IC);
      LTC6811_stsctrl();

      // Read S Control Register Group 
      wakeup_idle(TOTAL_IC);
      error=LTC6811_rdsctrl(TOTAL_IC,streg,BMS_IC);
      check_error(error);
      print_rxsctrl();
      break;

    case 27: // Clear S Control Register Group
      wakeup_sleep(TOTAL_IC);
      LTC6811_clrsctrl();
      
      wakeup_idle(TOTAL_IC);
      error=LTC6811_rdsctrl(TOTAL_IC,streg,BMS_IC); // Read S Control Register Group
      check_error(error);
      print_rxsctrl();
      break;
      
    case 28://SPI Communication 
      /*************************************************************
         Ensure to set the GPIO bits to 1 in the CFG register group. 
      *************************************************************/  
      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      {
        //Communication control bits and communication data bytes. Refer to the data sheet.
        BMS_IC[current_ic].com.tx_data[0]= 0x81; // Icom CSBM Low(8) + data D0 (0x11)
        BMS_IC[current_ic].com.tx_data[1]= 0x10; // Fcom CSBM Low(0) 
        BMS_IC[current_ic].com.tx_data[2]= 0xA2; // Icom CSBM Falling Edge (A) +  D1 (0x25)
        BMS_IC[current_ic].com.tx_data[3]= 0x50; // Fcom CSBM Low(0)    
        BMS_IC[current_ic].com.tx_data[4]= 0xA1; // Icom CSBM Falling Edge (A) +  D2 (0x17)
        BMS_IC[current_ic].com.tx_data[5]= 0x79; // Fcom CSBM High(9)
      }
      wakeup_sleep(TOTAL_IC);   
      LTC6811_wrcomm(TOTAL_IC,BMS_IC); // write to comm register                 
      print_wrcomm(); // print data in the comm register

      wakeup_idle(TOTAL_IC);
      LTC6811_stcomm(3); // data length=3 // initiates communication between master and the I2C slave

      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcomm(TOTAL_IC,BMS_IC); // read from comm register                       
      check_error(error);
      print_rxcomm();  // print received data into the comm register
      break;

  case 29: // write byte I2C Communication on the GPIO Ports(using I2C eeprom 24LC025)
       /************************************************************
         Ensure to set the GPIO bits to 1 in the CFG register group. 
      *************************************************************/   
      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      {
        //Communication control bits and communication data bytes. Refer to the data sheet.
        BMS_IC[current_ic].com.tx_data[0]= 0x6A; // Icom Start(6) + I2C_address D0 (0xA0)
        BMS_IC[current_ic].com.tx_data[1]= 0x08; // Fcom master NACK(8)  
        BMS_IC[current_ic].com.tx_data[2]= 0x00; // Icom Blank (0) + eeprom address D1 (0x00)
        BMS_IC[current_ic].com.tx_data[3]= 0x08; // Fcom master NACK(8)   
        BMS_IC[current_ic].com.tx_data[4]= 0x01; // Icom Blank (0) + data D2 (0x11)
        BMS_IC[current_ic].com.tx_data[5]= 0x19; // Fcom master NACK + Stop(9) 
      }
      wakeup_sleep(TOTAL_IC);       
      LTC6811_wrcomm(TOTAL_IC,BMS_IC); // write to comm register    
      print_wrcomm(); // print transmitted data from the comm register

      wakeup_idle(TOTAL_IC);
      LTC6811_stcomm(3); // data length=3 // initiates communication between master and the I2C slave

      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcomm(TOTAL_IC,BMS_IC); // read from comm register                       
      check_error(error);
      print_rxcomm(); // print received data into the comm register  
      break; 

    case 30: // Read byte data I2C Communication on the GPIO Ports(using I2C eeprom 24LC025)
      /************************************************************
         Ensure to set the GPIO bits to 1 in the CFG register group.  
      *************************************************************/     
      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      {
        //Communication control bits and communication data bytes. Refer to the data sheet.        
        BMS_IC[current_ic].com.tx_data[0]= 0x6A; // Icom Start (6) + I2C_address D0 (A0) (Write operation to set the word address)
        BMS_IC[current_ic].com.tx_data[1]= 0x08; // Fcom master NACK(8)  
        BMS_IC[current_ic].com.tx_data[2]= 0x00; // Icom Blank (0) + eeprom address(word address) D1 (0x00)
        BMS_IC[current_ic].com.tx_data[3]= 0x08; // Fcom master NACK(8)
        BMS_IC[current_ic].com.tx_data[4]= 0x6A; // Icom Start (6) + I2C_address D2 (0xA1)(Read operation)
        BMS_IC[current_ic].com.tx_data[5]= 0x18; // Fcom master NACK(8)  
      }
      wakeup_sleep(TOTAL_IC);         
      LTC6811_wrcomm(TOTAL_IC,BMS_IC); // write to comm register 

      wakeup_idle(TOTAL_IC);
      LTC6811_stcomm(3); // data length=3 // initiates communication between master and the I2C slave 

      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      { 
        //Communication control bits and communication data bytes. Refer to the data sheet.       
        BMS_IC[current_ic].com.tx_data[0]= 0x0F; // Icom Blank (0) + data D0 (FF)
        BMS_IC[current_ic].com.tx_data[1]= 0xF9; // Fcom master NACK + Stop(9) 
        BMS_IC[current_ic].com.tx_data[2]= 0x7F; // Icom No Transmit (7) + data D1 (FF)
        BMS_IC[current_ic].com.tx_data[3]= 0xF9; // Fcom master NACK + Stop(9)
        BMS_IC[current_ic].com.tx_data[4]= 0x7F; // Icom No Transmit (7) + data D2 (FF)
        BMS_IC[current_ic].com.tx_data[5]= 0xF9; // Fcom master NACK + Stop(9) 
      }  

      wakeup_idle(TOTAL_IC);
      LTC6811_wrcomm(TOTAL_IC,BMS_IC); // write to comm register

      wakeup_idle(TOTAL_IC);
      LTC6811_stcomm(1); // data length=1 // initiates communication between master and the I2C slave  

      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcomm(TOTAL_IC,BMS_IC); // read from comm register                
      check_error(error);
      print_rxcomm(); // print received data from the comm register    
      break;    

    case 31: // Set or reset the gpio pins(to drive output on gpio pins)
      /***********************************************************************
       Please ensure you have set the GPIO bits according to your requirement 
       in the configuration register.( check the global variable GPIOBITS_A )
      ************************************************************************/   
      wakeup_sleep(TOTAL_IC);
      for (uint8_t current_ic = 0; current_ic<TOTAL_IC;current_ic++) 
      {
        LTC6811_set_cfgr(current_ic,BMS_IC,REFON,ADCOPT,GPIOBITS_A,DCCBITS_A, DCTOBITS, UV, OV);
      } 
      wakeup_idle(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,BMS_IC);
      print_wrconfig();
      break;
        case 41:
        ack |= menu_1_automatic_mode(mAh_or_Coulombs, celcius_or_kelvin, prescalar_mode, prescalarValue, alcc_mode);  //! Automatic Mode
        break;
      case 42:
        ack |= menu_2_scan_mode(mAh_or_Coulombs, celcius_or_kelvin, prescalar_mode, prescalarValue, alcc_mode);      //! Scan Mode
        break;
      case 43:
        ack |= menu_3_manual_mode(mAh_or_Coulombs, celcius_or_kelvin, prescalar_mode, prescalarValue, alcc_mode);    //! Manual Mode
        break;
      case 44:
        ack |= menu_4_sleep_mode(mAh_or_Coulombs, prescalar_mode, prescalarValue, alcc_mode);                        //! Sleep Mode
        break;
      case 45:
        ack |= menu_5_shutdown_mode();                                                                                //! Shutdown Mode
        break;
      case 46:
        ack |= menu_6_settings(&mAh_or_Coulombs, &celcius_or_kelvin, &prescalar_mode, &prescalarValue, &alcc_mode);  //! Settings Mode
        break;

      case 47: // Loop Measurements of configuration register or cell voltages or auxiliary register or status register without data-log output
        wakeup_sleep(TOTAL_IC);
        LTC6811_wrcfg(TOTAL_IC,BMS_IC);
        measurement_loop2(DATALOG_DISABLED, mAh_or_Coulombs, celcius_or_kelvin, prescalar_mode, prescalarValue, alcc_mode);
        
        print_menu();

        break;
    case 'm': //prints menu
      print_menu();
      break;

    default:
      char str_error[]="Incorrect Option \n";
      serial_print_text(str_error);
      break;

  }
        if (ack != 0)   {                                                    //! If ack is not recieved print an error.
        Serial.println(ack_error);
      Serial.print(F("*************************"));
      //print_prompt();
    }
}


void measurement_loop2(uint8_t datalog_en, int8_t mAh_or_Coulombs , int8_t celcius_or_kelvin ,uint16_t prescalar_mode,uint16_t prescalarValue, uint16_t alcc_mode)
{
  int8_t error = 0;
  char input = 0;
  
  Serial.println(F("Transmit 'm' to quit"));
  
  while (input != 'm')
  {
     if (Serial.available() > 0)
      {
        input = read_char();
      } 
    if (WRITE_CONFIG == ENABLED)
    {
      wakeup_sleep(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,BMS_IC);
      print_wrconfig();
    }
  
    if (READ_CONFIG == ENABLED)
    {
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,BMS_IC);
      check_error(error);
      print_rxconfig();
    }
  
    if (MEASURE_CELL == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      LTC6811_adcv(ADC_CONVERSION_MODE,ADC_DCP,CELL_CH_TO_CONVERT);
      LTC6811_pollAdc();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcv(SEL_ALL_REG, TOTAL_IC,BMS_IC);
      check_error(error);
      print_cells(datalog_en);
      // BLE_cells will print Cell measurements to Serial1 which is where the Bluetooth is connected.
      BLE_cells(datalog_en); 
    }
  
    if (MEASURE_AUX == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      LTC6811_adax(ADC_CONVERSION_MODE , AUX_CH_ALL);
      LTC6811_pollAdc();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdaux(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Set to read back all aux registers
      check_error(error);
      print_aux(datalog_en);
    }
  
    if (MEASURE_STAT == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      LTC6811_adstat(ADC_CONVERSION_MODE, STAT_CH_ALL);
      LTC6811_pollAdc();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdstat(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Set to read back all aux registers
      check_error(error);
      print_stat();
    }
  
    if (PRINT_PEC == ENABLED)
    {
      print_pec_error_count();
    }
    {
  int8_t LTC2944_mode;
  int8_t ack = 0;
  LTC2944_mode = LTC2944_SCAN_MODE|prescalar_mode|alcc_mode ;                           //! Set the control mode of the LTC2944 to scan mode as well as set prescalar and AL#/CC# pin values.
  Serial.println();
  ack |= LTC2944_write(LTC2944_I2C_ADDRESS, LTC2944_CONTROL_REG, LTC2944_mode);          //! Writes the set mode to the LTC2944 control register

  //do
  //{
    Serial.print(F("*************************\n\n"));

    uint8_t status_code;
    uint16_t charge_code, current_code, voltage_code, temperature_code;


    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_ACCUM_CHARGE_MSB_REG, &charge_code);         //! Read MSB and LSB Accumulated Charge Registers for 16 bit charge code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_VOLTAGE_MSB_REG, &voltage_code);             //! Read MSB and LSB Voltage Registers for 16 bit voltage code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_CURRENT_MSB_REG, &current_code);             //! Read MSB and LSB Current Registers for 16 bit current code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_TEMPERATURE_MSB_REG, &temperature_code);     //! Read MSB and LSB Temperature Registers for 16 bit temperature code
    ack |= LTC2944_read(LTC2944_I2C_ADDRESS, LTC2944_STATUS_REG, &status_code);                           //! Read Status Registers for 8 bit status code

    float charge, current, voltage, temperature;
    if (mAh_or_Coulombs)
    {
      charge = LTC2944_code_to_coulombs(charge_code, resistor, prescalarValue);                             //! Convert charge code to Coulombs if Coulomb units are desired.
      Serial.print("Coulombs: ");
      Serial.print(charge, 4);
      Serial.print(F(" C\n"));
    }
    else
    {
      charge = LTC2944_code_to_mAh(charge_code, resistor, prescalarValue);                                  //! Convert charge code to mAh if mAh units are desired.
      Serial.print("mAh: ");
      Serial.print(charge, 4);
      Serial.print(F(" mAh\n"));
    }


    current = LTC2944_code_to_current(current_code, resistor);                                           //! Convert current code to Amperes
    voltage = LTC2944_code_to_voltage(voltage_code);                                                     //! Convert voltage code to Volts


    Serial.print(F("Current "));
    Serial.print(current, 4);
    Serial.print(F(" A\n"));

    Serial.print(F("Voltage "));
    Serial.print(voltage, 4);
    Serial.print(F(" V\n"));


    if (celcius_or_kelvin)
    {
      temperature = LTC2944_code_to_kelvin_temperature(temperature_code);                             //! Convert temperature code to Kelvin if Kelvin units are desired.
      Serial.print(F("Temperature "));
      Serial.print(temperature, 4);
      Serial.print(F(" K\n"));
    }
    else
    {
      temperature = LTC2944_code_to_celcius_temperature(temperature_code);                           //! Convert temperature code to Celcius if Celcius units are desired.
      Serial.print(F("Temperature "));
      Serial.print(temperature, 4);
      Serial.print(F(" C\n"));
    }
    checkAlerts(status_code);                                                                          //! Check status code for Alerts. If an Alert has been set, print out appropriate message in the Serial Prompt

    Serial.print(F("m-Main Menu\n\n"));

    Serial.flush();
    delay(SCAN_MODE_DISPLAY_DELAY);
  }
  //while (Serial.available() == false && !(ack));
  //read_int();  // clears the Serial.available
 // return(ack);
    
    //delay(MEASUREMENT_LOOP_TIME);

  //}
}
}


/*!**********************************************************************************************************************************************
 \brief For writing/reading configuration data or measuring cell voltages or reading aux register or reading status register in a continuous loop  
 @return void
*************************************************************************************************************************************************/



void measurement_loop(uint8_t datalog_en)
{
  int8_t error = 0;
  char input = 0;
  
  Serial.println(F("Transmit 'm' to quit"));
  
  while (input != 'm')
  {
     if (Serial.available() > 0)
      {
        input = read_char();
      } 
    if (WRITE_CONFIG == ENABLED)
    {
      wakeup_sleep(TOTAL_IC);
      LTC6811_wrcfg(TOTAL_IC,BMS_IC);
      print_wrconfig();
    }
  
    if (READ_CONFIG == ENABLED)
    {
      wakeup_sleep(TOTAL_IC);
      error = LTC6811_rdcfg(TOTAL_IC,BMS_IC);
      check_error(error);
      print_rxconfig();
    }
  
    if (MEASURE_CELL == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      LTC6811_adcv(ADC_CONVERSION_MODE,ADC_DCP,CELL_CH_TO_CONVERT);
      LTC6811_pollAdc();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdcv(SEL_ALL_REG, TOTAL_IC,BMS_IC);
      check_error(error);
      print_cells(datalog_en);
      // BLE_cells will print Cell measurements to Serial1 which is where the Bluetooth is connected.
      BLE_cells(datalog_en); 
    }
  
    if (MEASURE_AUX == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      LTC6811_adax(ADC_CONVERSION_MODE , AUX_CH_ALL);
      LTC6811_pollAdc();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdaux(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Set to read back all aux registers
      check_error(error);
      print_aux(datalog_en);
    }
  
    if (MEASURE_STAT == ENABLED)
    {
      wakeup_idle(TOTAL_IC);
      LTC6811_adstat(ADC_CONVERSION_MODE, STAT_CH_ALL);
      LTC6811_pollAdc();
      wakeup_idle(TOTAL_IC);
      error = LTC6811_rdstat(SEL_ALL_REG,TOTAL_IC,BMS_IC); // Set to read back all aux registers
      check_error(error);
      print_stat();
    }
  
    if (PRINT_PEC == ENABLED)
    {
      print_pec_error_count();
    }
    

    delay(MEASUREMENT_LOOP_TIME);
  }
}

/*!*********************************
  \brief Prints the main menu
 @return void
***********************************/
void print_menu(void)
{
  Serial.println(F("\n*****************************************************************"));
  Serial.print(F("* DC1812A and LTC6811Demonstration Program  by VS AE              *\n"));
  Serial.print(F("*                                                               *\n"));
  Serial.print(F("* This program communicates with the LTC2944 Multicell Coulomb  *\n"));
  Serial.print(F("* Counter found on the DC1812A demo board.                      *\n"));
  Serial.print(F("* Set the baud rate to 115200 and select the newline terminator.*\n"));
  Serial.print(F("*                                                               *\n"));
  Serial.print(F("*****************************************************************\n"));
  Serial.println(F("List of 6811 Commands: "));
  Serial.println(F("Write and Read Configuration: 1                            |Loop measurements with data-log output: 12     |Set Discharge: 23"));                         
  Serial.println(F("Read Configuration: 2                                      |Clear Registers: 13                            |Clear Discharge: 24"));
  Serial.println(F("Start Cell Voltage Conversion: 3                           |Read CV,AUX and ADSTAT Voltages: 14            |Write and Read of PWM: 25"));                 
  Serial.println(F("Read Cell Voltages: 4                                      |Run Mux Self Test: 15                          |Write and Read of S control: 26"));    
  Serial.println(F("Start Aux Voltage Conversion: 5                            |Run ADC Self Test: 16                          |Clear S control register: 27")); 
  Serial.println(F("Read Aux Voltages: 6                                       |ADC overlap Test : 17                          |SPI Communication: 28"));    
  Serial.println(F("Start Stat Voltage Conversion: 7                           |Run Digital Redundancy Test: 18                |I2C Communication Write to Slave: 29"));             
  Serial.println(F("Read Stat Voltages: 8                                      |Open Wire Test for single cell detection: 19   |I2C Communication Read from Slave:30"));                        
  Serial.println(F("Start Combined Cell Voltage and GPIO1, GPIO2 Conversion: 9 |Open Wire Test for multiple cell detection: 20 |Set or Reset the GPIO pins: 31 ")); 
  Serial.println(F("Start  Cell Voltage and Sum of cells : 10                  |Print PEC Counter: 21                          |"));
  Serial.println(F("Loop Measurements: 11                                      |Reset PEC Counter: 22                          | \n "));
  Serial.println(F("List of 2944 Commands: "));
  Serial.print(F("\n41-Automatic Mode\n"));
  Serial.print(F("42-Scan Mode\n"));
  Serial.print(F("43-Manual Mode\n"));
  Serial.print(F("44-Sleep Mode\n"));
  Serial.print(F("45-Shutdown Mode\n"));
  Serial.print(F("46-Settings\n"));
  //Serial.print(F("Enter a command: "));
  
  Serial.println(F("Print 'm' for menu"));
  Serial.println(F("Please enter command: \n"));
}

/*!******************************************************************************
 \brief Prints the configuration data that is going to be written to the LTC6811
 to the serial port.
 @return void
 ********************************************************************************/
void print_wrconfig(void)
{
  int cfg_pec;

  Serial.println(F("Written Configuration: "));
  for (int current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F("CFGA IC "));
    Serial.print(current_ic+1,DEC);
    for(int i = 0;i<6;i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].config.tx_data[i]);
    }
    Serial.print(F(", Calculated PEC: 0x"));
    cfg_pec = pec15_calc(6,&BMS_IC[current_ic].config.tx_data[0]);
    serial_print_hex((uint8_t)(cfg_pec>>8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(cfg_pec));
    Serial.println("\n");
  }
}

/*!*****************************************************************
 \brief Prints the configuration data that was read back from the
 LTC6811 to the serial port.
  @return void
 *******************************************************************/
void print_rxconfig(void)
{
  Serial.println(F("Received Configuration "));
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F("CFGA IC "));
    Serial.print(current_ic+1,DEC);
    for(int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].config.rx_data[i]);
    }
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(BMS_IC[current_ic].config.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].config.rx_data[7]);
    Serial.println("\n");
  }
}

/*!************************************************************
  \brief Prints cell voltage to the serial port
   @return void
 *************************************************************/
void print_cells(uint8_t datalog_en) {
//unsigned long int time = millis();
DateTime now = rtc.now(); //print timestamp

  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (datalog_en == 0)
    {
      Serial.println(String("DateTime:\t")+now.timestamp(DateTime::TIMESTAMP_FULL));

      Serial.print(" IC ");
      Serial.print(current_ic+1,DEC);
      Serial.print(": ");      for (int i=0; i< BMS_IC[0].ic_reg.cell_channels; i++)
      {
        Serial.print(" C");
        Serial.print(i+1,DEC);
        Serial.print(":");        
        Serial.print(BMS_IC[current_ic].cells.c_codes[i]*0.0001,4);
        Serial.print(",");
      }
      Serial.println();
    }
    else
    {
      Serial.print(" Cells :");
      for (int i=0; i<BMS_IC[0].ic_reg.cell_channels; i++)
      {
        Serial.print(BMS_IC[current_ic].cells.c_codes[i]*0.0001,4);
        Serial.print(",");
      }
    }
  }
  Serial.println("\n");
}

/*!************************************************************
  \brief Prints cell voltage to the serial1 port for BLE
   @return void
 *************************************************************/
void BLE_cells(uint8_t datalog_en)
{
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (datalog_en == 0)
    {
      Serial1.print(" IC ");
      Serial1.print(current_ic+1,DEC);
      Serial1.print(": ");      for (int i=0; i< BMS_IC[0].ic_reg.cell_channels; i++)
      {
        Serial1.print(" C");
        Serial1.print(i+1,DEC);
        Serial1.print(":");        
        Serial1.print(BMS_IC[current_ic].cells.c_codes[i]*0.0001,4);
        Serial1.print(",");
      }
      Serial1.println();
    }
    else
    {
      Serial1.print(" Cells :");
      for (int i=0; i<BMS_IC[0].ic_reg.cell_channels; i++)
      {
        Serial1.print(BMS_IC[current_ic].cells.c_codes[i]*0.0001,4);
        Serial1.print(",");
      }
    }
  }
  Serial1.println("\n");
}

/*!****************************************************************************
  \brief Prints GPIO voltage codes and Vref2 voltage code onto the serial port
 @return void
 *****************************************************************************/
void print_aux(uint8_t datalog_en)
{

  for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (datalog_en == 0)
    {
      Serial.print(" IC ");
      Serial.print(current_ic+1,DEC);
      Serial.print(":");
      
      for (int i=0; i < 5; i++)
      {
        Serial.print(F(" GPIO-"));
        Serial.print(i+1,DEC);
        Serial.print(":");
        Serial.print(BMS_IC[current_ic].aux.a_codes[i]*0.0001,4);
        Serial.print(",");
      }
      Serial.print(F(" Vref2"));
      Serial.print(":");
      Serial.print(BMS_IC[current_ic].aux.a_codes[5]*0.0001,4);
      Serial.println();
    }
    else
    {
      Serial.print("AUX ");
      Serial.print(" IC ");
      Serial.print(current_ic+1,DEC);
      Serial.print(": ");

      for (int i=0; i < 6; i++)
      {
        Serial.print(BMS_IC[current_ic].aux.a_codes[i]*0.0001,4);
        Serial.print(",");
      }
    }
  }
  Serial.println("\n");
}

/*!****************************************************************************
  \brief Prints Status voltage codes and Vref2 voltage code onto the serial port
 @return void
 *****************************************************************************/
void print_stat(void)
{
   double itmp;
  for (uint8_t current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": "));
    Serial.print(F(" SOC:"));
    Serial.print(BMS_IC[current_ic].stat.stat_codes[0]*0.0001*20,4);
    Serial.print(F(","));
    Serial.print(F(" Itemp:"));
    itmp = (double)((BMS_IC[current_ic].stat.stat_codes[1] * (0.0001 / 0.0075)) - 273);   //Internal Die Temperature(°C) = itmp • (100 µV / 7.5mV)°C - 273°C
    Serial.print(itmp,4);
    Serial.print(F(","));
    Serial.print(F(" VregA:"));
    Serial.print(BMS_IC[current_ic].stat.stat_codes[2]*0.0001,4);
    Serial.print(F(","));
    Serial.print(F(" VregD:"));
    Serial.print(BMS_IC[current_ic].stat.stat_codes[3]*0.0001,4);
    Serial.println();
    Serial.print(F(" Flags:"));
    Serial.print(F(" 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.flags[0]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.flags[1]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.flags[2]);
    Serial.print(F("   Mux fail flag:"));
    Serial.print(F(" 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.mux_fail[0]);
    Serial.print(F("   THSD:"));
    Serial.print(F(" 0x"));
    serial_print_hex(BMS_IC[current_ic].stat.thsd[0]);
    Serial.println("\n");  
  }
}

/*!****************************************************************************
  \brief Prints Status voltage codes for SOC onto the serial port
 @return void
 *****************************************************************************/
void print_sumofcells(void)
{
 for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(": "));
    Serial.print(F(" SOC:"));
    Serial.print(BMS_IC[current_ic].stat.stat_codes[0]*0.0001*20,4);
    Serial.print(F(","));
  }
  Serial.println("\n");
}

/*!****************************************************************
  \brief Function to check the MUX fail bit in the Status Register
   @return void
*******************************************************************/
void check_mux_fail(void)
{ 
  int8_t error = 0;
  for (int ic = 0; ic<TOTAL_IC; ic++)
    { 
      Serial.print(" IC ");
      Serial.println(ic+1,DEC);
      if (BMS_IC[ic].stat.mux_fail[0] != 0) error++;
    
      if (error==0) Serial.println(F("Mux Test: PASS \n"));
      else Serial.println(F("Mux Test: FAIL \n"));
    }
}

/*!************************************************************
  \brief Prints Errors Detected during self test
   @return void
*************************************************************/
void print_selftest_errors(uint8_t adc_reg ,int8_t error)
{
  if(adc_reg==1)
  {
    Serial.println("Cell ");
    }
  else if(adc_reg==2)
  {
    Serial.println("Aux ");
    }
  else if(adc_reg==3)
  {
    Serial.println("Stat ");
    }
  Serial.print(error, DEC);
  Serial.println(F(" : errors detected in Digital Filter and Memory \n"));
}

/*!************************************************************
  \brief Prints the output of  the ADC overlap test  
   @return void
*************************************************************/
void print_overlap_results(int8_t error)
{
  if (error==0) Serial.println(F("Overlap Test: PASS \n"));
  else Serial.println(F("Overlap Test: FAIL \n"));
}

/*!************************************************************
  \brief Prints Errors Detected during Digital Redundancy test
   @return void
*************************************************************/
void print_digital_redundancy_errors(uint8_t adc_reg ,int8_t error)
{
  if(adc_reg==2)
  {
    Serial.println("Aux ");
    }
  else if(adc_reg==3)
  {
    Serial.println("Stat ");
    }

  Serial.print(error, DEC);
  Serial.println(F(" : errors detected in Measurement \n"));
}

/*****************************************************************************
  \brief Prints Open wire test results to the serial port
  @return void
 *****************************************************************************/
void print_open_wires(void)
{
  for (int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    if (BMS_IC[current_ic].system_open_wire == 65535)
    {
      Serial.print("No Opens Detected on IC ");
      Serial.println(current_ic+1, DEC);
    }
    else
    {
      Serial.print(F("There is an open wire on IC "));
      Serial.print(current_ic + 1,DEC);
      Serial.print(F(" Channel: "));
      Serial.println(BMS_IC[current_ic].system_open_wire);
    }
    Serial.println("\n");
  }
}

/*!************************************************************
  \brief Prints the PEC errors detected to the serial port
  @return void
 *************************************************************/ 
void print_pec_error_count(void)
{
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.println("");
    Serial.print(BMS_IC[current_ic].crc_count.pec_count,DEC);
    Serial.print(F(" : PEC Errors Detected on IC"));
    Serial.println(current_ic+1,DEC);
  }
  Serial.println("\n");
}

/*!****************************************************
  \brief Function to select the S pin for discharge
  @return void
 ******************************************************/
int8_t select_s_pin(void)
{
  int8_t read_s_pin=0;
  
  Serial.print(F("Please enter the Spin number: "));
  read_s_pin = (int8_t)read_int();
  Serial.println(read_s_pin);
  return(read_s_pin);
}

/*!******************************************************************************
 \brief Prints  PWM the configuration data that is going to be written to the LTC6811
 to the serial port.
  @return void
 ********************************************************************************/
void print_wrpwm(void)
{
  int pwm_pec;

  Serial.println(F("Written PWM Configuration: "));
  for (uint8_t current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F("IC "));
    Serial.print(current_ic+1,DEC);
    for(int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
     serial_print_hex(BMS_IC[current_ic].pwm.tx_data[i]);
    }
    Serial.print(F(", Calculated PEC: 0x"));
    pwm_pec = pec15_calc(6,&BMS_IC[current_ic].pwm.tx_data[0]);
    serial_print_hex((uint8_t)(pwm_pec>>8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(pwm_pec));
    Serial.println("\n");
  } 
}

/*!*****************************************************************
 \brief Prints the PWM configuration data that was read back from the
 LTC6811 to the serial port.
 @return void
 *******************************************************************/
void print_rxpwm(void)
{
  Serial.println(F("Received pwm Configuration:"));
  for (uint8_t current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F("IC "));
    Serial.print(current_ic+1,DEC);
    for(int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
     serial_print_hex(BMS_IC[current_ic].pwm.rx_data[i]);
    }
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(BMS_IC[current_ic].pwm.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].pwm.rx_data[7]);
    Serial.println("\n");
  }
}

/*!************************************************************
  \brief Prints S control register data to the serial port
  @return void
 *************************************************************/ 
void print_wrsctrl(void)
{
   int sctrl_pec;

  Serial.println(F("Written Data in Sctrl register: "));
  for (int current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC: "));
    Serial.print(current_ic+1,DEC);
    Serial.print(F(" Sctrl register group:"));
    for(int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].sctrl.tx_data[i]);
    }
    
    Serial.print(F(", Calculated PEC: 0x"));
    sctrl_pec = pec15_calc(6,&BMS_IC[current_ic].sctrl.tx_data[0]);
    serial_print_hex((uint8_t)(sctrl_pec>>8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(sctrl_pec));
    Serial.println("\n");
  }
}

/*!************************************************************
  \brief Prints s control register data that was read back from the
 LTC6811 to the serial port.
@return void
 *************************************************************/
void print_rxsctrl(void)
{
  Serial.println(F("Received Data:"));
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC "));
    Serial.print(current_ic+1,DEC);
    
    for(int i = 0; i < 6; i++)
    {
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].sctrl.rx_data[i]);
    }
    
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(BMS_IC[current_ic].sctrl.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].sctrl.rx_data[7]);
    Serial.println("\n");
  }
}

/*!************************************************************
  \brief Prints comm register data to the serial port
  @return void
 *************************************************************/ 
void print_wrcomm(void)
{
  int comm_pec;

  Serial.println(F("Written Data in COMM Register: "));
  for (int current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC- "));
    Serial.print(current_ic+1,DEC);
    
    for(int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].com.tx_data[i]);
    }
    Serial.print(F(", Calculated PEC: 0x"));
    comm_pec = pec15_calc(6,&BMS_IC[current_ic].com.tx_data[0]);
    serial_print_hex((uint8_t)(comm_pec>>8));
    Serial.print(F(", 0x"));
    serial_print_hex((uint8_t)(comm_pec));
    Serial.println("\n");
  }
}

/*!************************************************************
  \brief Prints comm register data that was read back from the
 LTC6811 to the serial port. 
 @return void
 *************************************************************/
void print_rxcomm(void)
{
  Serial.println(F("Received Data in COMM register:"));
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(F(" IC- "));
    Serial.print(current_ic+1,DEC);
    
    for(int i = 0; i < 6; i++)
    {
      Serial.print(F(", 0x"));
      serial_print_hex(BMS_IC[current_ic].com.rx_data[i]);
    }
    Serial.print(F(", Received PEC: 0x"));
    serial_print_hex(BMS_IC[current_ic].com.rx_data[6]);
    Serial.print(F(", 0x"));
    serial_print_hex(BMS_IC[current_ic].com.rx_data[7]);
    Serial.println("\n");
  }
}

/*!****************************************************************************
  \brief Function to print the Conversion Time
  @return void
 *****************************************************************************/
void print_conv_time(uint32_t conv_time)
{
  uint16_t m_factor=1000;  // to print in ms

  Serial.print(F("Conversion completed in:"));
  Serial.print(((float)conv_time/m_factor), 1);
  Serial.println(F("ms \n"));
}

/*!************************************************************
  \brief Function to check error flag and print PEC error message
  @return void
 *************************************************************/ 
void check_error(int error)
{
  if (error == -1)
  {
    Serial.println(F("A PEC error was detected in the received data"));
  }
}

/*!************************************************************
  \brief Function to print text on serial monitor
  @return void
*************************************************************/ 
void serial_print_text(char data[])
{       
  Serial.println(data);
}

/*!************************************************************
 \brief Function to print in HEX form
 @return void
 *************************************************************/ 
void serial_print_hex(uint8_t data)
{
  if (data< 16)
  {
    Serial.print("0");
    Serial.print((byte)data,HEX);
  }
  else
    Serial.print((byte)data,HEX);
}

/*!************************************************************
 \brief Hex conversion constants
 *************************************************************/
char hex_digits[16]=
{
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*!************************************************************
 \brief Global Variables
 *************************************************************/
char hex_to_byte_buffer[5]=
{
  '0', 'x', '0', '0', '\0'
};               

/*!************************************************************
 \brief Buffer for ASCII hex to byte conversion
 *************************************************************/
char byte_to_hex_buffer[3]=
{
  '\0','\0','\0'
};

/*!************************************************************
 \brief Read 2 hex characters from the serial buffer and convert them to a byte
 @return char data Read Data
 *************************************************************/
char read_hex(void)    
{
  byte data;
  hex_to_byte_buffer[2]=get_char();
  hex_to_byte_buffer[3]=get_char();
  get_char();
  get_char();
  data = strtol(hex_to_byte_buffer, NULL, 0);
  return(data);
}

/*!************************************************************
 \brief Read a command from the serial port
 @return char 
 *************************************************************/
char get_char(void)
{
  while (Serial.available() <= 0);
  return(Serial.read());
}


//! Automatic Mode.
int8_t menu_1_automatic_mode(int8_t mAh_or_Coulombs, int8_t celcius_or_kelvin ,uint16_t prescalar_mode, uint16_t prescalarValue, uint16_t alcc_mode)
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge.
{
  int8_t LTC2944_mode;
  int8_t ack = 0;
  LTC2944_mode = LTC2944_AUTOMATIC_MODE|prescalar_mode|alcc_mode ;               //! Set the control register of the LTC2944 to automatic mode as well as set prescalar and AL#/CC# pin values.
  Serial.println();
  ack |= LTC2944_write(LTC2944_I2C_ADDRESS, LTC2944_CONTROL_REG, LTC2944_mode);   //! Writes the set mode to the LTC2944 control register

  do
  {
    Serial.print(F("*************************\n\n"));

    uint8_t status_code, hightemp_code, lowtemp_code;
    uint16_t charge_code, current_code, voltage_code, temperature_code;

    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_ACCUM_CHARGE_MSB_REG, &charge_code);     //! Read MSB and LSB Accumulated Charge Registers for 16 bit charge code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_VOLTAGE_MSB_REG, &voltage_code);         //! Read MSB and LSB Voltage Registers for 16 bit voltage code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_CURRENT_MSB_REG, &current_code);         //! Read MSB and LSB Current Registers for 16 bit current code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_TEMPERATURE_MSB_REG, &temperature_code); //! Read MSB and LSB Temperature Registers for 16 bit temperature code
    ack |= LTC2944_read(LTC2944_I2C_ADDRESS, LTC2944_STATUS_REG, &status_code);                       //! Read Status Register for 8 bit status code


    float charge, current, voltage, temperature;
    if (mAh_or_Coulombs)
    {
      charge = LTC2944_code_to_coulombs(charge_code, resistor, prescalarValue); //! Convert charge code to Coulombs if Coulomb units are desired.
      Serial.print("Coulombs: ");
      Serial.print(charge, 4);
      Serial.print(F(" C\n"));
    }
    else
    {
      charge = LTC2944_code_to_mAh(charge_code, resistor, prescalarValue);      //! Convert charge code to mAh if mAh units are desired.
      Serial.print("mAh: ");
      Serial.print(charge, 4);
      Serial.print(F(" mAh\n"));
    }


    current = LTC2944_code_to_current(current_code, resistor);                //! Convert current code to Amperes
    voltage = LTC2944_code_to_voltage(voltage_code);                          //! Convert voltage code to Volts

    Serial.print(F("Current "));
    Serial.print(current, 4);
    Serial.print(F(" A\n"));

    Serial.print(F("Voltage "));
    Serial.print(voltage, 4);
    Serial.print(F(" V\n"));


    if (celcius_or_kelvin)
    {
      temperature = LTC2944_code_to_kelvin_temperature(temperature_code);   //! Convert temperature code to kelvin
      Serial.print(F("Temperature "));
      Serial.print(temperature, 4);
      Serial.print(F(" K\n"));
    }
    else
    {
      temperature = LTC2944_code_to_celcius_temperature(temperature_code);  //! Convert temperature code to celcius
      Serial.print(F("Temperature "));
      Serial.print(temperature, 4);
      Serial.print(F(" C\n"));
    }

    checkAlerts(status_code);                                                   //! Check status code for Alerts. If an Alert has been set, print out appropriate message in the Serial Prompt.

    Serial.print(F("m-Main Menu\n\n"));

    Serial.flush();
    delay(AUTOMATIC_MODE_DISPLAY_DELAY);                                      //! Delay for 1s before next polling
  }
  while (Serial.available() == false && !(ack));                                 //! if Serial is not available and an NACK has not been recieved, keep polling the registers.
  read_int();  // clears the Serial.available
  return(ack);
}

//! Scan Mode
int8_t menu_2_scan_mode(int8_t mAh_or_Coulombs , int8_t celcius_or_kelvin ,uint16_t prescalar_mode,uint16_t prescalarValue, uint16_t alcc_mode)
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t LTC2944_mode;
  int8_t ack = 0;
  LTC2944_mode = LTC2944_SCAN_MODE|prescalar_mode|alcc_mode ;                           //! Set the control mode of the LTC2944 to scan mode as well as set prescalar and AL#/CC# pin values.
  Serial.println();
  ack |= LTC2944_write(LTC2944_I2C_ADDRESS, LTC2944_CONTROL_REG, LTC2944_mode);          //! Writes the set mode to the LTC2944 control register

  do
  {
    Serial.print(F("*************************\n\n"));

    uint8_t status_code;
    uint16_t charge_code, current_code, voltage_code, temperature_code;


    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_ACCUM_CHARGE_MSB_REG, &charge_code);         //! Read MSB and LSB Accumulated Charge Registers for 16 bit charge code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_VOLTAGE_MSB_REG, &voltage_code);             //! Read MSB and LSB Voltage Registers for 16 bit voltage code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_CURRENT_MSB_REG, &current_code);             //! Read MSB and LSB Current Registers for 16 bit current code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_TEMPERATURE_MSB_REG, &temperature_code);     //! Read MSB and LSB Temperature Registers for 16 bit temperature code
    ack |= LTC2944_read(LTC2944_I2C_ADDRESS, LTC2944_STATUS_REG, &status_code);                           //! Read Status Registers for 8 bit status code

    float charge, current, voltage, temperature;
    if (mAh_or_Coulombs)
    {
      charge = LTC2944_code_to_coulombs(charge_code, resistor, prescalarValue);                             //! Convert charge code to Coulombs if Coulomb units are desired.
      Serial.print("Coulombs: ");
      Serial.print(charge, 4);
      Serial.print(F(" C\n"));
    }
    else
    {
      charge = LTC2944_code_to_mAh(charge_code, resistor, prescalarValue);                                  //! Convert charge code to mAh if mAh units are desired.
      Serial.print("mAh: ");
      Serial.print(charge, 4);
      Serial.print(F(" mAh\n"));
    }


    current = LTC2944_code_to_current(current_code, resistor);                                           //! Convert current code to Amperes
    voltage = LTC2944_code_to_voltage(voltage_code);                                                     //! Convert voltage code to Volts


    Serial.print(F("Current "));
    Serial.print(current, 4);
    Serial.print(F(" A\n"));

    Serial.print(F("Voltage "));
    Serial.print(voltage, 4);
    Serial.print(F(" V\n"));


    if (celcius_or_kelvin)
    {
      temperature = LTC2944_code_to_kelvin_temperature(temperature_code);                             //! Convert temperature code to Kelvin if Kelvin units are desired.
      Serial.print(F("Temperature "));
      Serial.print(temperature, 4);
      Serial.print(F(" K\n"));
    }
    else
    {
      temperature = LTC2944_code_to_celcius_temperature(temperature_code);                           //! Convert temperature code to Celcius if Celcius units are desired.
      Serial.print(F("Temperature "));
      Serial.print(temperature, 4);
      Serial.print(F(" C\n"));
    }
    checkAlerts(status_code);                                                                          //! Check status code for Alerts. If an Alert has been set, print out appropriate message in the Serial Prompt

    Serial.print(F("m-Main Menu\n\n"));

    Serial.flush();
    delay(SCAN_MODE_DISPLAY_DELAY);
  }
  while (Serial.available() == false && !(ack));
  read_int();  // clears the Serial.available
  return(ack);

}

//! Manual Mode
int8_t menu_3_manual_mode(int8_t mAh_or_Coulombs ,int8_t celcius_or_kelvin ,uint16_t prescalar_mode, uint16_t prescalarValue, uint16_t alcc_mode)
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t LTC2944_mode;
  int8_t ack = 0;
  LTC2944_mode = LTC2944_MANUAL_MODE|prescalar_mode|alcc_mode ;                                    //! Set the control mode of the LTC2944 to manual mode as well as set prescalar and AL#/CC# pin values.
  Serial.println();
  ack |= LTC2944_write(LTC2944_I2C_ADDRESS, LTC2944_CONTROL_REG, LTC2944_mode);                     //! Writes the set mode to the LTC2944 control register

  int staleData = 0;                                                                                //! Stale Data Check variable. When set to 1 it indicates that stale data is being read from the voltage, current and temperature registers.

  do
  {
    Serial.print(F("*************************\n\n"));

    uint8_t status_code;
    uint16_t charge_code, current_code, voltage_code, temperature_code;


    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_ACCUM_CHARGE_MSB_REG, &charge_code);         //! Read MSB and LSB Accumulated Charge Registers for 16 bit charge code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_VOLTAGE_MSB_REG, &voltage_code);             //! Read MSB and LSB Voltage Registers for 16 bit voltage code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_CURRENT_MSB_REG, &current_code);             //! Read MSB and LSB Current Registers for 16 bit current code
    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_TEMPERATURE_MSB_REG, &temperature_code);     //! Read MSB and LSB Temperature Registers for 16 bit temperature code
    ack |= LTC2944_read(LTC2944_I2C_ADDRESS, LTC2944_STATUS_REG, &status_code);                           //! Read Status Registers for 8 bit status code


    float charge, current, voltage, temperature;
    if (mAh_or_Coulombs)
    {
      charge = LTC2944_code_to_coulombs(charge_code, resistor, prescalarValue);                             //! Convert charge code to Coulombs if Coulomb units are desired.
      Serial.print("Coulombs: ");
      Serial.print(charge, 4);
      Serial.print(F(" C\n"));
    }
    else
    {
      charge = LTC2944_code_to_mAh(charge_code, resistor, prescalarValue);                                  //! Convert charge code to mAh if mAh units are desired.
      Serial.print("mAh: ");
      Serial.print(charge, 4);
      Serial.print(F(" mAh\n"));
    }


    current = LTC2944_code_to_current(current_code, resistor);                                            //! Convert current code to Amperes
    voltage = LTC2944_code_to_voltage(voltage_code);                                                      //! Convert voltage code to Volts


    Serial.print(F("Current "));
    Serial.print(current, 4);
    Serial.print(F(" A"));
    if (staleData) Serial.print(F("     ***** Stale Data ******\n"));                                     //! If Stale data is inside the register after initial snapshot, Print Stale Data message.
    else Serial.println("");

    Serial.print(F("Voltage "));
    Serial.print(voltage, 4);
    Serial.print(F(" V"));
    if (staleData) Serial.print(F("     ***** Stale Data ******\n"));                                     //! If Stale data is inside the register after initial snapshot, Print Stale Data message.
    else Serial.println("");


    if (celcius_or_kelvin)
    {
      temperature = LTC2944_code_to_kelvin_temperature(temperature_code);                               //! Convert temperature code to Kelvin if Kelvin units are desired.
      Serial.print(F("Temperature "));
      Serial.print(temperature, 4);
      Serial.print(F(" K"));
    }
    else
    {
      temperature = LTC2944_code_to_celcius_temperature(temperature_code);                              //! Convert temperature code to Celcius if Celcius units are desired.
      Serial.print(F("Temperature "));
      Serial.print(temperature, 4);
      Serial.print(F(" C"));
    }
    if (staleData) Serial.print(F("   ***** Stale Data ******\n"));
    else Serial.println("");


    checkAlerts(status_code);                                                                             //! Check status code for Alerts. If an Alert has been set, print out appropriate message in the Serial Prompt


    Serial.print(F("m-Main Menu\n\n"));

    staleData = 1;
    Serial.flush();
    delay(AUTOMATIC_MODE_DISPLAY_DELAY);
  }
  while (Serial.available() == false && !(ack));
  read_int();  // clears the Serial.available
  return(ack);
}

//! Sleep Mode
int8_t menu_4_sleep_mode(int8_t mAh_or_Coulombs ,uint16_t prescalar_mode, uint16_t prescalarValue, uint16_t alcc_mode)
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t LTC2944_mode;
  int8_t ack = 0;
  LTC2944_mode = LTC2944_SLEEP_MODE|prescalar_mode|alcc_mode ;                            //! Set the control mode of the LTC2944 to sleep mode as well as set prescalar and AL#/CC# pin values.
  Serial.println();
  ack |= LTC2944_write(LTC2944_I2C_ADDRESS, LTC2944_CONTROL_REG, LTC2944_mode);            //! Writes the set mode to the LTC2944 control register


  do
  {
    Serial.print(F("*************************\n\n"));
    delay(100);
    uint8_t status_code;
    uint16_t charge_code;


    ack |= LTC2944_read_16_bits(LTC2944_I2C_ADDRESS, LTC2944_ACCUM_CHARGE_MSB_REG, &charge_code);     //! Read MSB and LSB Accumulated Charge Registers for 16 bit charge code
    ack |= LTC2944_read(LTC2944_I2C_ADDRESS, LTC2944_STATUS_REG, &status_code);                       //! Read Status Registers for 8 bit status code


    float charge;
    if (mAh_or_Coulombs)
    {
      charge = LTC2944_code_to_coulombs(charge_code, resistor, prescalarValue);                         //! Convert charge code to Coulombs if Coulomb units are desired.
      Serial.print("Coulombs: ");
      Serial.print(charge, 4);
      Serial.print(F(" C\n"));
    }
    else
    {
      charge = LTC2944_code_to_mAh(charge_code, resistor, prescalarValue);                              //! Convert charge code to mAh if mAh units are desired.
      Serial.print("mAh: ");
      Serial.print(charge, 4);
      Serial.print(F(" mAh\n"));
    }

    Serial.print(F("Current "));
    Serial.print(F("     ADC Sleep...\n"));


    Serial.print(F("Voltage "));
    Serial.print(F("     ADC Sleep...\n"));

    Serial.print(F("Temperature "));
    Serial.print(F(" ADC Sleep...\n"));

    Serial.print(F("m-Main Menu\n\n"));

    checkAlerts(status_code);

    Serial.flush();
    delay(AUTOMATIC_MODE_DISPLAY_DELAY);
  }
  while (Serial.available() == false || (ack));
  read_int();  // clears the Serial.available
  return(ack);
}

//! Shutdown Mode
int8_t menu_5_shutdown_mode()
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  ack |= LTC2944_write(LTC2944_I2C_ADDRESS, LTC2944_CONTROL_REG, LTC2944_SHUTDOWN_MODE);                      //! Sets the LTC2944 into shutdown mode
  Serial.print("LTC2944 Has Been ShutDown\n");
  return(ack);
}
//! Settings Menu
int8_t menu_6_settings(uint8_t *mAh_or_Coulombs, uint8_t *celcius_or_kelvin, uint16_t *prescalar_mode, uint16_t *prescalarValue, uint16_t *alcc_mode)
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  int8_t user_command;
  do
  {
    Serial.print(F("*************************\n\n"));
    Serial.print(F("1-Set Alert Thresholds\n"));
    Serial.print(F("2-Set Prescalar Value\n"));
    Serial.print(F("3-Set AL#/CC# Pin State\n"));
    Serial.print(F("4-Set Units\n"));
    Serial.print(F("m-Main Menu\n\n"));
    Serial.print(F("Enter a command: "));

    user_command = read_int();
    if (user_command == 'm')
      Serial.println("m");
    else
      Serial.println(user_command);
    Serial.println();
    switch (user_command)
    {
      case 1:
        ack |= menu_6_settings_menu_1_set_alert_thresholds();                                          //! Settings Menu to set Alert Thresholds
        break;

      case 2:
        ack |= menu_6_settings_menu_2_set_prescalar_values(prescalar_mode, prescalarValue);            //! Settings Menu to set Prescalar Values
        break;

      case 3:
        ack |= menu_6_alert_menu_3_set_allcc_state(alcc_mode);                                        //! Settings Menu to set AL#/CC# mode
        break;

      case 4:
        ack |= menu_6_alert_menu_4_set_units(mAh_or_Coulombs, celcius_or_kelvin);                     //! Settings Menu to set Temperature and Charge Units
        break;

      default:
        if (user_command != 'm')
          Serial.println("Incorrect Option");
        break;
    }
  }
  while (!((user_command == 'm') || (ack)));
  return(ack);

}

//! Alert Threshold Menu
int8_t menu_6_settings_menu_1_set_alert_thresholds()
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  int8_t user_command;
  do
  {
    Serial.print(F("*************************\n\n"));
    Serial.print(F("1-Set Charge Thresholds\n"));
    Serial.print(F("2-Set Voltage Thresholds\n"));
    Serial.print(F("3-Set Current Thresholds\n"));
    Serial.print(F("4-Set Temperature Thresholds\n"));
    Serial.print(F("m-Main Menu\n\n"));
    Serial.print(F("Enter a command: "));

    user_command = read_int();
    if (user_command == 'm')
      Serial.println("m");
    else
      Serial.println(user_command);
    Serial.println();
    switch (user_command)
    {
      case 1:
        ack |= menu_6_alert_menu_1_set_charge_thresholds();                   //! Set Max and Min Charge Thresholds. The ACR charge lsb size changes with respect to the prescalar and sense resistor value. Due to this variability, for the purpose of this demo enter values in hexadecimal.
        break;
      case 2:
        ack |= menu_6_alert_menu_2_set_voltage_thresholds();                  //! Set Max and Min Voltage Thresholds. Enter Values in Volts
        break;
      case 3:
        ack |= menu_6_alert_menu_3_set_current_thresholds();                  //! Set Max and Min Current Thresholds. Enter Values in Amperes.
        break;
      case 4:
        ack |= menu_6_alert_menu_4_set_temperature_thresholds();              //! Set Max and Min Temperature Thresholds. Enter Values in Celcius.
        break;
      default:
        if (user_command != 'm')
          Serial.println("Incorrect Option");
        break;
    }
  }
  while (!((user_command == 'm') || (ack)));
  return(ack);

}
//! Set Charge Threshold Function
int8_t menu_6_alert_menu_1_set_charge_thresholds()
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  Serial.print(F("Enter RAW Max Charge Threshold:"));

  uint16_t max_charge_threshold;
  max_charge_threshold = read_int();                                                                              //! Read user entered value
  Serial.println(max_charge_threshold);

  ack |= LTC2944_write_16_bits(LTC2944_I2C_ADDRESS, LTC2944_CHARGE_THRESH_HIGH_MSB_REG, max_charge_threshold);    //! write user entered value to HIGH threshold register

  Serial.print(F("Enter RAW Min Charge Threshold:"));

  float min_charge_threshold;
  min_charge_threshold = read_int();                                                                              //! Read user entered value
  Serial.println(min_charge_threshold);

  ack |= LTC2944_write_16_bits(LTC2944_I2C_ADDRESS, LTC2944_CHARGE_THRESH_LOW_MSB_REG, min_charge_threshold);     //! write user entered value to HIGH threshold register
  return(ack);

}

//! Set Voltage Thresholds
int8_t menu_6_alert_menu_2_set_voltage_thresholds()
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  Serial.print(F("Enter Max Voltage Threshold:"));

  float max_voltage_threshold;
  max_voltage_threshold = read_float();                                                                                    //! Read user entered value
  Serial.print(max_voltage_threshold, 3);
  Serial.println("V");

  uint16_t max_voltage_threshold_code = max_voltage_threshold*(0xFFFF)/(LTC2944_FULLSCALE_VOLTAGE);                         //! Convert user entered voltage into adc code.

  ack |= LTC2944_write_16_bits(LTC2944_I2C_ADDRESS, LTC2944_VOLTAGE_THRESH_HIGH_MSB_REG, max_voltage_threshold_code);       //! Write adc code to HIGH threshold register

  Serial.print(F("Enter Min Voltage Threshold:"));

  float min_voltage_threshold;
  min_voltage_threshold = read_float();                                                                                     //! Read user entered value
  Serial.println(min_voltage_threshold, 3);
  Serial.println("V");

  uint16_t min_voltage_threshold_code = min_voltage_threshold*(0xFFFF)/(LTC2944_FULLSCALE_VOLTAGE);                         //! Convert user entered voltage into adc code.

  ack |= LTC2944_write_16_bits(LTC2944_I2C_ADDRESS, LTC2944_VOLTAGE_THRESH_LOW_MSB_REG, min_voltage_threshold_code);        //! Write adc code to LOW threshold register
  return(ack);
}
//! Set Current Thresholds
int8_t menu_6_alert_menu_3_set_current_thresholds()
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  Serial.print(F("Enter Max Current Threshold:"));

  float max_current_threshold;
  max_current_threshold = read_float();                                                                                    //! Read user entered value
  Serial.print(max_current_threshold, 3);
  Serial.println("A");

  uint16_t max_current_threshold_code = resistor*max_current_threshold*(0x7FFF)/(LTC2944_FULLSCALE_CURRENT) + 0x7FFF;      //! Convert user entered current into adc code.

  ack |= LTC2944_write_16_bits(LTC2944_I2C_ADDRESS, LTC2944_CURRENT_THRESH_HIGH_MSB_REG, max_current_threshold_code);      //! Write adc code to HIGH threshold register

  Serial.print(F("Enter Min Current Threshold:"));

  float min_current_threshold;
  min_current_threshold = read_float();                                                                                    //! Read user entered value
  Serial.print(min_current_threshold, 3);
  Serial.println("A");

  uint16_t min_current_threshold_code = resistor*min_current_threshold*(0x7FFF)/(LTC2944_FULLSCALE_CURRENT) + 0x7FFF;      //! Convert user entered current into adc code.

  ack |= LTC2944_write_16_bits(LTC2944_I2C_ADDRESS, LTC2944_CURRENT_THRESH_LOW_MSB_REG, min_current_threshold_code);       //! Write adc code to LOW threshold register
  return(ack);
}

//! Set Temperature Thresholds
int8_t menu_6_alert_menu_4_set_temperature_thresholds()
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  Serial.print(F("Enter Max Temperature Threshold in Celcius:"));

  float max_temperature_threshold;
  max_temperature_threshold = read_float();                                                                                  //! Read user entered value
  Serial.print(max_temperature_threshold, 2);
  Serial.println("C");

  uint16_t max_temperature_threshold_code = (max_temperature_threshold + 273.15)*(0xFFFF)/(LTC2944_FULLSCALE_TEMPERATURE);   //! Convert user entered temperature into adc code.


  ack |= LTC2944_write_16_bits(LTC2944_I2C_ADDRESS, LTC2944_TEMPERATURE_THRESH_HIGH_REG, max_temperature_threshold_code);    //! Write adc code to HIGH threshold register

  Serial.print(F("Enter Min Temperature Threshold in Celcius:"));

  float min_temperature_threshold;
  min_temperature_threshold = read_float();                                                                                 //! Read user entered value
  Serial.print(min_temperature_threshold, 2);
  Serial.println("C");

  uint16_t min_temperature_threshold_code = (min_temperature_threshold + 273.15)*(0xFFFF)/(LTC2944_FULLSCALE_TEMPERATURE);  //! Convert user entered temperature into adc code.

  ack |= LTC2944_write_16_bits(LTC2944_I2C_ADDRESS, LTC2944_TEMPERATURE_THRESH_LOW_REG, min_temperature_threshold_code);    //! Write adc code to LOW threshold register
  return(ack);

}

//! Prescalar Menu
int8_t menu_6_settings_menu_2_set_prescalar_values(uint16_t *prescalar_mode, uint16_t *prescalarValue)
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  int8_t user_command;
  do
  {
    Serial.print(F("*************************\n\n"));
    Serial.print(F("1-Set Prescalar M = 1\n"));
    Serial.print(F("2-Set Prescalar M = 4\n"));
    Serial.print(F("3-Set Prescalar M = 16\n"));
    Serial.print(F("4-Set Prescalar M = 64\n"));
    Serial.print(F("5-Set Prescalar M = 256\n"));
    Serial.print(F("6-Set Prescalar M = 1024\n"));
    Serial.print(F("7-Set Prescalar M = 4096\n"));
    Serial.print(F("m-Main Menu\n\n"));
    Serial.print(F("Enter a command: "));

    user_command = read_int();
    if (user_command == 'm')
      Serial.println("m");
    else
      Serial.println(user_command);
    Serial.println();
    switch (user_command)
    {
      case 1:
        *prescalar_mode = LTC2944_PRESCALAR_M_1;                                   //! Set Prescalar Value M = 1
        *prescalarValue = 1;
        Serial.println(F("\nPrescalar Set to 1\n"));
        break;
      case 2:
        *prescalar_mode = LTC2944_PRESCALAR_M_4;                                  //! Set Prescalar Value M = 4
        *prescalarValue = 4;
        Serial.println(F("\nPrescalar Set to 4\n"));
        break;
      case 3:
        *prescalar_mode = LTC2944_PRESCALAR_M_16;                                 //! Set Prescalar Value M = 16
        *prescalarValue = 16;
        Serial.println(F("\nPrescalar Set to 16\n"));
        break;
      case 4:
        *prescalar_mode = LTC2944_PRESCALAR_M_64;                                //! Set Prescalar Value M = 64
        *prescalarValue = 64;
        Serial.println(F("\nPrescalar Set to 64\n"));
        break;
      case 5:
        *prescalar_mode = LTC2944_PRESCALAR_M_256;                               //! Set Prescalar Value M = 256
        *prescalarValue = 256;
        Serial.println(F("\nPrescalar Set to 256\n"));
        break;
      case 6:
        *prescalar_mode = LTC2944_PRESCALAR_M_1024;                              //! Set Prescalar Value M = 1024
        *prescalarValue = 1024;
        \
        Serial.println(F("\nPrescalar Set to 1024\n"));
        break;
      case 7:
        *prescalar_mode = LTC2944_PRESCALAR_M_4096;                              //! Set Prescalar Value M = 4096
        *prescalarValue = 4096;
        Serial.println(F("\nPrescalar Set to 4096\n"));
        break;
      default:
        if (user_command != 'm')
          Serial.println("Incorrect Option");
        break;
    }
  }
  while (!((user_command == 'm') || (ack)));
  return(ack);
}

//! AL#/CC# Pin Mode Menu
uint8_t menu_6_alert_menu_3_set_allcc_state(uint16_t *alcc_mode)
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  int8_t user_command;
  do
  {
    Serial.print(F("*************************\n\n"));
    Serial.print(F("1-Enable Alert Mode\n"));
    Serial.print(F("2-Enable Charge Complete Mode\n"));
    Serial.print(F("3-Disable AL#/CC# Pin\n"));
    Serial.print(F("m-Main Menu\n\n"));
    Serial.print(F("Enter a command: "));

    user_command = read_int();
    if (user_command == 'm')
      Serial.println("m");
    else
      Serial.println(user_command);
    Serial.println();
    switch (user_command)
    {
      case 1:
        *alcc_mode = LTC2944_ALERT_MODE;                         //! Set AL#/CC# mode to Alert Mode
        Serial.println(F("\nAlert Mode Enabled\n"));
        break;
      case 2:
        *alcc_mode = LTC2944_CHARGE_COMPLETE_MODE;               //! Set AL#/CC# mode to Charge Complete Mode
        Serial.println(F("\nCharge Mode Enabled\n"));
        break;
      case 3:
        *alcc_mode = LTC2944_DISABLE_ALCC_PIN;                   //! Disable AL#/CC# pin.
        Serial.println(F("\nAL#/CC# Pin Disabled\n"));
        break;
      default:
        if (user_command != 'm')
          Serial.println("Incorrect Option");
        break;
    }
  }
  while (!((user_command == 'm') || (ack)));
  return(ack);
}

//! Set Charge and Temperature Units Menu
uint8_t menu_6_alert_menu_4_set_units(uint8_t *mAh_or_Coulombs, uint8_t *celcius_or_kelvin)
//! @return Returns the state of the acknowledge bit after the I2C address write. 0=acknowledge, 1=no acknowledge
{
  int8_t ack = 0;
  int8_t user_command;
  do
  {
    Serial.print(F("*************************\n\n"));
    Serial.print(F("1-Set Charge Units to mAh\n"));
    Serial.print(F("2-Set Charge Units to Coulombs\n"));
    Serial.print(F("3-Set Temperature Units to Celsius\n"));
    Serial.print(F("4-Set Temperature Units to Kelvin\n"));
    Serial.print(F("m-Main Menu\n\n"));
    Serial.print(F("Enter a command: "));

    user_command = read_int();
    if (user_command == 'm')
      Serial.println("m");
    else
      Serial.println(user_command);
    Serial.println();
    switch (user_command)
    {
      case 1:
        *mAh_or_Coulombs = 0;
        Serial.println(F("\nCharge Units Set to mAh\n"));
        break;
      case 2:
        *mAh_or_Coulombs = 1;
        Serial.println(F("\nCharge Units Set to Coulombs\n"));
        break;
      case 3:
        *celcius_or_kelvin = 0;
        Serial.println(F("\nTemperature Units Set to Celcius\n"));
        break;
      case 4:
        *celcius_or_kelvin = 1;
        Serial.println(F("\nTemperature Units Set to Kelvin\n"));
        break;
      default:
        if (user_command != 'm')
          Serial.println("Incorrect Option");
        break;
    }

  }
  while (!((user_command == 'm') || (ack)));
  return(ack);

}

//! Checks to see if a bit in a certain position is set.
bool isBitSet(uint8_t value, uint8_t position)
//! @return Returns the state of a bit at "position" in a byte. 1 = Set, 0 = Not Set
{
  return((1<<position)&value);
}
//! Check Alerts Function - Checks to see if an alert has been set in the status register. If an alert has been set, it prints out the appropriate message.
void checkAlerts(uint8_t status_code)
//! @return
{
  if (isBitSet(status_code,6))
  {
    Serial.print(F("\n***********************\n"));
    Serial.print(F("Alert: "));
    Serial.print(F("Current Alert\n"));
    Serial.print(F("***********************\n"));
  }
  if (isBitSet(status_code,5))
  {
    Serial.print(F("\n***********************\n"));
    Serial.print(F("Alert: "));
    Serial.print(F("Charge Over/Under Flow Alert\n"));
    Serial.print(F("***********************\n"));
  }
  if (isBitSet(status_code,4))
  {
    Serial.print(F("\n***********************\n"));
    Serial.print(F("Alert: "));
    Serial.print(F("Temperature Alert\n"));
    Serial.print(F("***********************\n"));
  }
  if (isBitSet(status_code,3))
  {
    Serial.print(F("\n***********************\n"));
    Serial.print(F("Alert: "));
    Serial.print(F("Charge High Alert\n"));
    Serial.print(F("***********************\n"));
  }
  if (isBitSet(status_code,2))
  {
    Serial.print(F("\n***********************\n"));
    Serial.print(F("Alert: "));
    Serial.print(F("Charge Low Alert\n"));
    Serial.print(F("***********************\n"));
  }
  if (isBitSet(status_code,1))
  {
    Serial.print(F("\n***********************\n"));
    Serial.print(F("Alert: "));
    Serial.print(F("Voltage Alert\n"));
    Serial.print(F("***********************\n"));
  }
  if (isBitSet(status_code,0))
  {
    Serial.print(F("\n***********************\n"));
    Serial.print(F("Alert: "));
    Serial.print(F("UVLO Alert\n"));
    Serial.print(F("***********************\n"));
  }
}