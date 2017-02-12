#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/select.h>
#include <stdint.h>
#include <memory>
#include <vector>

struct DeviceInfo
{
    uint8_t devclass;
    uint16_t vendorId;
    uint16_t deviceId;
    uint8_t serialNo[6];
    uint8_t hwRevision;
    uint8_t swRevision;
} __attribute__((packed));

struct Message
{
    Message() : data() { }

    bool isResponse;
    uint8_t receiver;
    uint8_t sender;
    uint8_t command;
    std::vector<uint8_t> data;

    template<typename T>
    void setData(T const & value)
    {
        data.resize(sizeof(T));
        memcpy(data.data(), &value, sizeof(T));
    }

    void setData(void const * ptr, uint8_t len)
    {
        data.resize(len);
        memcpy(data.data(), ptr, len);
    }

    void setNullData() { data.clear(); }

    template<typename T>
    bool getData(T & value) const
    {
        if(data.size() >= sizeof(T)) {
            memcpy(&value, data.data(), sizeof(T));
            return true;
        } else {
            return false;
        }
    }

    void makeResponse(uint8_t responder)
    {
        this->receiver = this->sender;
        this->sender = responder;
        this->isResponse = true;
    }
};

class Port
{
    friend class Cable;
private:
    int fd;
    static const uint8_t magic = 0x77;
public:
    Port() : fd(-1) { }

    void send(Message const & msg)
    {
        if(fd == -1) {
            return;
        }
        uint8_t data[6];
        data[0] = 0x77;
        data[1] = 0x20;
        if(msg.isResponse)
            data[1] |= 0x01; // Set response flag
        data[2] = msg.receiver;
        data[3] = msg.sender;
        data[4] = msg.command;
        data[5] = msg.data.size();

        uint16_t cs = 0;
        for(int i = 0; i < data[5]; i++) {
            cs += msg.data[i];
        }

        ::send(fd, data, 6, 0);
        ::send(fd, msg.data.data(), data[5], 0);
        ::send(fd, &cs, sizeof cs, 0);
    }

    bool receive(Message & msg)
    {
        if(fd == -1) {
            return false;
        }
        fd_set master;
        struct timeval tv;

        FD_ZERO(&master);
        FD_SET(fd, &master);

        tv.tv_sec = 0;
        tv.tv_usec = 100;

        int rdy = select(fd+1, &master, NULL, NULL, &tv);
        if(rdy < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if(FD_ISSET(fd, &master) == false) {
            return false;
        }
        char header[6];
        int len = recv(fd, header, sizeof header, MSG_WAITALL);
        if(len != sizeof(header)) {
            fprintf(stderr, "Invalid read: %d, %s\n", len, strerror(errno));
            exit(EXIT_FAILURE);
        }
        // One could check magic here...
        if(header[0] != magic) {
            fprintf(stderr, "Invalid magic: %02X\n", (int)header[0]);
        }

        msg.isResponse = !!(header[1] & 0x01);
        msg.receiver = header[2];
        msg.sender = header[3];
        msg.command = header[4];
        msg.data.resize(header[5]);

        memset(msg.data.data(), 0, msg.data.size());
        recv(fd, msg.data.data(), msg.data.size(), MSG_WAITALL);

        uint16_t cs0;
        recv(fd, &cs0, sizeof cs0, MSG_WAITALL);

        uint16_t cs1 = 0;
        for(uint i = 0; i < msg.data.size(); i++) {
            cs1 += msg.data[i];
        }

        if(cs0 != cs1) {
            fprintf(stderr, "Invalid checksum: %d != %d.\n", (int)cs0, (int)cs1);
            exit(EXIT_FAILURE);
        }

        return true;
    }
};

class Cable
{
    int fd[2];
public:
    Cable()
    {
        if(socketpair(AF_UNIX, SOCK_STREAM, 0, this->fd) != 0)
        {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    ~Cable()
    {
        close(0[this->fd]);
        close(1[this->fd]);
    }

    void connect(Port &p0, Port &p1)
    {
        p0.fd = this->fd[0];
        p1.fd = this->fd[1];
    }
};

class Node
{
    static void * threadmain(void * n)
    {
        ((Node*)n)->run();
        return nullptr;
    }
public:
    uint8_t address;
public:
    void start()
    {
        pthread_t thr;
        if(pthread_create(&thr, NULL, & threadmain, this) != 0)
        {
            fprintf(stderr, "Failed to spawn thread.");
            exit(EXIT_FAILURE);
        }
    }

    virtual void run() = 0;
};

class SerialNode : public Node
{
public:
    Port north;
    Port south;
    DeviceInfo info;
    char const * name;
public:
    SerialNode() { }

    void run() override
    {
        init();
        while(true)
        {
            poll(south, north);
            poll(north, south);
            update();
        }
    }

    virtual void init() { }

    virtual void update() { }

    virtual void processRequest(Port &, Message const &) { }

    virtual void processResponse(Port &, Message const &) { }

    void poll(Port & in, Port & out)
    {
        Message msg;
        if(in.receive(msg) == false) {
            return;
        }
        bool accept = (msg.receiver == address) || (msg.receiver == 0) || (msg.receiver == 0xFF);
        bool forward = (msg.receiver != address) && (msg.receiver != 0);

        if(forward) {
            // printf("[%02X] Forward msg (%02X,%02X)\n", (int)address, (int)msg.sender, (int)msg.receiver);
            out.send(msg);
        }
        if(accept == false) {
            return;
        }
        if(msg.isResponse) {
            processResponse(in, msg);
            return;
        }
        switch(msg.command)
        {
            case 0x01:
                msg.makeResponse(address);
                in.send(msg); // bounce back
                return;
            case 0x02:
            {
                uint8_t query;
                if(msg.getData(query))
                {
                    msg.makeResponse(address);
                    switch(query)
                    {
                        case 0x01:
                        {
                            struct {
                                uint8_t rsp;
                                DeviceInfo info;
                            } payload {
                                0x01,
                                info
                            };
                            msg.setData(payload);
                            in.send(msg);
                            return;
                        }
                        case 0x02:
                            msg.data.resize(strlen(name) + 2);
                            msg.data[0] = 0x02;
                            strcpy((char*)&msg.data[1], name);
                            in.send(msg);
                            return;
                    }
                }
                return;
            }
            default:
                processRequest(in, msg);
                return;
        }
    }
};

class Printer : public SerialNode
{
public:
    Printer() { }

    void processRequest(Port &, const Message &msg)
    {
        printf("[%02X] Received message from %02X to %02X: %.*s\n",
            (int)address,
            (int)msg.sender,
            (int)msg.receiver,
            (int)msg.data.size(),
            (char*)msg.data.data());
    }
};

class Sender : public SerialNode
{
public:
    Sender() { }

    void init() override
    {
        Message msg;
        msg.isResponse = false;
        msg.sender = address;
        msg.receiver = 0xFF;
        msg.command = 0x02;

        msg.setData<uint8_t>(0x02);

        north.send(msg);
    }

    void processResponse(Port &, const Message &msg)
    {
        switch(msg.command)
        {
            case 0x01:
                printf("[%02X] Discovered %02X\n", (int)address, (int)msg.sender);
                break;
            case 0x02:
            {
                uint8_t query;
                msg.getData(query);
                switch(query)
                {
                    case 0x01:
                    {
                        struct {
                            uint8_t msg;
                            DeviceInfo info;
                        } payload;
                        msg.getData(payload);
                        printf("[%02X] Query(%02X,Info) = { %d, %d, %d, %02X-%02X-%02X-%02X-%02X-%02X, %d, %d }\n",
                               (int)address,
                               (int)msg.sender,
                               (int)payload.info.devclass,
                               (int)payload.info.vendorId,
                               (int)payload.info.deviceId,
                               (int)payload.info.serialNo[0],
                                (int)payload.info.serialNo[1],
                                (int)payload.info.serialNo[2],
                                (int)payload.info.serialNo[3],
                                (int)payload.info.serialNo[4],
                                (int)payload.info.serialNo[5],
                               (int)payload.info.hwRevision,
                               (int)payload.info.swRevision);
                        break;
                    }
                    case 0x02:
                        printf("[%02X] Query(%02X,Name) = '%.*s'\n", (int)address, (int)msg.sender, (int)msg.data.size() - 1, msg.data.data() + 1);
                        break;
                }
            }
        }
    }
};


int main(int argc, char ** argv)
{
    Printer print0, print1, print2;
    Sender sender0;

    sender0.address = 0x10;
    print0.address = 0x40;
    print1.address = 0x20;
    print2.address = 0x30;

    sender0.info = DeviceInfo { 0x30, 0x0000, 0x0000, { 0,0,0,0,0,1 }, 0x00, 0x00 };
    sender0.name = "Sender 0";

    print0.info = DeviceInfo { 0x30, 0x0000, 0x0001, { 0,0,0,0,0,2 }, 0x00, 0x00 };
    print0.name = "Printer 0";

    print1.info = DeviceInfo { 0x30, 0x0000, 0x0001, { 0,0,0,0,0,3 }, 0x00, 0x00 };
    print1.name = "Printer 1";

    print2.info = DeviceInfo { 0x30, 0x0000, 0x0001, { 0,0,0,0,0,4 }, 0x00, 0x00 };
    print2.name = "Printer 2";

    Cable c0, c1, c2;

    c0.connect(sender0.north, print0.south);
    c1.connect(print0.north, print1.south);
    c2.connect(print1.north, print2.south);

    print0.start();
    print1.start();
    print2.start();
    sender0.start();


    while(true);
}



/*
void * str_send (void * iptr)
{
    int sock = *((int*)iptr);
    printf("Sender %d\n", sock);
    while(true)
    {
        char buffer[64];
        int len = read(STDIN_FILENO, buffer, sizeof buffer);
        if(len > 0)
        {
            write(sock, buffer, len);
        }
    }
}

void * str_receive (void * iptr)
{
    int sock = *((int*)iptr);
    printf("Receiver %d\n", sock);
    while(true)
    {
        fd_set master;
        struct timeval tv;

        FD_ZERO(&master);
        FD_SET(sock, &master);

        tv.tv_sec = 0;
        tv.tv_usec = 100;

        int rdy = select(sock+1, &master, NULL, NULL, &tv);
        if(rdy < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if(FD_ISSET(sock, &master))
        {
            char buffer[64];
            int len = recv(sock, buffer, sizeof buffer, 0);
            if(len > 0)
            {
                fwrite(buffer, 1, len, stdout);
            }
        }
        usleep(10);
    }
}
*/
