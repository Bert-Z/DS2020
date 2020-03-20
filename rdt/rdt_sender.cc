/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or
 *       reordering.  You will need to enhance it to deal with all these
 *       situations.  In this implementation, the packet format is laid out as
 *       the following:
 *
 *   |<- 4 byte -> |<- 4 byte  ->|<-  1 byte  ->|<-  1 byte  ->|><-  The rest   ->|
 *   |  checksum   |  packet seq |  message seq | payload size |<-   payload    ->|
 *
 *
 *   Message infomation package:
 *   |<- 4 byte -> |<- 4 byte  ->|<-  1 byte  ->|<-  1 byte  ->|<-  4 byte  ->|<-  The rest   ->|
 *   |  checksum   |  packet seq |  message seq | payload size | message size |<-   payload   ->|
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"
#include "rdt_check.h"

// max message buffer size
const int MAX_BUFFER_SIZE = 15000;
// sliding window size
const int MAX_WINDOW_SIZE = 10;
const double TIMEOUT = 0.3;
const int HEADER_SIZE = 9;
const int MAX_PAYLOAD_SIZE = (RDT_PKTSIZE - HEADER_SIZE);

// message buffer
static message *buffer;
// message sequence
static int next_message_seq;
// message number stored in buffer
static int msg_buffer_num;
// current message sequence
static int cur_message_seq;
// cur msg cursor
static int cursor_sender;

// pkt sliding window
static packet *windows;
// pkt squence
static int pkt_seq;
// next pkt seq
static int next_packet_to_send;
// expected ack
static int ack_expected;
// pkt number stored in the window
static int pkt_window_num;

static void Sender_ConstructPKT()
{
    message msg = buffer[cur_message_seq % MAX_BUFFER_SIZE];

    packet pkt;

    while (pkt_window_num < MAX_WINDOW_SIZE && cur_message_seq < next_message_seq)
    {
        if (msg.size - cursor_sender > MAX_PAYLOAD_SIZE)
        {
            memcpy(pkt.data + CHECKSUM_SIZE, &pkt_seq, sizeof(int));
            pkt.data[HEADER_SIZE - 1] = MAX_PAYLOAD_SIZE;

            memcpy(pkt.data + HEADER_SIZE, msg.data + cursor_sender, MAX_PAYLOAD_SIZE);

            RDT_AddChecksum(&pkt);

            memcpy(&(windows[pkt_seq % MAX_WINDOW_SIZE]), &pkt, sizeof(packet));

            cursor_sender += MAX_PAYLOAD_SIZE;
            pkt_seq++;
            pkt_window_num++;
        }
        else if (msg.size > cursor_sender)
        {
            // last pkt of the cur msg
            memcpy(pkt.data + CHECKSUM_SIZE, &pkt_seq, sizeof(int));
            pkt.data[HEADER_SIZE - 1] = msg.size - cursor_sender;

            memcpy(pkt.data + HEADER_SIZE, msg.data + cursor_sender, msg.size - cursor_sender);

            RDT_AddChecksum(&pkt);

            memcpy(&(windows[pkt_seq % MAX_WINDOW_SIZE]), &pkt, sizeof(packet));

            msg_buffer_num--;
            cur_message_seq++;

            // if there are some msg have been buffered
            if (cur_message_seq < next_message_seq)
                msg = buffer[cur_message_seq % MAX_BUFFER_SIZE];

            cursor_sender = 0;
            pkt_seq++;
            pkt_window_num++;
        }
    }
}

static void Sender_SendPKT()
{
    while (next_packet_to_send < pkt_seq)
    {
        Sender_ToLowerLayer(&(windows[next_packet_to_send % MAX_WINDOW_SIZE]));
        next_packet_to_send++;
    }
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());

    // initialization
    buffer = (message *)malloc((MAX_BUFFER_SIZE) * sizeof(message));
    memset(buffer, 0, (MAX_BUFFER_SIZE) * sizeof(message));
    next_message_seq = 0;
    msg_buffer_num = 0;
    cur_message_seq = 0;
    cursor_sender = 0;
    windows = (packet *)malloc((MAX_WINDOW_SIZE) * sizeof(packet));
    pkt_seq = 0;
    next_packet_to_send = 0;
    ack_expected = 0;
    pkt_window_num = 0;
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
    {
        if (buffer[i].size != 0)
            free(buffer[i].data);
    }
    free(buffer);
    free(windows);
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    // Go-Back N
    if (buffer[next_message_seq % MAX_BUFFER_SIZE].size != 0)
        free(buffer[next_message_seq % MAX_BUFFER_SIZE].data);

    buffer[next_message_seq % MAX_BUFFER_SIZE].size = msg->size + sizeof(int);
    buffer[next_message_seq % MAX_BUFFER_SIZE].data = (char *)malloc(msg->size + sizeof(int));
    memcpy(buffer[next_message_seq % MAX_BUFFER_SIZE].data, &msg->size, sizeof(int));
    memcpy(buffer[next_message_seq % MAX_BUFFER_SIZE].data + sizeof(int), msg->data, msg->size);

    next_message_seq++;
    msg_buffer_num++;

    // Timer
    if (Sender_isTimerSet())
        return;

    Sender_StartTimer(TIMEOUT);

    Sender_ConstructPKT();
    Sender_SendPKT();
}

/* event handler, called when a packet is passed from the lower layer at the
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    if (!RDT_VerifyChecksum(pkt))
        return;

    int ack;
    memcpy(&ack, pkt->data + CHECKSUM_SIZE, sizeof(int));

    if (ack_expected <= ack && ack < pkt_seq)
    {
        // sliding window
        Sender_StartTimer(TIMEOUT);
        pkt_window_num -= (ack - ack_expected + 1);
        ack_expected = ack + 1;
        Sender_ConstructPKT();
        Sender_SendPKT();
    }

    if (ack == pkt_seq - 1)
        Sender_StopTimer();
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    Sender_StartTimer(TIMEOUT);
    next_packet_to_send = ack_expected;
    Sender_ConstructPKT();
    Sender_SendPKT();
}