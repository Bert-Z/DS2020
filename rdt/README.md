# Lab1 Reliable Data Transport Protocol

> ID: 516072910091
> Name: 张万强
> Email: wanqiangzhang@sjtu.edu.cn



## Packet Format

### Sender -- Message infomation packet

|<- 4 byte -> |<- 4 byte -> |  <- 1 byte ->    |  <- 1 byte ->  |   <- 4 byte ->   |<- The rest ->|

| checksum  | packet seq | message seq | payload size | message size |   payload      |



### Sender -- Following packet

|<- 4 byte -> |<- 4 byte ->|   <- 1 byte ->   |  <- 1 byte ->  | <- The rest  ->|

| checksum  | packet seq | message seq | payload size |     payload     |



### Receiver -- Ack

|<- 4 byte -->|<- 4 byte ->|<-       the rest      ->|

|  checksum |       ack       |<-       nothing       ->|



## Protocol Methods

- Go-Back N
- At the sender side:
  - There is a **buffer** ( a 1000-message-long array ) storing the messages attained from the upper layer, in order to retain the unprocessed messages.
  - Once the sender get the message, it will construct some packets -- one **message infomation packet** and some **following payload packets**.
  - The packets will be filled in the sender sliding windows ( the max window size is **10** ). Only when the sender receives the acknowledge packet from the receiver side, the sliding window can remove the received pkts and provide more space for the following packets.
  - If one packet get timeout, sender will resend the packet.
- At the receiver side:
  - There is also a sliding window ( the max window size is **10** ), in order to store the received packets and reorder them later.
  - Construct the message by the **packet sequence**.
- Checksum -- **crc32 checksum**



## Feelings

I am really grateful for the help of [dynamicheart-2015](https://github.com/dynamicheart/ds-labs) and [gousaiyang-2015](https://github.com/gousaiyang/ds-labs) . I didn't design my packet format perfectly at first and it caused a lot of bugs after I finished, for instance, the program would free the packet pointer before it had been sent. I tried to fix them but eventually got failed. Then I learnt about the methods of these two seniors and finished implementing my lab. 