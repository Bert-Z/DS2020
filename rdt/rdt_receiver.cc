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
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_check.h"

const int HEADER_SIZE = 9;
const int WINDOWS_SIZE = 10;
const int MAX_PAYLOAD_SIZE = (RDT_PKTSIZE - HEADER_SIZE);

//当前消息
static message *cur_message;
//当前message还未收到的数据的第一个byte的偏移量
static int cursor_receiver;

//应该收到的packet
static int packet_expected;
// Buffer
static packet *buffer;
// Buffer 有效位 (1为有效， 0为无效)
static char *valid_checks;

static void send_ack(int ack)
{
    packet ack_packet;
    memcpy(ack_packet.data + CHECKSUM_SIZE, &ack, sizeof(int));
    RDT_AddChecksum(&ack_packet);

    Receiver_ToLowerLayer(&ack_packet);
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    cur_message = (message *)malloc(sizeof(message));
    memset(cur_message, 0, sizeof(message));
    cursor_receiver = 0;

    packet_expected = 0;
    buffer = (packet *)malloc(WINDOWS_SIZE * sizeof(packet));
    valid_checks = (char *)malloc(WINDOWS_SIZE);
    memset(valid_checks, 0, WINDOWS_SIZE);
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    free(cur_message);
    free(buffer);
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
    if (packet_seq > packet_expected && packet_seq < packet_expected + WINDOWS_SIZE)
    {
        //存入buffer
        if (!valid_checks[packet_seq % WINDOWS_SIZE])
        {
            memcpy(&(buffer[packet_seq % WINDOWS_SIZE].data), pkt->data, RDT_PKTSIZE);
            valid_checks[packet_seq % WINDOWS_SIZE] = 1;
        }
        send_ack(packet_expected - 1);
        return;
    }
    else if (packet_seq != packet_expected)
    {
        send_ack(packet_expected - 1);
        return;
    }

    while (true)
    {
        packet_expected++;
        payload_size = pkt->data[HEADER_SIZE - 1];

        //第一个包
        if (cursor_receiver == 0)
        {
            if (cur_message->size != 0)
                free(cur_message->data);
            memcpy(&cur_message->size, pkt->data + HEADER_SIZE, sizeof(int));
            cur_message->data = (char *)malloc(cur_message->size);
            memcpy(cur_message->data, pkt->data + HEADER_SIZE + sizeof(int), payload_size - sizeof(int));
            cursor_receiver += payload_size - sizeof(int);
        }
        else
        {
            memcpy(cur_message->data + cursor_receiver, pkt->data + HEADER_SIZE, payload_size);
            cursor_receiver += payload_size;
        }

        if (cur_message->size == cursor_receiver)
        {
            Receiver_ToUpperLayer(cur_message);
            cursor_receiver = 0;
        }

        //从buffer中检查是否有包
        if (valid_checks[packet_expected % WINDOWS_SIZE])
        {
            pkt = &buffer[packet_expected % WINDOWS_SIZE];
            memcpy(&packet_seq, pkt->data + CHECKSUM_SIZE, sizeof(int));
            valid_checks[packet_expected % WINDOWS_SIZE] = 0;
        }
        else
        {
            break;
        }
    }

    send_ack(packet_seq);
}