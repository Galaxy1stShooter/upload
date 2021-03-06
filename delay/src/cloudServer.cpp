#include <ros/ros.h>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <zmq.h>
#include "../include/delay/delay.pb.h"
//#include "../include/delay/delay.pb.cc"
#include <nav_msgs/Odometry.h>
#include "../include/delay/config.h"
using namespace std;

void* ctx = zmq_ctx_new();
void* server = zmq_socket(ctx, ZMQ_REP);
void* publisher = zmq_socket(ctx, ZMQ_PUB);
vector<void*> serverList = vector<void*>(5, zmq_socket(ctx, ZMQ_REP));

int numUav;

int response(){
    string send_str = "ok";

    //string转成zmq_msg_t
    zmq_msg_t send_msg;
    zmq_msg_init_size(&send_msg,send_str.length());
    memcpy(zmq_msg_data(&send_msg),send_str.c_str(),send_str.size());
    
    //发送zmq_msg_t
    int send_byte = zmq_msg_send(&send_msg,server,0);
    ROS_INFO("server response message ok(%d bytes) success.",send_byte);
    zmq_msg_close(&send_msg);

    return send_byte;
}

int response(int index){
    string send_str = "ok";

    //string转成zmq_msg_t
    zmq_msg_t send_msg;
    zmq_msg_init_size(&send_msg,send_str.length());
    memcpy(zmq_msg_data(&send_msg),send_str.c_str(),send_str.size());
    
    //发送zmq_msg_t
    int send_byte = zmq_msg_send(&send_msg,serverList[index],0);
    ROS_INFO("server response message ok(%d bytes) success.",send_byte);
    zmq_msg_close(&send_msg);

    return send_byte;
}

int publishMsg(delayMessage::DelayMsg delaymsg){
    //delaymsg.set_send_time(ros::Time::now().toSec());
    string send_str = delaymsg.SerializeAsString();

    //string转成zmq_msg_t
    zmq_msg_t send_msg;
    zmq_msg_init_size(&send_msg,send_str.length());
    memcpy(zmq_msg_data(&send_msg),send_str.c_str(),send_str.size());
    
    //发送zmq_msg_t
    int send_byte = zmq_msg_send(&send_msg,publisher,0);
    ROS_INFO("server publish message (%d bytes) of uav%d success.",send_byte, delaymsg.uav_id());
    zmq_msg_close(&send_msg);

    return send_byte;
}

// void status_callback(const nav_msgs::Odometry& status){
//     delayMessage::DelayMsg delaymsg;
//     delaymsg.set_uav_id(1);
//     delaymsg.set_lat(status.pose.pose.position.x);
//     delaymsg.set_lon(status.pose.pose.position.y);
//     delaymsg.set_alt(status.pose.pose.position.z);
//     delaymsg.set_vx(status.twist.twist.linear.x);
//     delaymsg.set_vy(status.twist.twist.linear.y);
//     delaymsg.set_vz(status.twist.twist.linear.z);

//     publishMsg(delaymsg);
// }

int main(int argc, char** argv)
{
    ros::init(argc, argv, "server_node");
    ros::NodeHandle nh;

    ros::param::get("~num_uav", numUav);

    for(int i=0;i<numUav;++i){
        if(0==zmq_bind(serverList[i], server_addr_list[i])){
            ROS_INFO("server %d bind success", i);//return 0 if success
        }
        else{
            perror("server bind failed:");
            return -1;
        }
    }

    // if(0==zmq_bind(server, server_addr)){
    //     ROS_INFO("server bind success");//return 0 if success
    // }
    // else{
    //     perror("server bind failed:");
    //     return -1;
    // }

    if(0 == zmq_bind(publisher, pubaddr)){
        ROS_INFO("server publisher bind success");//return 0 if success
    }else{
        perror("server publisher bind failed:");
        return -1;
    }

    delayMessage::DelayMsg delaymsg;
    ros::Rate loop_rate(10);

    while(ros::ok()){
        delaymsg.Clear();
        zmq_msg_t recv_msg;
        zmq_msg_init(&recv_msg);
        for(int i=0;i<numUav;++i){
            int recv_byte = zmq_msg_recv(&recv_msg,serverList[i],0);//ZMQ_DONTWAIT

            if(recv_byte > 0)
            {
                ROS_INFO("Server receive message (%d bytes) success.",recv_byte);
                response(i);

                string str;
                str.assign((char*)zmq_msg_data(&recv_msg),recv_byte);
                zmq_msg_close(&recv_msg);
                //    int index = str.find_first_of('\0');
                //    ROS_INFO("%d",index);
                //    delaymsg.ParseFromString(str.substr(0,index));
                delaymsg.ParseFromString(str);
                ROS_INFO("Server recieve: msg_id:%d send time:%.9lf",delaymsg.msg_id(),ros::Time::now().toSec()-delaymsg.send_time());
                ROS_INFO("Server recieve: id:%d, lat:%f, lon:%f, alt:%f, vx:%f, vy:%f, vz:%f", delaymsg.uav_id(), delaymsg.lat(), delaymsg.lon(), delaymsg.alt(), delaymsg.vx(), delaymsg.vy(), delaymsg.vz());

                publishMsg(delaymsg);
                //ROS_INFO("Server publish message");
            }    
        }

        ros::spinOnce();
        loop_rate.sleep();
    }

    // while(ros::ok()){
    //     delaymsg.Clear();
    //     zmq_msg_t recv_msg;
    //     zmq_msg_init(&recv_msg);
    //     int recv_byte = zmq_msg_recv(&recv_msg,server,0);//ZMQ_DONTWAIT

    //     if(recv_byte > 0)
    //     {
    //         ROS_INFO("Server receive message (%d bytes) success.",recv_byte);
    //         response();

    //         string str;
    //         str.assign((char*)zmq_msg_data(&recv_msg),recv_byte);
    //         zmq_msg_close(&recv_msg);
    //         //    int index = str.find_first_of('\0');
    //         //    ROS_INFO("%d",index);
    //         //    delaymsg.ParseFromString(str.substr(0,index));
    //         delaymsg.ParseFromString(str);
    //         ROS_INFO("Server recieve: msg_id:%d send time:%.9lf",delaymsg.msg_id(),delaymsg.send_time());
    //         ROS_INFO("Server recieve: id:%d, lat:%f, lon:%f, alt:%f, vx:%f, vy:%f, vz:%f", delaymsg.uav_id(), delaymsg.lat(), delaymsg.lon(), delaymsg.alt(), delaymsg.vx(), delaymsg.vy(), delaymsg.vz());

    //         publishMsg(delaymsg);
    //         //ROS_INFO("Server publish message");
    //     }    

    //     ros::spinOnce();
    //     loop_rate.sleep();
    // }

    zmq_close(server);
    zmq_ctx_destroy(ctx);

    ros::shutdown();
    return 0;

}