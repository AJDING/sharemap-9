#include "udp.hpp"
#include <cstdlib>
#include <iostream>

int main(void)
{
    std::cout << "testing udp socket class..." << std::endl;

    anysignal::udp_sock s0;
    anysignal::udp_sock s1;
    s0.bind("udp://0.0.0.0:5619");
    s1.connect("udp://localhost:5619");

    std::string tx_message = "hello world";
    if (s1.send_ready(std::chrono::milliseconds(10)))
    {
        auto r = s1.send(tx_message.data(), tx_message.size());
        if (r != int(tx_message.size()))
        {
            std::cerr << "failed to send message " << r << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "sent message " << tx_message << std::endl;
    }
    else
    {
        std::cerr << "failed to send message!" << std::endl;
        return EXIT_FAILURE;
    }

    std::array<char, 1024> rx_message = {}; // null terminated string
    if (s0.recv_ready(std::chrono::milliseconds(10)))
    {
        auto r = s0.recv(rx_message.data(), rx_message.size());
        if (r < 0)
        {
            std::cerr << "failed to recv message " << r << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "received message " << rx_message.data() << std::endl;
        if (rx_message.data() != tx_message)
        {
            std::cerr << "message mismatch!" << std::endl;
            return EXIT_FAILURE;
        }
    }
    else
    {
        std::cerr << "failed to recv message!" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "udp socket class works!" << std::endl;
    return EXIT_SUCCESS;
}
