#define QUEUE_SIZE 256

volatile uint8_t queue_buf[QUEUE_SIZE];
volatile uint16_t queue_head = 0;
volatile uint16_t queue_tail = 0;
volatile bool received_correct_packet = false;

inline bool queue_is_empty() { return queue_head == queue_tail; }
inline bool queue_is_full() { return ((queue_tail + 1) % QUEUE_SIZE) == queue_head; }

bool queue_push(uint8_t b) {
    uint16_t next = (queue_tail + 1) % QUEUE_SIZE;
    if (next == queue_head) return false;
    queue_buf[queue_tail] = b;
    queue_tail = next;
    return true;
}

int queue_pop() {
    if (queue_is_empty()) return -1;
    uint8_t b = queue_buf[queue_head];
    queue_head = (queue_head + 1) % QUEUE_SIZE;
    return b;
}

void clear_queue() {
    queue_head = 0;
    queue_tail = 0;
}

// ==== PROTOCOL VARIABLES ====
volatile int pos = 0;
volatile int recv_index = 0;
volatile size_t msg_len = 0;
static uint8_t recv_buff[512];

// ==== STRUCT ====
struct DataPacket {
    int a;
    int b;
};

// ==== SEND FUNCTION ====
void send_data(uint8_t *data, size_t len, int id){
    uint8_t data_to_send[len + 7];
    data_to_send[0] = 0xAF;
    data_to_send[1] = 0xAF;
    data_to_send[2] = 0xAF;
    data_to_send[3] = 0xAF;
    data_to_send[4] = len;
    data_to_send[5] = id;
    memcpy(&data_to_send[6], data, len);

    uint8_t checksum = 0xAF ^ 0xAF ^ 0xAF ^ 0xAF; // start with headers
    checksum ^= data_to_send[4]; // len
    checksum ^= data_to_send[5]; // id
    for(size_t i = 0; i < len; i++) checksum ^= data_to_send[6+i]; // payload

    data_to_send[len + 6] = checksum;

    Serial.write(data_to_send, len + 7);
}


// ==== CALLBACK ====
void onDatarecv(uint8_t *data, volatile size_t *len, uint8_t *id){
    if(*id == 9 && *len == sizeof(DataPacket)){
        DataPacket pkt;
        memcpy(&pkt, data, sizeof(DataPacket));
        // Serial.println("Received struct:");
        // Serial.print("A = "); Serial.println(pkt.a);
        // Serial.print("B = "); Serial.println(pkt.b);
        received_correct_packet = (pkt.a == 6 && pkt.b == 7) || (pkt.a == 105 && pkt.b == 105);
        // 🔄 Echo back with +1 values
       
    }
}


/// ==== USB RECEIVE ISR ====
extern "C" void usbSerialRxInterrupt(uint8_t *data, uint32_t length){
    uint8_t id;

    for(uint32_t i=0; i<length; i++){
        switch(pos){
          case 0: case 1: case 2: case 3:
            if(data[i]==0xAF) pos++; else pos=0;
            break;
          case 4:
            msg_len = data[i];        // ← remove the “+1” here
            recv_index = 0;
            pos++;
            break;
          case 5:
            id = data[i];
            pos++;
            break;
          case 6:
            recv_buff[recv_index++] = data[i];
            if (recv_index >= msg_len)  // we expect exactly msg_len payload bytes
                pos++;
            break;
          case 7:
            {
                uint8_t ch = 0xAF ^ 0xAF ^ 0xAF ^ 0xAF;  // reset checksum calculation
                ch ^= msg_len;                // len
                ch ^= id;                     // id
                for(int j=0; j<recv_index; j++) ch ^= recv_buff[j];  // payload
                if(data[i] == ch){
                    size_t payload_len = recv_index ;
                    onDatarecv(recv_buff, &payload_len, &id);
                    // received_correct_packet = true;
                }
                pos = recv_index = msg_len = 0;
            }
            break;
        }
    }
}

void setup(){
    Serial.begin(115200);
    delay(1000);

}

void loop(){
    DataPacket pkt;

    if(received_correct_packet){
        received_correct_packet = false;
        pkt.a = 100;
        pkt.b = 100;
        send_data((uint8_t*)&pkt, sizeof(pkt), 9);

    }
}
