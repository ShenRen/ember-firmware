
/* 
 * File:   Motor.cpp
 * Author: Richard Greene
 * 
 * Interfaces with a motor controller via I2C
 * 
 * Created on March 13, 2014, 5:51 PM
 */

#include <Motor.h>
#include <MotorController.h>
#include <Settings.h>

#define DELAY_AFTER_RESET_MSEC  (500)

/// Public constructor, base class opens I2C connection and sets slave address
Motor::Motor(unsigned char slaveAddress) :
I2C_Device(slaveAddress)
{
}

/// Disables motors (base class closes I2C connection)
Motor::~Motor() 
{
    DisableMotors();
}    

/// Send a set of commands to the motor controller.  Returns false immediately 
/// if any of the commands cannot be sent.
bool Motor::SendCommands(std::vector<MotorCommand> commands)
{
    for(int i = 0; i < commands.size(); i++)
        if(!commands[i].Send(this))
            return false;
    return true;
}

/// Enable (engage) both motors.  Return false if they can't be enabled.
bool Motor::EnableMotors()
{
    return MotorCommand(MC_GENERAL_REG, MC_ENABLE).Send(this);
}

/// Disable (disengage) both motors.  Return false if they can't be disabled.
bool Motor::DisableMotors()
{
    return MotorCommand(MC_GENERAL_REG, MC_DISABLE).Send(this);    
}

/// Pause the current motor command(s) in progress (if any).
bool Motor::Pause()
{
    return MotorCommand(MC_GENERAL_REG, MC_PAUSE).Send(this);
}

/// Resume the  motor command(s) pending at last pause (if any).
bool Motor::Resume()
{
    return MotorCommand(MC_GENERAL_REG, MC_RESUME).Send(this);
}

/// Clear pending motor command(s).  Typical use would be after a pause, to
/// implement a cancel.
bool Motor::ClearPendingCommands()
{
    return MotorCommand(MC_GENERAL_REG, MC_CLEAR).Send(this);
}

/// Reset and initialize the motor controller.
bool Motor::Initialize()
{    
    std::vector<MotorCommand> commands;
    
    // perform a software reset
    if(!MotorCommand(MC_GENERAL_REG, MC_RESET).Send(this))
        return false;
    
    // wait for the reset to complete before sending any commands
    // (that would otherwise be erased as part of the reset)
    usleep(DELAY_AFTER_RESET_MSEC * 1000);
    
    // set up parameters applying to all Z motions
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_STEP_ANGLE,
                                    SETTINGS.GetInt(Z_STEP_ANGLE)));
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_UNITS_PER_REV, 
                                    SETTINGS.GetInt(Z_MICRONS_PER_REV)));
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_MICROSTEPPING, 
                                    SETTINGS.GetInt(Z_MICRO_STEP)));

    // set up parameters applying to all rotations
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_STEP_ANGLE, 
                                    SETTINGS.GetInt(R_STEP_ANGLE)));
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_UNITS_PER_REV, 
                   SETTINGS.GetInt(R_MILLIDEGREES_PER_REV) / R_SCALE_FACTOR));
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_MICROSTEPPING, 
                                    SETTINGS.GetInt(R_MICRO_STEP)));

    // enable the motors
    commands.push_back(MotorCommand(MC_GENERAL_REG, MC_ENABLE));
    
    // no interrupt is needed here since no movement was requested 
    return SendCommands(commands);        
}


/// Move the motors to their home position, with optional interrupt such that
/// it may be chained with GoToStartPosition() with only a single interrupt at 
/// the end of both.
bool Motor::GoHome(bool withInterrupt)
{
    std::vector<MotorCommand> commands;
    
    // set rotation parameters
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_JERK, 
                                    SETTINGS.GetInt(R_HOMING_JERK)));
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_SPEED, 
                   R_SPEED_FACTOR * SETTINGS.GetInt(R_HOMING_SPEED)));
           
    // rotate to the home position (but no more than a full rotation)
    commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_HOME,
                                    UNITS_PER_REVOLUTION));
    
    int homeAngle = SETTINGS.GetInt(R_HOMING_ANGLE) / R_SCALE_FACTOR;
    if(homeAngle != 0)
    {
        // rotate 60 degrees back
        commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_MOVE, homeAngle));
    }
    
    // set Z motion parameters
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_JERK,
                                    SETTINGS.GetInt(Z_HOMING_JERK)));
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_SPEED,
                   Z_SPEED_FACTOR * SETTINGS.GetInt(Z_HOMING_SPEED)));
                                               
    // go up to the Z home position (but no more than twice the max Z travel)
    commands.push_back(MotorCommand(MC_Z_ACTION_REG, MC_HOME,
                               -2 * SETTINGS.GetInt(Z_START_PRINT_POSITION)));
     
    if(withInterrupt)
    {  
        // request an interrupt when these commands are completed
        commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));
    }
    return SendCommands(commands);
}

/// Goes to home position (without interrupt), then lowers the build platform to
/// the PDMS in order to calibrate and/or start a print
bool Motor::GoToStartPosition()
{
    GoHome(false);
    
    std::vector<MotorCommand> commands;
    
    // set rotation parameters
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_JERK, 
                                    SETTINGS.GetInt(R_START_PRINT_JERK)));
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_SPEED, 
                   R_SPEED_FACTOR * SETTINGS.GetInt(R_START_PRINT_SPEED)));
      
    int startAngle = SETTINGS.GetInt(R_START_PRINT_ANGLE) / R_SCALE_FACTOR;
    if(startAngle != 0)
    {
        // rotate to the start position
        commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_MOVE, startAngle));
    }
    
    // set Z motion parameters
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_JERK,
                                    SETTINGS.GetInt(Z_START_PRINT_JERK)));
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_SPEED,
                   Z_SPEED_FACTOR * SETTINGS.GetInt(Z_START_PRINT_SPEED)));

    // move down to the PDMS
    commands.push_back(MotorCommand(MC_Z_ACTION_REG, MC_MOVE, 
                                    SETTINGS.GetInt(Z_START_PRINT_POSITION)));
    
    // request an interrupt when these commands are completed
    commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));
    
    return SendCommands(commands);
}

/// Separate the current layer 
bool Motor::Separate(LayerType currentLayerType, int nextLayerNum, 
                     LayerSettings& ls)
{
    int rSeparationJerk;
    int rSeparationSpeed;
    int rotation;
    int zSeparationJerk;
    int zSeparationSpeed;
    int deltaZ;
        
    // get the parameters for the current type of layer
    switch(currentLayerType)
    {
        case First:
            rSeparationJerk = ls.GetInt(nextLayerNum, FL_SEPARATION_R_JERK);
            rSeparationSpeed = ls.GetInt(nextLayerNum, FL_SEPARATION_R_SPEED);
            rotation = ls.GetInt(nextLayerNum, FL_ROTATION);
            zSeparationJerk = ls.GetInt(nextLayerNum, FL_SEPARATION_Z_JERK);
            zSeparationSpeed = ls.GetInt(nextLayerNum,FL_SEPARATION_Z_SPEED);
            deltaZ = ls.GetInt(nextLayerNum, FL_Z_LIFT);
            break;
            
        case BurnIn:
            rSeparationJerk = ls.GetInt(nextLayerNum, BI_SEPARATION_R_JERK);
            rSeparationSpeed = ls.GetInt(nextLayerNum, BI_SEPARATION_R_SPEED);
            rotation = ls.GetInt(nextLayerNum, BI_ROTATION);
            zSeparationJerk = ls.GetInt(nextLayerNum, BI_SEPARATION_Z_JERK);
            zSeparationSpeed = ls.GetInt(nextLayerNum, BI_SEPARATION_Z_SPEED);
            deltaZ = ls.GetInt(nextLayerNum, BI_Z_LIFT);
            break;
            
        case Model:
            rSeparationJerk = ls.GetInt(nextLayerNum, ML_SEPARATION_R_JERK);
            rSeparationSpeed = ls.GetInt(nextLayerNum, ML_SEPARATION_R_SPEED);
            rotation = ls.GetInt(nextLayerNum, ML_ROTATION);
            zSeparationJerk = ls.GetInt(nextLayerNum, ML_SEPARATION_Z_JERK);
            zSeparationSpeed = ls.GetInt(nextLayerNum, ML_SEPARATION_Z_SPEED);
            deltaZ = ls.GetInt(nextLayerNum, ML_Z_LIFT);
            break;
    }
        
    rSeparationSpeed *= R_SPEED_FACTOR;
    zSeparationSpeed *= Z_SPEED_FACTOR;
    rotation         /= R_SCALE_FACTOR;

    std::vector<MotorCommand> commands;

    // rotate the previous layer from the PDMS
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_JERK, 
                                    rSeparationJerk));
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_SPEED, 
                                    rSeparationSpeed));
    if(rotation != 0)
        commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_MOVE, -rotation));
    
    // lift the build platform
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_JERK, 
                                    zSeparationJerk));
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_SPEED, 
                                    zSeparationSpeed));
    
    if(deltaZ != 0)
        commands.push_back(MotorCommand(MC_Z_ACTION_REG, MC_MOVE, deltaZ));
    
    // request an interrupt when these commands are completed
    commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));
    
    return SendCommands(commands);
}

/// Go to the position for exposing the next layer (with optional jam recovery
/// motion first).
bool Motor::Approach(LayerType currentLayerType, int nextLayerNum, 
                     LayerSettings& ls, bool unJamFirst)
{
    int thickness = ls.GetInt(nextLayerNum, LAYER_THICKNESS);
    
    if(unJamFirst)
        if(!UnJam(currentLayerType, nextLayerNum, ls,false))
            return false;
            
    int deltaZ;
    int rotation;
    int rApproachJerk;
    int rApproachSpeed;
    int zApproachJerk;
    int zApproachSpeed;
        
    // get the parameters for the current type of layer
    switch(currentLayerType)
    {
        case First:
            deltaZ = ls.GetInt(nextLayerNum, FL_Z_LIFT);
            rApproachJerk = ls.GetInt(nextLayerNum, FL_APPROACH_R_JERK);
            rApproachSpeed = ls.GetInt(nextLayerNum, FL_APPROACH_R_SPEED);
            rotation = ls.GetInt(nextLayerNum, FL_ROTATION);
            zApproachJerk = ls.GetInt(nextLayerNum, FL_APPROACH_Z_JERK);
            zApproachSpeed = ls.GetInt(nextLayerNum, FL_APPROACH_Z_SPEED);
            break;
            
        case BurnIn:
            deltaZ = ls.GetInt(nextLayerNum, BI_Z_LIFT);
            rApproachJerk = ls.GetInt(nextLayerNum, BI_APPROACH_R_JERK);
            rApproachSpeed = ls.GetInt(nextLayerNum, BI_APPROACH_R_SPEED);
            rotation = ls.GetInt(nextLayerNum, BI_ROTATION);
            zApproachJerk = ls.GetInt(nextLayerNum, BI_APPROACH_Z_JERK);
            zApproachSpeed = ls.GetInt(nextLayerNum, BI_APPROACH_Z_SPEED);
            break;
            
        case Model:
            deltaZ = ls.GetInt(nextLayerNum, ML_Z_LIFT);
            rApproachJerk = ls.GetInt(nextLayerNum, ML_APPROACH_R_JERK);
            rApproachSpeed = ls.GetInt(nextLayerNum, ML_APPROACH_R_SPEED);
            rotation = ls.GetInt(nextLayerNum, ML_ROTATION);
            zApproachJerk = ls.GetInt(nextLayerNum, ML_APPROACH_Z_JERK);
            zApproachSpeed = ls.GetInt(nextLayerNum, ML_APPROACH_Z_SPEED);
            break;
    }
        
    rApproachSpeed   *= R_SPEED_FACTOR;
    zApproachSpeed   *= Z_SPEED_FACTOR;
    rotation         /= R_SCALE_FACTOR;

    std::vector<MotorCommand> commands;
    
    // rotate back to the PDMS
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_JERK, 
                                    rApproachJerk));
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_SPEED, 
                                    rApproachSpeed));
    if(rotation != 0)
        commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_MOVE,  rotation));
    
    // lower into position to expose the next layer
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_JERK, 
                                    zApproachJerk));
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_SPEED, 
                                    zApproachSpeed));
    if(thickness != deltaZ)
        commands.push_back(MotorCommand(MC_Z_ACTION_REG, MC_MOVE, 
                                                         thickness - deltaZ));
    
    // request an interrupt when these commands are completed
    commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));
    
    return SendCommands(commands);
}

/// Rotate the tray and lift the build head to inspect the print in progress.
bool Motor::PauseAndInspect(int rotation)
{    
    std::vector<MotorCommand> commands;
    
    // use same speeds & jerks as used for homing, since we're already separated     
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_JERK, 
                                    SETTINGS.GetInt(R_HOMING_JERK)));
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_SPEED, 
                   R_SPEED_FACTOR * SETTINGS.GetInt(R_HOMING_SPEED)));
           
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_JERK,
                                    SETTINGS.GetInt(Z_HOMING_JERK)));
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_SPEED,
                   Z_SPEED_FACTOR * SETTINGS.GetInt(Z_HOMING_SPEED)));

    // rotate the tray to cover stray light from the projector
    rotation /= R_SCALE_FACTOR;
    if(rotation != 0)
        commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_MOVE, -rotation));
    
    // lift the build head for inspection
    int h = SETTINGS.GetInt(INSPECTION_HEIGHT);
    if(h != 0)
        commands.push_back(MotorCommand(MC_Z_ACTION_REG, MC_MOVE, h));
    
    // request an interrupt when these commands are completed
    commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));
    
    return SendCommands(commands);
}

/// Rotate the tray and lower the build head from the inspection position,
/// to resume printing. 
bool Motor::ResumeFromInspect(int rotation)
{
    std::vector<MotorCommand> commands;

    // use same speeds & jerks as used for moving to start position, 
    // since we're already calibrated     
    // set rotation parameters
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_JERK, 
                                    SETTINGS.GetInt(R_START_PRINT_JERK)));
    commands.push_back(MotorCommand(MC_ROT_SETTINGS_REG, MC_SPEED, 
                   R_SPEED_FACTOR * SETTINGS.GetInt(R_START_PRINT_SPEED)));
      
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_JERK,
                                    SETTINGS.GetInt(Z_START_PRINT_JERK)));
    commands.push_back(MotorCommand(MC_Z_SETTINGS_REG, MC_SPEED,
                   Z_SPEED_FACTOR * SETTINGS.GetInt(Z_START_PRINT_SPEED)));

    // rotate the tray back into exposing position
    rotation /= R_SCALE_FACTOR;
    if(rotation != 0)
        commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_MOVE, rotation));
    
    // lower the build head for exposure
    int h = SETTINGS.GetInt(INSPECTION_HEIGHT);
    if(h != 0)
        commands.push_back(MotorCommand(MC_Z_ACTION_REG, MC_MOVE, -h));
    
    // request an interrupt when these commands are completed
    commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));
    
    return SendCommands(commands);
}

/// Attempt to recover from a jam by homing the build tray.  It's up to the 
/// caller to determine if the anti-jam sensor is successfully triggered
/// during the attempt.  This move (without the interrupt request)is also 
/// required before resuming after a manual recovery, in order first to  
/// align the tray correctly.
bool Motor::UnJam(LayerType currentLayerType, int nextLayerNum, 
                  LayerSettings& ls, bool withInterrupt)
{
    // assumes speed & jerk have already 
    // been set as needed for separation from the current layer type 
    
    int rotation;
        
    // get the separation rotation for the current type of layer
    switch(currentLayerType)
    {
        case First:
            rotation = ls.GetInt(nextLayerNum, FL_ROTATION);
            break;
            
        case BurnIn:
            rotation = ls.GetInt(nextLayerNum, BI_ROTATION);
            break;
            
        case Model:
            rotation = ls.GetInt(nextLayerNum, ML_ROTATION);
            break;
    }
        
    rotation /= R_SCALE_FACTOR;

    std::vector<MotorCommand> commands;
               
    // rotate to the home position (but no more than a full rotation)
    commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_HOME,
                                    UNITS_PER_REVOLUTION));
    if(rotation != 0)      
        commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_MOVE, -rotation));
  
    if(withInterrupt)
    {
        // request an interrupt when these commands are completed
        commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));
    }

    return SendCommands(commands);    
}