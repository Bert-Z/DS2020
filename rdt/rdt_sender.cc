/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "rdt_struct.h"
#include "rdt_sender.h"
#include "rdt_parameters.h"
#include "rdt_check.h"

// current message
// static message *cur_msg;
// expected sequence
static uint32_t seq = 0;
// pkt arrays
static packet *sender_pkts[PKT_MAX_SEQ_NUM];
// pkts status
static bool valid[PKT_MAX_SEQ_NUM];

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
   fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
   fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

// Increment current sequence number.
void Sender_IncrementSeq()
{
   seq = (seq + 1) % PKT_MAX_SEQ_NUM;
}

// construct a pkt
void Sender_ContructPKT(int payload_size, bool is_end, uint32_t seq, const char *payload, packet *pkt)
{
   PacketInfo pktinfo = PacketInfo(seq, payload_size, is_end, payload);

   memcpy(pkt->data, &pktinfo, sizeof(PacketInfo));

   RDT_AddChecksum(pkt);
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
   ASSERT(msg);
   ASSERT(msg->size >= 0);

   if (!msg->size)
      return;

   // split message into many packets
   uint32_t pkt_nums = msg->size / PKT_MAX_PAYLOAD_SIZE + 1;

   for (uint32_t i = 0; i < pkt_nums; i++)
   {
      int cur_size = PKT_MAX_PAYLOAD_SIZE;
      bool is_end = false;
      if (i == pkt_nums - 1)
      {
         cur_size = msg->size - i * PKT_MAX_PAYLOAD_SIZE;
         is_end = true;
      }
      sender_pkts[seq] = (packet *)malloc(sizeof(packet));
      Sender_ContructPKT(cur_size, is_end, seq, msg->data + i * PKT_MAX_PAYLOAD_SIZE, sender_pkts[seq]);

      Sender_ToLowerLayer(sender_pkts[seq]);
      Sender_IncrementSeq();
   }
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{

   if (!RDT_VerifyChecksum(pkt))
      return;

   uint32_t seq;
   memcpy(&seq, pkt->data + 4, sizeof(uint32_t));
   valid[seq] = 1;
   free(sender_pkts[seq]);
   sender_pkts[seq] = nullptr;
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
}
