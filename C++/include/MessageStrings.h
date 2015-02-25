/* 
 * File:   MessageStrings.h
 * Author: Richard Greene
 * 
 * 
 *
 * Created on March 31, 2014, 4:57 PM
 */

#ifndef MESSAGESTRINGS_H
#define	MESSAGESTRINGS_H

#define PRINTER_STARTUP_MSG ("Autodesk Ember 3D Printer")
#define FW_VERSION_MSG ("Firmware version: ")
#define BOARD_SER_NUM_MSG ("Serial number: ")
#define NO_IP_ADDRESS ("None")

#define PRINTER_STATUS_FORMAT (", layer %d of %d, seconds left: %d")
#define LOG_STATUS_FORMAT ("%s %s %s")
#define ERROR_FORMAT "%s: %s"
#define LOG_ERROR_FORMAT (ERROR_FORMAT "\n")
#define LOG_MOTOR_EVENT ("motor interrupt: %d")
#define LOG_BUTTON_EVENT ("button interrupt: %d")
#define LOG_DOOR_EVENT ("door interrupt: %c")
#define LOG_KEYBOARD_INPUT ("keyboard input: %s")
#define LOG_UI_COMMAND ("UI command: %s")
#define LOG_WEB_COMMAND ("web command: %s")
#define LOG_TEMPERATURE_PRINTING ("printing layer #%d of %d: temperature = %g")
#define LOG_TEMPERATURE ("temperature = %g")
#define LOG_JAM_DETECTED ("jam detected at layer %d: temperature = %g")
#define LOG_NO_PROJECTOR_I2C ("no I2C connection to projector")

#define UNKNOWN_REGISTRATION_CODE ("unknown code")
#define UNKNOWN_REGISTRATION_URL ("unknown URL")

#endif	/* MESSAGESTRINGS_H */

