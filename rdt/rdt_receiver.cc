/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or
 *       reordering.  You will need to enhance it to deal with all these
 *       situations.  In this implementation, the ack packet format is laid out as
 *       the following:
 *
 *       |<-  4 byte -->|<-  4 byte  ->|<-             the rest            ->|
 *       |   check sum  |     ack      |<-             nothing             ->|
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_check.h"

// header size
const int HEADER_SIZE = 9;
// sliding window size
const int WINDOWS_SIZE = 10;
// max payload size
const int MAX_PAYLOAD_SIZE = (RDT_PKTSIZE - HEADER_SIZE);

// current message
static message *cur_msg;
// cursor for the receiving message
static int cursor;

// expected pkt sequence
static int expected;
// window
static packet *window;
// window check
static char *valid_checks;

static void Receiver_Ack(int ack)
{
    packet pkt;
    memcpy(pkt.data + CHECKSUM_SIZE, &ack, sizeof(int));
    RDT_AddChecksum(&pkt);

    Receiver_ToLowerLayer(&pkt);
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());

    // initialization
    cur_msg = (message *)malloc(sizeof(message));
    memset(cur_msg, 0, sizeof(message));
    cursor = 0;
    expected = 0;
    window = (packet *)malloc(WINDOWS_SIZE * sizeof(packet));
    valid_checks = (char *)malloc(WINDOWS_SIZE);
    memset(valid_checks, 0, WINDOWS_SIZE);
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    free(cur_msg);
    free(window);
    free(valid_checks);
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    if (!RDT_VerifyChecksum(pkt))
        return;

    int packet_seq = 0, payload_size = 0;
    memcpy(&packet_seq, pkt->data + CHECKSUM_SIZE, sizeof(int));
    if (packet_seq > expected && packet_seq < expected + WINDOWS_SIZE)
    {
        // store in the window
        if (!valid_checks[packet_seq % WINDOWS_SIZE])
        {
            memcpy(&(window[packet_seq % WINDOWS_SIZE].data), pkt->data, RDT_PKTSIZE);
            valid_checks[packet_seq % WINDOWS_SIZE] = 1;
        }
        Receiver_Ack(expected - 1);
        return;
    }
    else if (packet_seq != expected)
    {
        Receiver_Ack(expected - 1);
        return;
    }

    while (true)
    {
        expected++;
        payload_size = pkt->data[HEADER_SIZE - 1];

        // message info package
        if (cursor == 0)
        {
            if (cur_msg->size != 0)
                free(cur_msg->data);
            memcpy(&cur_msg->size, pkt->data + HEADER_SIZE, sizeof(int));
            cur_msg->data = (char *)malloc(cur_msg->size);
            memcpy(cur_msg->data, pkt->data + HEADER_SIZE + sizeof(int), payload_size - sizeof(int));
            cursor += payload_size - sizeof(int);
        }
        else
        {
            memcpy(cur_msg->data + cursor, pkt->data + HEADER_SIZE, payload_size);
            cursor += payload_size;
        }

        if (cur_msg->size == cursor)
        {
            Receiver_ToUpperLayer(cur_msg);
            cursor = 0;
        }

        // check some pkts received before by reording
        if (valid_checks[expected % WINDOWS_SIZE])
        {
            pkt = &window[expected % WINDOWS_SIZE];
            memcpy(&packet_seq, pkt->data + CHECKSUM_SIZE, sizeof(int));
            valid_checks[expected % WINDOWS_SIZE] = 0;
        }
        else
            break;
        
    }

    Receiver_Ack(packet_seq);
}