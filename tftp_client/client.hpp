#include <boost/asio.hpp>

#include <tftp/tftp.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

using namespace tftp::packets;
using boost::asio::ip::udp;

namespace tftp_client {

template <typename Packet> bool try_parse(std::uint8_t *data, std::size_t len, Packet &packet) {
    auto result = Parser<Packet>::parse(data, len);
    packet = std::move(result.get().Packet);

    if (result.isSuccess())
        return true;
    else {
        Error errPacket;
        auto result = Parser<Error>::parse(data, len);
        if (result.isSuccess())
            throw errPacket;
    }
    return false;
}

class TFTPClient {
    udp::resolver &resolver;
    udp::endpoint &receiverEndpoint, senderEndpoint;
    udp::socket &socket;
    std::vector<std::uint8_t> sendBuffer, recvBuffer;
    std::size_t blockSize = 512;

    std::size_t getBlockSize(const OptionAcknowledgment &packet) const {
        if (packet.hasOption("blksize")) {
            return std::atoi(packet.getOptionValue("blksize").data());
        }
        return 512;
    }

  public:
    TFTPClient(udp::resolver &resolver, udp::endpoint &receiverEndpoint, udp::socket &socket)
        : resolver(resolver), receiverEndpoint(receiverEndpoint), socket(socket) {
        sendBuffer.resize(2 * blockSize);
        recvBuffer.resize(2 * blockSize);
    }

    void send(std::string fromPath, std::string toPath, std::string transferMode,
              const std::vector<std::string> &optionsNames, const std::vector<std::string> &optionsValues) {
        std::ifstream file{fromPath};
        if (!file && file.eof())
            return;

        std::size_t packetSize = Request(tftp::packets::types::Type::WriteRequest, toPath, transferMode, optionsNames, optionsValues)
                                     .serialize(sendBuffer.begin());
        socket.send_to(boost::asio::buffer(sendBuffer, packetSize), receiverEndpoint);
        std::size_t bytesRead = socket.receive_from(boost::asio::buffer(recvBuffer), senderEndpoint);

        bool optionsSuccess = false;
        if (optionsNames.size() > 0) {
            OptionAcknowledgment optionAcknowledgmentPacket;
            optionsSuccess = try_parse(recvBuffer.data(), bytesRead, optionAcknowledgmentPacket);

            if (optionsSuccess)
                blockSize = getBlockSize(optionAcknowledgmentPacket);
        }

        std::string newPort = std::to_string(senderEndpoint.port());
        receiverEndpoint = *resolver.resolve(udp::v4(), receiverEndpoint.address().to_string(), newPort).begin();

        Acknowledgment acknowledgmentPacket;
        Data currentPacket;
        uint16_t currentBlock = 0;

        while (file && !file.eof()) {
            try_parse(recvBuffer.data(), bytesRead, acknowledgmentPacket);
            std::vector<std::uint8_t> dataBuffer(blockSize);
            std::size_t packetSize;

            /*
                    Новый пакет данных следует формировать лишь в том случае, если
                    предыдущий был отправлен успешно, иначе следует отправлять прошлый
                    пакет данных
            */
            if (acknowledgmentPacket.getBlock() == currentBlock) {
                currentBlock += 1;

                file.read(reinterpret_cast<char *>(dataBuffer.data()), blockSize);
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

        std::size_t packetSize = Request(tftp::packets::types::Type::ReadRequest, fromPath, transferMode, optionsNames, optionsValues)
                                     .serialize(sendBuffer.begin());
        socket.send_to(boost::asio::buffer(sendBuffer, packetSize), receiverEndpoint);
        std::size_t bytesRead = socket.receive_from(boost::asio::buffer(recvBuffer), senderEndpoint);

        bool optionsSuccess = false;
        if (optionsNames.size() > 0) {
            OptionAcknowledgment optionAcknowledgmentPacket;
            optionsSuccess = try_parse(recvBuffer.data(), bytesRead, optionAcknowledgmentPacket);

            if (optionsSuccess)
                blockSize = getBlockSize(optionAcknowledgmentPacket);
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
            if (packetData.size() != blockSize)
                break;
            bytesRead = socket.receive_from(boost::asio::buffer(recvBuffer), senderEndpoint);
        }
    }
};

} // namespace tftp_client