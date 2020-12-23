/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:     pi.cpp                                                      */
/*    Authors:    James Pearman     Justin Hall                               */
/*    Created:    3 August 2020                                               */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include "vex.h"
#include<stdio.h>
#include<stdlib.h>
#include<string>
#include<iostream>
using namespace std;

/*----------------------------------------------------------------------------*/
/** @file    pi.cpp
  * @brief   class for Raspberry Pi communication
*//*--------------------------------------------------------------------------*/

using namespace vex;

/*---------------------------------------------------------------------------*/
/** @brief  Constructor                                                      */
/*---------------------------------------------------------------------------*/
// Create high priority task to handle incomming data
//
pi::pi() {
    state = pi_state::kStateSyncWait1;

    thread t1 = thread( receive_task, static_cast<void *>(this) );
    t1.setPriority(thread::threadPriorityHigh);
}

pi::~pi() {
}

/*---------------------------------------------------------------------------*/
/** @brief  Get the total number of good received packets                    */
/*---------------------------------------------------------------------------*/
int32_t
pi::get_packets() {
    return packets;
}
/*---------------------------------------------------------------------------*/
/** @brief  Get the total number of bad received packets                     */
/*---------------------------------------------------------------------------*/
int32_t
pi::get_errors() {
    return errors;
}
/*---------------------------------------------------------------------------*/
/** @brief  Get the number of timeouts that have been triggered              */
/*---------------------------------------------------------------------------*/
int32_t
pi::get_timeouts() {
    return timeouts;
}
/*---------------------------------------------------------------------------*/
/** @brief  Get the total number of bytes received                           */
/*---------------------------------------------------------------------------*/
int32_t
pi::get_total() {
    return total_data_received;
}

/*---------------------------------------------------------------------------*/
/** @brief  Get the last received map record                                 */
/*---------------------------------------------------------------------------*/
//
// The map record is copied to user supplied buffer
// length of the valid data is returned
// 
int32_t
pi::get_data( MAP_RECORD *map ) {
    int32_t length = 0;

    if( map != NULL ) {
        maplock.lock();
        memcpy( map, &last_map, sizeof(MAP_RECORD));
        length = last_payload_length;
        maplock.unlock();
    }

    return length;
}

/*---------------------------------------------------------------------------*/
/** @brief  Parse a single received byte                                     */
/*---------------------------------------------------------------------------*/
bool
pi::parse( uint8_t data ) {
    bool  bRecall = false;

    // 250mS interbyte timeout
    if( state != pi_state::kStateSyncWait1 && timer.time() > 250 ) {
      timeouts++;
      state = pi_state::kStateSyncWait1;
    }

    // reset timeout
    timer.clear();
    
    switch( state ) {
      /*----------------------------------------------------------------------*/
      // crude multi byte sync
      case pi_state::kStateSyncWait1:
        if( data == SYNC1 ) {
          state = pi_state::kStateSyncWait2;
        }
        break;

      case pi_state::kStateSyncWait2:
        state = pi_state::kStateSyncWait1;
        if( data == SYNC2 ) {
          state = pi_state::kStateSyncWait3;
        }
        break;
        
      case pi_state::kStateSyncWait3:
        state = pi_state::kStateSyncWait1;
        if( data == SYNC3 ) {
          state = pi_state::kStateSyncWait4;
        }
        break;
        
      case pi_state::kStateSyncWait4:
        state = pi_state::kStateSyncWait1;
        if( data == SYNC4 ) {
          state = pi_state::kStateLength;
          index = 0;
          payload_length = 0;
        }
        break;

      /*----------------------------------------------------------------------*/
      // get payload length
      case pi_state::kStateLength:
        // data is 2 byte little endian
        payload_length = (payload_length >> 8) + ((uint16_t)data << 8);

        if( index++ == 1 ) {
          state = pi_state::kStateSpare;
          index = 0;
          payload_type = 0;
        }
        break;

      /*----------------------------------------------------------------------*/
      // get packet type
      case pi_state::kStateSpare:
        // data is 2 byte little endian
        payload_type = (payload_type >> 8) + ((uint16_t)data << 8);

        if( index++ == 1 ) {
          state = pi_state::kStateCrc32;
          index = 0;
          payload_crc32 = 0;
        }
        break;

      /*----------------------------------------------------------------------*/
      // get payload crc32
      case pi_state::kStateCrc32:
        // data is 4 byte little endian
        payload_crc32 = (payload_crc32 >> 8) + ((uint32_t)data << 24);
        
        if( index++ == 3 ) {
          state = pi_state::kStatePayload;
          index = 0;
          calc_crc32 = 0;
        }
        break;

      /*----------------------------------------------------------------------*/
      // get payload data
      case pi_state::kStatePayload:
        if( index < sizeof(payload) ) {
          // add byte to buffer
          payload.bytes[index] = data;
          index++;

          // keep runnint crc32, save calculating all at once later
          calc_crc32 = Crc32Generate( &data, 1, calc_crc32  );
          
          // all data received ?
          if( index == payload_length ) {
            // check crc32
            if( payload_crc32 == calc_crc32 ) {
              state = pi_state::kStateGoodPacket;
              bRecall = true;
            }
            else {
              state = pi_state::kStateBadPacket;
              bRecall = true;
            }
          }
        }
        else {
          // if we end up here then error
          //
          state = pi_state::kStateBadPacket;
          bRecall = true;
        }
        break;

      case pi_state::kStateGoodPacket:
        if( payload_type == MAP_PACKET_TYPE ) {
          // lock access to last_map and copy data
          maplock.lock();
          memcpy( &last_map, &payload.map, sizeof(MAP_RECORD));
          maplock.unlock();
        }

        // timestamp this packet
        last_packet_time = timer.system();

        packets++;
        state = pi_state::kStateSyncWait1;
        break;

      case pi_state::kStateBadPacket:
        // bad packet
        errors++;
        state = pi_state::kStateSyncWait1;
        break;

      default:
        state = pi_state::kStateSyncWait1;
        break;
    }

    return bRecall;
}

/*---------------------------------------------------------------------------*/
/** @brief  Send request to the Jetson to ask for next packet                */
/*---------------------------------------------------------------------------*/
void
pi::request_map() {
    // check timeout and clear state machine if necessary
    if( state != pi_state::kStateSyncWait1 && timer.time() > 250 ) {
      state = pi_state::kStateSyncWait1;
    }

    // only send if we are waiting for a message
    if( state == pi_state::kStateSyncWait1 ) {
      // Send small message to pi asking for data
      // using simple C stdio for simplicity
      // serial1 is the second USB cdc channel dedicated to user programs
      // we use this rather than stdout so linefeed is not expanded
      //
      FILE *fp = fopen("/dev/serial1", "w");

      // This is arbitary message at the moment
      // just using ASCII for convienience and debug porposes
      //
      static char msg[] = "AA55CC3301\r\n"; 

      // send
      fwrite( msg, 1, strlen(msg), fp);
        
      // close, flush buffers
      fclose(fp);
    }
}

/*---------------------------------------------------------------------------*/
/** @brief  Receive and process receive data from Raspberry Pi               */
/*---------------------------------------------------------------------------*/
int
pi::receive_task( void *arg ) {
    if( arg == NULL)
      return(0);
      
    // get our Pi instance
    pi *instance = static_cast<pi *>(arg);

    // process one character at a time
    // getchar() is blocking and will call yield internally
    
    while(1) {
      // get the data sent from the Pi
      instance->total_data_received++;
      int move_to = getchar();
      Brain.Screen.printAt( 180, 20, "Move To Location: %c", move_to);

      int vel1,vel2;
      vel1 = getchar();
      vel1 = vel1 - '0';
      vel2 = getchar();
      vel2 = vel2 - '0';
      int motor_speed = 10*vel1 + vel2;
      Brain.Screen.printAt( 180, 40, "At Speed: %d", motor_speed);

      // If the data is received by the Pi
      // Open serial connection to send a response back to the Pi
      //FILE *fsend = fopen("/dev/serial1", "w");

      // process the data byte sent from the Pi
        if(move_to == 'a')
        {
          //Send Message back to RPi
          FILE *fsend1 = fopen("/dev/serial1", "w");
          char msg1[] = "Moving to A"; 
          fwrite( msg1, 1, strlen(msg1), fsend1);
          fclose(fsend1);

          Motor1.setVelocity(motor_speed, rpm);
          Motor2.setVelocity(motor_speed, rpm);
          Motor3.setVelocity(motor_speed, rpm);
          Motor1.rotateFor(fwd, 1, rev, false);   //rotate all motors simultaneously 
          Motor2.rotateFor(fwd, 1, rev, false);
          Motor3.rotateFor(fwd, 1, rev, true);   //wait to continue until motor3 finishes

          FILE *fsend2 = fopen("/dev/serial1", "w");
          char msg2[] = "At Location A"; 
          fwrite( msg2, 1, strlen(msg2), fsend2);
          // close serial connection, flush buffers
          fclose(fsend2);
        }
        if(move_to == 'b')
        {
          //Send Message back to Pi
          FILE *fsend1 = fopen("/dev/serial1", "w");
          char msg1[] = "Moving to B"; 
          fwrite( msg1, 1, strlen(msg1), fsend1);
          fclose(fsend1);

          Motor1.setVelocity(motor_speed, rpm);
          Motor2.setVelocity(motor_speed, rpm);
          Motor3.setVelocity(motor_speed, rpm);
          Motor1.rotateFor(reverse, 1, rev, false);   //rotate all motors simultaneously
          Motor2.rotateFor(reverse, 1, rev, false);
          Motor3.rotateFor(reverse, 1, rev, true);   //wait to continue until motor3 finishes

          FILE *fsend2 = fopen("/dev/serial1", "w");
          char msg2[] = "At Location B"; 
          fwrite( msg2, 1, strlen(msg2), fsend2);
          // close serial connection, flush buffers
          fclose(fsend2);
        }
        
        // parse returns true if there is more processing to do
        //while( instance->parse( (uint8_t)rxchar ) )
        //  this_thread::yield();

      // no need for sleep/yield as stdin is blocking
    }
}