/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following: 
 *
 *       AckInfo:
 *       | 4 bytes  | 4 bytes      | 120 bytes    |
 *       | checksum | ack sequence | unused space |
 * 
 * 
 *       PacketInfo:
 *       | 4 bytes  | 4 bytes         | 1 byte       |  1 byte |  118 byte |
 *       | checksum | sequence number | payload size |  isend  |  payload  |
 * 
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_parameters.h"
#include "rdt_check.h"

// current message
static message *cur_msg;
// message size
// static int msg_size;
// message vector
static std::vector<PacketInfo> msg_vector;
// message cursor
// static int cursor;
// expect sequence
static uint32_t expected;
// buffer
static PacketInfo buffer[WINDOW_SIZE];
// valid check
static bool valid[WINDOW_SIZE];

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());

    // init
    // msg_size = 0;
    // cursor = 0;
    expected = 0;
    memset(buffer, 0, WINDOW_SIZE * sizeof(PacketInfo));
    memset(valid, 0, WINDOW_SIZE);
    msg_vector.clear();

    return;
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

// send ack
void Receiver_SendAck(uint32_t seq)
{

    AckInfo ackinfo(seq);
    packet ack;

    memcpy(ack.data, &ackinfo, sizeof(AckInfo));
    RDT_AddChecksum(&ack);

    Receiver_ToLowerLayer(&ack);

    return;
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    PacketInfo packetinfo;
    memcpy(&packetinfo, pkt, RDT_PKTSIZE);

    // verify checksum
    if (!RDT_VerifyChecksum(pkt))
        return;

    if (packetinfo.seqnum > expected && packetinfo.seqnum < expected + WINDOW_SIZE)
    {
        int setnum = packetinfo.seqnum % WINDOW_SIZE;
        // store in buffer
        if (!valid[setnum])
        {
            buffer[setnum] = packetinfo;
            valid[setnum] = 1;
        }

        Receiver_SendAck(expected - 1);
        return;
    }
    else if (expected != packetinfo.seqnum)
    {
        Receiver_SendAck(expected - 1);
        return;
    }

    while (true)
    {
        expected++;
        msg_vector.push_back(packetinfo);
        cur_msg->size += packetinfo.payload_size;

        if (packetinfo.isend)
        {
            cur_msg->data = (char *)malloc(cur_msg->size);
            int cursor = 0;
            for (auto x : msg_vector)
            {
                memcpy(cur_msg->data + cursor, x.payload, x.payload_size);
                cursor += x.payload_size;
            }
            Receiver_ToUpperLayer(cur_msg);
            break;
        }

        if (valid[expected % WINDOW_SIZE])
        {
            packetinfo = buffer[expected % WINDOW_SIZE];
            valid[expected % WINDOW_SIZE] = 0;
        }
        else
            break;
    }

    Receiver_SendAck(packetinfo.seqnum);
}
