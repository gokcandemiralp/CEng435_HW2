#pragma pack(push, 1)
struct {
  unsigned int id;
  char reserved[12];
} myAck;

#pragma pack(push, 1)
struct {
  unsigned int id;
  char content[12];
} myPacket;