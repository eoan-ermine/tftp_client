#include <boost/asio.hpp>

#ifdef BUILD_EXAMPLES
#include "../../../tftp_common/tftp_common.hpp"
#else
#include <tftp_common/tftp_common.hpp>
#endif

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

using namespace tftp_common::packets;
using boost::asio::ip::udp;

namespace tftp_client {

template <typename Packet>
bool try_parse(std::uint8_t *data, std::size_t len, Packet &packet) {
    auto [result, _] = parse(data, len, packet);
    if (result)
        return true;
    else {
        Error errPacket;
        auto [result, _] = parse(data, len, errPacket);
        if (result)
            throw errPacket;
    }
    return false;
}

class TFTPClient {
    udp::resolver &resolver;
    udp::endpoint &receiverEndpoint, senderEndpoint;
    udp::socket &socket;
    std::vector<std::uint8_t> sendBuffer, recvBuffer;

  public:
    TFTPClient(udp::resolver &resolver, udp::endpoint &receiverEndpoint, udp::socket &socket)
        : resolver(resolver), receiverEndpoint(receiverEndpoint), socket(socket) {
        sendBuffer.resize(1024);
        recvBuffer.resize(1024);
    }

    void send(std::string fromPath, std::string toPath, std::string transferMode,
              const std::vector<std::string> &optionsNames, const std::vector<std::string> &optionsValues) {
        std::ifstream file{fromPath};
        if (!file && file.eof())
            return;

        std::size_t packetSize = Request(Type::WriteRequest, toPath, transferMode, optionsNames, optionsValues)
                                     .serialize(sendBuffer.begin());
        socket.send_to(boost::asio::buffer(sendBuffer, packetSize), receiverEndpoint);
        std::size_t bytesRead = socket.receive_from(boost::asio::buffer(recvBuffer), senderEndpoint);

        bool optionsSuccess = false;
        if (optionsNames.size() > 0) {
            OptionAcknowledgment optionAcknowledgmentPacket;
            bool success = try_parse(recvBuffer.data(), bytesRead, optionAcknowledgmentPacket);
            optionsSuccess = success;
        }

        std::string newPort = std::to_string(senderEndpoint.port());
        receiverEndpoint = *resolver.resolve(udp::v4(), receiverEndpoint.address().to_string(), newPort).begin();

        Acknowledgment acknowledgmentPacket;
        Data currentPacket;
        uint16_t currentBlock = 0;

        while (file && !file.eof()) {
            try_parse(recvBuffer.data(), bytesRead, acknowledgmentPacket);
            std::vector<std::uint8_t> dataBuffer(512);
            std::size_t packetSize;

            /*
                    Новый пакет данных следует формировать лишь в том случае, если
                    предыдущий был отправлен успешно, иначе следует отправлять прошлый
                    пакет данных
            */
            if (acknowledgmentPacket.getBlock() == currentBlock) {
                currentBlock += 1;

                file.read(reinterpret_cast<char *>(dataBuffer.data()), 512);
                dataBuffer.resize(file.gcount());
                packetSize = Data(currentBlock, std::move(dataBuffer)).serialize(sendBuffer.begin());
            }

            socket.send_to(boost::asio::buffer(sendBuffer, packetSize), receiverEndpoint);
            bytesRead = socket.receive_from(boost::asio::buffer(recvBuffer), senderEndpoint);
        }
    }

    void read(std::string fromPath, std::string toPath, std::string transferMode,
              const std::vector<std::string> &optionsNames, const std::vector<std::string> &optionsValues) {
        std::ofstream file{toPath};
        if (!file)
            return;

        std::size_t packetSize = Request(Type::ReadRequest, fromPath, transferMode, optionsNames, optionsValues)
                                     .serialize(sendBuffer.begin());
        socket.send_to(boost::asio::buffer(sendBuffer, packetSize), receiverEndpoint);
        std::size_t bytesRead = socket.receive_from(boost::asio::buffer(recvBuffer), senderEndpoint);

        bool optionsSuccess = false;
        if (optionsNames.size() > 0) {
            OptionAcknowledgment optionAcknowledgmentPacket;
            bool success = try_parse(recvBuffer.data(), bytesRead, optionAcknowledgmentPacket);
            optionsSuccess = success;
        }

        std::string newPort = std::to_string(senderEndpoint.port());
        receiverEndpoint = *resolver.resolve(udp::v4(), receiverEndpoint.address().to_string(), newPort).begin();

        if (optionsSuccess)
            bytesRead = socket.receive_from(boost::asio::buffer(recvBuffer), senderEndpoint);

        Data dataPacket;
        uint16_t currentBlock = 1;

        while (true) {
            try_parse(recvBuffer.data(), bytesRead, dataPacket);
            const auto &packetData = dataPacket.getData();
            std::size_t packetSize;

            /*
                    Новый пакет данных следует формировать лишь в том случае, если
                    предыдущий был отправлен успешно, иначе следует отправлять прошлый
                    пакет данных
            */
            if (dataPacket.getBlock() == currentBlock) {
                currentBlock += 1;
                file.write(reinterpret_cast<const char *>(packetData.data()), packetData.size());
                packetSize = Acknowledgment(dataPacket.getBlock()).serialize(sendBuffer.begin());
            }

            socket.send_to(boost::asio::buffer(sendBuffer, packetSize), receiverEndpoint);
            if (packetData.size() != 512)
                break;
            bytesRead = socket.receive_from(boost::asio::buffer(recvBuffer), senderEndpoint);
        }
    }
};

} // namespace tftp_client