// Source file for main of the server
#include "packet.h"

int main(void){
    
    char body[] = "Hello World";

    Packet testPkt;
    testPkt.setSrc(0x00A0B1C2); // Give 4 bytes, only use last 3
    testPkt.setDest(0x00123456); // Give 4 bytes, only use last 3
    testPkt.setFlag(Packet::Flag::EMPTY);
    testPkt.setSeqNum(1);
    testPkt.setTotalCount(1);
    testPkt.setData(body, sizeof(body));
    testPkt.setCrc(69);

    testPkt.display();
    unsigned int bytes = 0;
    const char* serializedData = testPkt.Serialize(&bytes);

    Packet newPkt(serializedData);
    newPkt.display();

    return 1;
}