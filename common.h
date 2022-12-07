enum origin : unsigned char {SERVER,CLIENT};

#pragma pack(push, 1)
struct myPacket{
  unsigned int id;
  char packet_origin;
  char content[11];
};

