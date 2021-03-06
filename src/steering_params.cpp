#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <sensor_msgs/Joy.h>
#include <stdio.h>
#include <string>
#include <actionlib_msgs/GoalID.h>
//#include <actionlib/client/simple_action_client.h>
//#include <actionlib/client/terminal_state.h>
//#include <move_base_msgs/MoveBaseAction.h>

ros::Publisher jt_pub, nav_pub;
ros::Publisher actionlib_pub;
ros::Subscriber sub, sub_nav;
geometry_msgs::Twist cmd_vel;

//set by params
std::string type;
int linearX,linearY,angularZ, brake, unlock, home;
double max_vel_x, max_angular_z;//in m/s or m/s^2
double normalized_min_x;

void JoyCallback( const sensor_msgs::Joy::ConstPtr& js)
{
	bool   unlockOn   = false;
	if(type=="gamepad")
	{
		cmd_vel.linear.x  = js->axes[linearX]; //to/fro
		cmd_vel.linear.z  = js->buttons[brake];             //brake
		cmd_vel.linear.y  = js->buttons[unlock];            //unlock
		cmd_vel.angular.z = js->axes[angularZ]; //rotate left/right
		cmd_vel.angular.x = js->buttons[home];    //homing

		if( js->buttons[brake])
		{
			actionlib_msgs::GoalID cancel_msg;
			actionlib_pub.publish(cancel_msg);
		}

	}
	else if (type=="joystick")
	{
		cmd_vel.linear.x  = js->axes[linearX]; //to/fro
		cmd_vel.linear.z  = js->buttons[brake];             //brake
		cmd_vel.linear.y  = js->buttons[unlock];            //unlock
		cmd_vel.angular.z = js->axes[angularZ]; //rotate left/right
		cmd_vel.angular.x = js->axes[home];    //homing
	}
	else
	{
		cmd_vel.linear.x =  0.0;
		cmd_vel.angular.z = 0.0;
		cmd_vel.linear.y  = 0.0;
		ROS_INFO("No valid controller");
	}

		unlockOn   = cmd_vel.linear.y;
		if( cmd_vel.linear.z==1.0 )
		{
			cmd_vel.linear.x = 0.0;
		}

		if( unlockOn==0  )
		{
			cmd_vel.angular.z = 0.0;
			cmd_vel.linear.x  = 0.0;
		}
	ROS_INFO("%s: cmd_vel: lx:%.3f,ly:%.3f,lz:%.3f,az:%.3f,ax:%.3f", type.c_str(), cmd_vel.linear.x, cmd_vel.linear.y,cmd_vel.linear.z, cmd_vel.angular.z, cmd_vel.angular.x);

	jt_pub.publish( cmd_vel );
}

void NavCallback( const geometry_msgs::Twist::ConstPtr& vel)
{
	double ratio = (vel->linear.x !=0.0) ? vel->linear.x + normalized_min_x : 0.0;
	if(ratio>1.0)ratio =1.0;

	geometry_msgs::Twist out_vel;
	out_vel.linear.x =  ratio;
	out_vel.angular.z = vel->angular.z;

	ROS_INFO("From NavCallback: received %.3f conv to %.3f.Ratio: %.3f Angular: %.3f,%.3f", vel->linear.x, out_vel.linear.x, ratio,vel->angular.z, out_vel.angular.z);
	nav_pub.publish(out_vel);
}


int main(int argc, char** argv)
{
    ros::init(argc,argv,"buggy");
    ros::NodeHandle nh("steering");

    nh.param<std::string>("type", type,"null");
    nh.param("linearX",linearX,0);
    nh.param("linearY",linearY,3);
    nh.param("angularZ",angularZ,2);
    nh.param("brake", brake ,0);
    nh.param("unlock",unlock,1);
    nh.param("home",home,0);


    nh.param<double>("max_vel_x",max_vel_x,1.0);
    nh.param<double>("max_angular_z",max_angular_z,1.0);
	nh.param<double>("normalized_min_x",normalized_min_x,0.4);


    cmd_vel.linear.x=0.0;
    cmd_vel.linear.y=0.0;
    cmd_vel.linear.z=0.0;
    cmd_vel.angular.x=0.0;
    cmd_vel.angular.y=0.0;
    cmd_vel.angular.z=0.0;

    nav_pub = nh.advertise<geometry_msgs::Twist>("/joy_vel",1);	//Changed from cmd_vel -> joy_vel
	jt_pub = nh.advertise<geometry_msgs::Twist>("/nav_vel_mod",1);	//Changed from cmd_vel -> joy_vel
	actionlib_pub = nh.advertise<actionlib_msgs::GoalID>("/move_base/cancel",1);

    sub=nh.subscribe<sensor_msgs::Joy>("/joy",10, JoyCallback);
	sub_nav=nh.subscribe<geometry_msgs::Twist>("/nav_vel",10, NavCallback);


    ROS_INFO("%s: Axes: %d, %d, %d",type.c_str(),linearX, linearY, angularZ);
	ROS_INFO("max_vel_x: %.2f m/s, max_angular_z: %.3f", max_vel_x, max_angular_z);
    while(ros::ok())
    {
    	ros::spinOnce();
    }


	return 0;
}
