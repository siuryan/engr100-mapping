#include <iostream>
#include "Copter.h"
#include <string>
#include <fstream>
#include <cstring>
#include <cstdlib> 
#include <ctime> 

using namespace std;
/*
 * Init and run calls for autonomous flight mode (largely based off of the AltHold flight mode)
 */

// autonomous_init - initialise autonomous controller
bool Copter::autonomous_init(bool ignore_checks)
{
    // initialize vertical speeds and leash lengths
    pos_control->set_speed_z(-g.pilot_velocity_z_max, g.pilot_velocity_z_max);
    pos_control->set_accel_z(g.pilot_accel_z);

    // initialise position and desired velocity
    if (!pos_control->is_active_z()) {
        pos_control->set_alt_target_to_current_alt();
        pos_control->set_desired_velocity_z(inertial_nav.get_velocity_z());
    }

    // stop takeoff if running
    takeoff_stop();

    // reset integrators for roll and pitch controllers
    g.pid_roll.reset_I();
    g.pid_pitch.reset_I();

    return true;
}

// autonomous_run - runs the autonomous controller
// should be called at 100hz or more
void Copter::autonomous_run()
{
    AltHoldModeState althold_state;
    float takeoff_climb_rate = 0.0f;

    // initialize vertical speeds and acceleration
    pos_control->set_speed_z(-g.pilot_velocity_z_max, g.pilot_velocity_z_max);
    pos_control->set_accel_z(g.pilot_accel_z);

    // apply SIMPLE mode transform to pilot inputs
    update_simple_mode();

    // desired roll, pitch, and yaw_rate
    float target_roll = 0.0f, target_pitch = 0.0f, target_yaw_rate = 0.0f;

    // get pilot desired climb rate
    float target_climb_rate = get_pilot_desired_climb_rate(channel_throttle->get_control_in());
    target_climb_rate = constrain_float(target_climb_rate, -g.pilot_velocity_z_max, g.pilot_velocity_z_max);

#if FRAME_CONFIG == HELI_FRAME
    // helicopters are held on the ground until rotor speed runup has finished
    bool takeoff_triggered = (ap.land_complete && (target_climb_rate > 0.0f) && motors->rotor_runup_complete());
#else
    bool takeoff_triggered = ap.land_complete && (target_climb_rate > 0.0f);
#endif
    target_climb_rate = 0.0f;

    // Alt Hold State Machine Determination
    if (!motors->armed() || !motors->get_interlock()) {
        althold_state = AltHold_MotorStopped;
    } else if (takeoff_state.running || takeoff_triggered) {
        althold_state = AltHold_Takeoff;
    } else if (!ap.auto_armed || ap.land_complete) {
        althold_state = AltHold_Landed;
    } else {
        althold_state = AltHold_Flying;
    }

    // Alt Hold State Machine
    switch (althold_state) {

    case AltHold_MotorStopped:

        motors->set_desired_spool_state(AP_Motors::DESIRED_SHUT_DOWN);
        attitude_control->input_euler_angle_roll_pitch_euler_rate_yaw(target_roll, target_pitch, target_yaw_rate, get_smoothing_gain());
        attitude_control->reset_rate_controller_I_terms();
        attitude_control->set_yaw_target_to_current_heading();
#if FRAME_CONFIG == HELI_FRAME    
        // force descent rate and call position controller
        pos_control->set_alt_target_from_climb_rate(-abs(g.land_speed), G_Dt, false);
        heli_flags.init_targets_on_arming=true;
#else
        pos_control->relax_alt_hold_controllers(0.0f);   // forces throttle output to go to zero
#endif
        pos_control->update_z_controller();
        break;

    case AltHold_Takeoff:
#if FRAME_CONFIG == HELI_FRAME    
        if (heli_flags.init_targets_on_arming) {
            heli_flags.init_targets_on_arming=false;
        }
#endif
        // set motors to full range
        motors->set_desired_spool_state(AP_Motors::DESIRED_THROTTLE_UNLIMITED);

        // initiate take-off
        if (!takeoff_state.running) {
            takeoff_timer_start(constrain_float(g.pilot_takeoff_alt,0.0f,1000.0f));
            // indicate we are taking off
            set_land_complete(false);
            // clear i terms
            set_throttle_takeoff();
        }

        // get take-off adjusted pilot and takeoff climb rates
        takeoff_get_climb_rates(target_climb_rate, takeoff_climb_rate);

        // get avoidance adjusted climb rate
        target_climb_rate = get_avoidance_adjusted_climbrate(target_climb_rate);

        // call attitude controller
        attitude_control->input_euler_angle_roll_pitch_euler_rate_yaw(target_roll, target_pitch, target_yaw_rate, get_smoothing_gain());

        // call position controller
        pos_control->set_alt_target_from_climb_rate_ff(target_climb_rate, G_Dt, false);
        pos_control->add_takeoff_climb_rate(takeoff_climb_rate, G_Dt);
        pos_control->update_z_controller();
        break;

    case AltHold_Landed:
        // set motors to spin-when-armed if throttle below deadzone, otherwise full range (but motors will only spin at min throttle)
        if (target_climb_rate < 0.0f) {
            motors->set_desired_spool_state(AP_Motors::DESIRED_SPIN_WHEN_ARMED);
        } else {
            motors->set_desired_spool_state(AP_Motors::DESIRED_THROTTLE_UNLIMITED);
        }

#if FRAME_CONFIG == HELI_FRAME    
        if (heli_flags.init_targets_on_arming) {
            attitude_control->reset_rate_controller_I_terms();
            attitude_control->set_yaw_target_to_current_heading();
            if (motors->get_interlock()) {
                heli_flags.init_targets_on_arming=false;
            }
        }
#else
        attitude_control->reset_rate_controller_I_terms();
        attitude_control->set_yaw_target_to_current_heading();
#endif
        attitude_control->input_euler_angle_roll_pitch_euler_rate_yaw(target_roll, target_pitch, target_yaw_rate, get_smoothing_gain());
        pos_control->relax_alt_hold_controllers(0.0f);   // forces throttle output to go to zero
        pos_control->update_z_controller();
        break;

    case AltHold_Flying:
        // compute the target climb rate, roll, pitch and yaw rate
        // land if autonomous_controller returns false
        if (!autonomous_controller(target_climb_rate, target_roll, target_pitch, target_yaw_rate)) {
            // switch to land mode
            set_mode(LAND, MODE_REASON_MISSION_END);
            break;
        }

        motors->set_desired_spool_state(AP_Motors::DESIRED_THROTTLE_UNLIMITED);

#if AC_AVOID_ENABLED == ENABLED
        // apply avoidance
        avoid.adjust_roll_pitch(target_roll, target_pitch, aparm.angle_max);
#endif

        // call attitude controller
        attitude_control->input_euler_angle_roll_pitch_euler_rate_yaw(target_roll, target_pitch, target_yaw_rate, get_smoothing_gain());

        // adjust climb rate using rangefinder
        if (rangefinder_alt_ok()) {
            // if rangefinder is ok, use surface tracking
            target_climb_rate = get_surface_tracking_climb_rate(target_climb_rate, pos_control->get_alt_target(), G_Dt);
        }

        // get avoidance adjusted climb rate
        target_climb_rate = get_avoidance_adjusted_climbrate(target_climb_rate);

        // call position controller
        pos_control->set_alt_target_from_climb_rate_ff(target_climb_rate, G_Dt, false);
        pos_control->update_z_controller();
        break;
    }
}

// autonomous_controller - computes target climb rate, roll, pitch, and yaw rate for autonomous flight mode
// returns true to continue flying, and returns false to land
bool Copter::autonomous_controller(float &target_climb_rate, float &target_roll, float &target_pitch, float &target_yaw_rate)
{
    // get downward facing sensor reading in meters
    float rangefinder_alt = (float)rangefinder_state.alt_cm / 100.0f;

    // get horizontal sensor readings in meters
    float dist_forward, dist_right, dist_backward, dist_left;
    g2.proximity.get_horizontal_distance(0, dist_forward);
    g2.proximity.get_horizontal_distance(90, dist_right);
    g2.proximity.get_horizontal_distance(180, dist_backward);
    g2.proximity.get_horizontal_distance(270, dist_left);

    
    // set desired climb rate in centimeters per second
    target_climb_rate = 0.0f;

    /*
    // set desired roll and pitch in centi-degrees
    //target_pitch = 0.0f;
    g.pid_pitch.set_input_filter_all(0.5f - dist_forward);
    target_pitch = 100.0f * g.pid_pitch.get_pid();

    g.pid_roll.set_input_filter_all(dist_right- dist_left);
    target_roll = 100.0f * g.pid_roll.get_pid();
    //target_roll = 0.0f;
    */
    
    // set desired yaw rate in centi-degrees per second (set to zero to hold constant heading)
    target_yaw_rate = 0.0f;

    //const char *s_dist_forward = to_string(dist_forward).c_str();

    static int num_time = time(0) % 10000;
	static string filename = to_string(num_time) += "mapping_data.csv";


    static ofstream os(filename);
    static int c = 0; 
    if(c++%50==0){
        //c%50 can be 100. lets see what data we get.//need to understand what one second is. 
        Vector3f accel = ins.get_accel();
	float p = 100.0f * g.pid_pitch.get_pid();
	float r = 100.0f * g.pid_roll.get_pid();
        logging(os,c,dist_forward,dist_right,dist_backward,dist_left,accel,
        p, r);
    }
    
    static int counter = 0;
    /*if(counter++ > 400){
	    gcs_send_text(MAV_SEVERITY_INFO, "Autonomous flight mode for GameOfDrones");
	    //gcs_send_text(MAV_SEVERITY_INFO, s_dist_forward);	
    	

	    //Debugging print statements - Make sure the thresholds are actually set on Mission Planner
	    //Check to make sure the variable is accessible
	    const char *ptr_distThreshold = to_string(distThreshold).c_str();
	    const char *ptr_centerThreshold = to_string(centerThreshold).c_str();

	    gcs_send_text(MAV_SEVERITY_INFO, "distThreshold");
		gcs_send_text(MAV_SEVERITY_INFO, ptr_distThreshold);
		gcs_send_text(MAV_SEVERITY_INFO, "centerThreshold");
	    gcs_send_text(MAV_SEVERITY_INFO, ptr_centerThreshold);

	    // Print the four lidars readings to see if they are correct (might be out of range and cause problems)

	    // print each message in a line
	    const char *ptr_left = to_string(dist_left).c_str();
	    const char *ptr_right = to_string(dist_right).c_str();
	    const char *ptr_front = to_string(dist_forward).c_str();
	    const char *ptr_back = to_string(dist_backward).c_str();
	    gcs_send_text(MAV_SEVERITY_INFO, "Left");
	    gcs_send_text(MAV_SEVERITY_INFO, ptr_left);
	    gcs_send_text(MAV_SEVERITY_INFO, "Right");
	    gcs_send_text(MAV_SEVERITY_INFO, ptr_right);
	    gcs_send_text(MAV_SEVERITY_INFO, "Front");
	    gcs_send_text(MAV_SEVERITY_INFO, ptr_front);
	    gcs_send_text(MAV_SEVERITY_INFO, "Back");
	    gcs_send_text(MAV_SEVERITY_INFO, ptr_back);


	    counter = 0;
    }*/

    // The C format version to print data for debugiing
    if(counter++ > 400) {
    	gcs_send_text(MAV_SEVERITY_INFO, "wowowo Autonomous flight mode for GameOfDrones, C format printing \n");

    	gcs_send_text_fmt(MAV_SEVERITY_INFO, "distThreshold is %.2f \n", distThreshold);
    	gcs_send_text_fmt(MAV_SEVERITY_INFO, "centerThreshold is %.2f \n", centerThreshold);
    	/*gcs_send_text_fmt(MAV_SEVERITY_INFO, "Left: %.2f \n", dist_left);
    	gcs_send_text_fmt(MAV_SEVERITY_INFO, "Right: %.2f \n", dist_right);
    	gcs_send_text_fmt(MAV_SEVERITY_INFO, "Front: %.2f \n", dist_forward);
    	gcs_send_text_fmt(MAV_SEVERITY_INFO, "Back: %.2f \n", dist_backward);*/
    	gcs_send_text_fmt(MAV_SEVERITY_INFO, "backDirection: %i \n", backDirection);
	gcs_send_text_fmt(MAV_SEVERITY_INFO, "prevbackDirection: %i \n", prevBackDirection);
    	counter = 0;
    }

    //Project Implementation Below//
    //                            //
    //                            //

    
    //Centers the drone between adjacent walls at every iteration
    center_drone(target_roll, target_pitch, dist_forward, 
    dist_right, dist_backward, dist_left, counter);

    //Assuming back direction is not the front and the sensors detect 
    //a reasonable distance then move the drone forward
    if(backDirection != Direction::front && dist_forward > distThreshold){
        g.pid_pitch.set_input_filter_all(distThreshold - dist_forward);
        target_pitch = 100.0f * g.pid_pitch.get_pid();
        
        //backDirection is now backwards
	prevBackDirection = backDirection;
        backDirection = Direction::back;



        //Debugging print statements
	if(counter > 400) {
		gcs_send_text(MAV_SEVERITY_INFO, "Moving Forward");
	}
        
    }
    //Move the drone to the right
    else if(backDirection != Direction::right && dist_right > distThreshold){
        //Positive roll should be to the right
        g.pid_roll.set_input_filter_all(dist_right - distThreshold);
        target_roll = 100.0f * g.pid_roll.get_pid();

        //backDirection is now to the left
	//prevBackDirection = backDirection;
        //backDirection = Direction::left;
	
	if(prevBackDirection==Direction::front){
	  backDirection = Direction::front;
	}
	else{
	  prevBackDirection = backDirection;
          backDirection = Direction::left;	
	}	

        //Debugging print statements
        if(counter > 400) gcs_send_text(MAV_SEVERITY_INFO, "Moving Right");
    }
    //Move the drone backward
    else if(backDirection != Direction::back && dist_backward > distThreshold){
        g.pid_pitch.set_input_filter_all(dist_backward - distThreshold);
        target_pitch = 100.0f * g.pid_pitch.get_pid();

        //backDirection is now to the front
	prevBackDirection = backDirection;
        backDirection = Direction::front;

        //Debugging print statements
        if(counter > 400) gcs_send_text(MAV_SEVERITY_INFO, "Moving Backward");
    }
    //Move the drone to the left
    else if(backDirection != Direction::left && dist_left > distThreshold){
        g.pid_roll.set_input_filter_all(distThreshold - dist_left);
        target_roll = 100.0f * g.pid_roll.get_pid();

        //backDirection is now to the right
	prevBackDirection = backDirection;
        backDirection = Direction::right;

        //Debugging print statements
        if(counter > 400) gcs_send_text(MAV_SEVERITY_INFO, "Moving Left");
    }

    // need a landing command, uncomment later...
    else {
    	return false;
    }
    
   
    return true;
}
//EFFECTS: Centers the drone between its adjacent walls depending on what
//the drone sees as backwards
void Copter::center_drone(float &target_roll, float &target_pitch, float &dist_forward, 
    float &dist_right, float &dist_backward, float &dist_left, int count){

    //If the back direction is the front or back and the walls are not too far away 
    //then center between left and right walls
    if((backDirection == Direction::front || backDirection == Direction::back) && 
        (dist_right < centerThreshold || dist_left < centerThreshold)){
        g.pid_roll.set_input_filter_all((dist_right - dist_left) / 2);
        target_roll = 100.0f * g.pid_roll.get_pid();

        //Debugging print statements
		if(count > 400) {
        	gcs_send_text(MAV_SEVERITY_INFO, "Centering Between Left and Right Walls");
		}
    }	
    //If the back direction is left or right and the walls are not too far away
    //then center between front and back walls
    else if((backDirection == Direction::right || backDirection == Direction::left) && 
        (dist_forward < centerThreshold || dist_backward < centerThreshold)){
        g.pid_pitch.set_input_filter_all((dist_backward - dist_forward) / 2);
        target_pitch = 100.0f * g.pid_pitch.get_pid();

        //Debugging print statements
		if(count > 400) {
			gcs_send_text(MAV_SEVERITY_INFO, "Centering Between Front and Back Walls");
		}
    }
	++count;
	if(count > 400) {
			gcs_send_text(MAV_SEVERITY_INFO, "Centering running");
	}


    return;
}
//LOGGING:
void Copter::logging(ofstream &os,int counter,float &dist_forward, 
    float &dist_right, float &dist_backward, float &dist_left, Vector3f &accel,float &pitch,float &roll) const{
    os<<counter<<","<<dist_right<<","<<dist_backward<<","<<dist_left<<","<<dist_forward<<","<<accel.y<<","<<
    accel.x<<","<<pitch<<","<<roll<<endl;
}

